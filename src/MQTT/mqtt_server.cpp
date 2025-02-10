#include "mqtt/mqtt_server.h"

#include "iostream"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_heap_trace.h"


#include "mongoose/mongoose.h"
#include "ui/ui.h"
#include "utils/helper_functions.h"

static const char *s_listen_on = "mqtt://0.0.0.0:1883";

#define MAX_TOPICS_COUNT 12
#define KEEP_ALIVE_INTERVAL_IN_SECONDS 60
#define CHECK_INTERVAL_IN_SECONDS 10

std::vector<std::shared_ptr<mqtt_client>> s_clients = {};
std::vector<std::shared_ptr<mqtt_sub>> s_subs = {};
std::vector<std::shared_ptr<mqtt_will>> s_wills = {};

// Limited to 12 topics!
// topic name | subscriber count | current_value
std::map<std::string, std::pair<int, std::string>> s_topics = {};

std::recursive_mutex mqtt_server_mutex;

// #define NUM_RECORDS 100
// static heap_trace_record_t trace_record[NUM_RECORDS]; // This buffer must be in internal RAM

static size_t mg_mqtt_next_topic(struct mg_mqtt_message *msg, struct mg_str *topic, uint8_t *qos, size_t pos);
size_t mg_mqtt_next_sub(struct mg_mqtt_message *msg, struct mg_str *topic, uint8_t *qos, size_t pos);
size_t mg_mqtt_next_unsub(struct mg_mqtt_message *msg, struct mg_str *topic, size_t pos);
void _mg_mqtt_dump(char *tag, struct mg_mqtt_message *msg);
int _mg_mqtt_parse_header(struct mg_mqtt_message *msg, struct mg_str *client, struct mg_str *topic, struct mg_str *payload, uint8_t *qos, uint8_t *retain);
int isasciis(char *buffer, int length);
void handle_mqtt_connect(struct mg_connection *c, struct mg_mqtt_message *mm);
void handle_mqtt_subscribe(struct mg_connection *c, struct mg_mqtt_message *mm);
void handle_mqtt_unsubscribe(struct mg_connection *c, struct mg_mqtt_message *mm);
void handle_mqtt_publish(struct mg_connection *c, struct mg_mqtt_message *mm);
void handle_mqtt_pingreq(struct mg_connection *c);
void handle_mqtt_close(struct mg_connection *c);

static size_t mg_mqtt_next_topic(struct mg_mqtt_message *msg,
                                 struct mg_str *topic, uint8_t *qos,
                                 size_t pos)
{
    unsigned char *buf = (unsigned char *)msg->dgram.buf + pos;
    size_t new_pos;
    if (pos >= msg->dgram.len)
        return 0;

    topic->len = (size_t)(((unsigned)buf[0]) << 8 | buf[1]);
    topic->buf = (char *)buf + 2;
    new_pos = pos + 2 + topic->len + (qos == NULL ? 0 : 1);
    if ((size_t)new_pos > msg->dgram.len)
        return 0;
    if (qos != NULL)
        *qos = buf[2 + topic->len];
    return new_pos;
}

size_t mg_mqtt_next_sub(struct mg_mqtt_message *msg, struct mg_str *topic,
                        uint8_t *qos, size_t pos)
{
    uint8_t tmp;
    return mg_mqtt_next_topic(msg, topic, qos == NULL ? &tmp : qos, pos);
}

size_t mg_mqtt_next_unsub(struct mg_mqtt_message *msg, struct mg_str *topic,
                          size_t pos)
{
    return mg_mqtt_next_topic(msg, topic, NULL, pos);
}

#define WILL_FLAG 0x04
#define WILL_QOS 0x18
#define WILL_RETAIN 0x20

int _mg_mqtt_parse_header(struct mg_mqtt_message *msg, struct mg_str *client, struct mg_str *topic, struct mg_str *payload, uint8_t *qos, uint8_t *retain)
{
    client->len = 0;
    topic->len = 0;
    payload->len = 0;
    unsigned char *buf = (unsigned char *)msg->dgram.buf;
    int Protocol_Name_length = buf[2] << 8 | buf[3];
    int Connect_Flags_position = Protocol_Name_length + 5;
    uint8_t Connect_Flags = buf[Connect_Flags_position];
    ESP_LOGI("_mg_mqtt_parse_header", "Connect_Flags=%x", Connect_Flags);
    uint8_t Will_Flag = (Connect_Flags & WILL_FLAG) >> 2;
    *qos = (Connect_Flags & WILL_QOS) >> 3;
    *retain = (Connect_Flags & WILL_RETAIN) >> 5;
    ESP_LOGI("_mg_mqtt_parse_header", "Will_Flag=%d *qos=%x *retain=%x", Will_Flag, *qos, *retain);
    client->len = buf[Connect_Flags_position + 3] << 8 | buf[Connect_Flags_position + 4];
    client->buf = (char *)&buf[Connect_Flags_position + 5];
    ESP_LOGI("_mg_mqtt_parse_header", "client->len=%d", client->len);
    if (Will_Flag == 0)
        return 0;

    int Will_Topic_position = Protocol_Name_length + client->len + 10;
    topic->len = buf[Will_Topic_position] << 8 | buf[Will_Topic_position + 1];
    topic->buf = (char *)&(buf[Will_Topic_position]) + 2;
    ESP_LOGI("_mg_mqtt_parse_header", "topic->len=%d topic->buf=[%.*s]", topic->len, topic->len, topic->buf);
    int Will_Payload_position = Will_Topic_position + topic->len + 2;
    payload->len = buf[Will_Payload_position] << 8 | buf[Will_Payload_position + 1];
    payload->buf = (char *)&(buf[Will_Payload_position]) + 2;
    ESP_LOGI("_mg_mqtt_parse_header", "payload->len=%d payload->buf=[%.*s]", payload->len, payload->len, payload->buf);
    return 1;
}

void check_timeouts()
{
    std::lock_guard<std::recursive_mutex> lock(mqtt_server_mutex);

    auto now = std::chrono::steady_clock::now();
    for (auto it : s_clients)
    {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->last_seen).count();
        ESP_LOGI("check_timeouts", "CLIENT %p [%.*s] last seen \"%d\" seconds ago.", it->c->fd, (int)it->cid.len, it->cid.buf, duration);
        if (duration > KEEP_ALIVE_INTERVAL_IN_SECONDS)
        {
            ESP_LOGI("check_timeouts", "Hence closing the Connection to CLIENT %p [%.*s].", it->c->fd, (int)it->cid.len, it->cid.buf);
            mg_error(it->c, "Keep-alive timeout");
            mg_mqtt_disconnect(it->c, NULL);
            it->c->is_closing = 1;

            // uint8_t response[] = {0, 0};
            // mg_mqtt_send_header(it->c, MQTT_CMD_DISCONNECT, 0, sizeof(response)); // Send acknowledgement
            // mg_send(it->c, response, sizeof(response));
            // handle_mqtt_close(it->c);
        }
        else if (duration > (KEEP_ALIVE_INTERVAL_IN_SECONDS * 0.5))
        {
            ESP_LOGI("check_timeouts", "Hence sending PINGREQ to CLIENT %p [%.*s].", it->c->fd, (int)it->cid.len, it->cid.buf);
            // mg_mqtt_send_header(it->c, MQTT_CMD_PINGREQ, 0, 0);
            // mg_send(it->c, NULL, 0); // forces flush by sending 0 package
            mg_mqtt_ping(it->c);
        }
    }
}

void update_last_seen(mg_connection *c)
{
    for (auto client = s_clients.begin(); client != s_clients.end(); client++)
    {
        if (c == (*client)->c)
        {
            (*client)->last_seen = std::chrono::steady_clock::now();
            ESP_LOGI("update_last_seen", "CLIENT %p [%.*s] updated last_seen", c->fd, (int)(*client)->cid.len, (*client)->cid.buf);
            break;
        }
    }
}

std::shared_ptr<mqtt_client> find_connection(mg_connection *c)
{
    for (auto client = s_clients.begin(); client != s_clients.end(); client++)
    {
        if (c == (*client)->c)
        {
            return *client;
        }
    }
    return NULL;
}

void handle_mqtt_connect(struct mg_connection *c, struct mg_mqtt_message *mm)
{
    update_last_seen(c);

    if (mm->dgram.len < 9) // Check if the message is malformed
    {
        mg_error(c, "Malformed MQTT frame");
    }
    else if (mm->dgram.buf[8] != 4) // Check if the MQTT version is supported
    {
        mg_error(c, "Unsupported MQTT version %d", mm->dgram.buf[8]);
    }
    else
    {
        struct mg_str cid;
        struct mg_str topic;
        struct mg_str payload;
        uint8_t qos;
        uint8_t retain;

        int willFlag = _mg_mqtt_parse_header(mm, &cid, &topic, &payload, &qos, &retain);
        ESP_LOGI("handle_mqtt_connect", "cid=[%.*s] willFlag=%d", cid.len, cid.buf, willFlag);

        if(cid.len == 0)
        {
            mg_error(c, "Client ID not found");
            return;
        }

        
        std::shared_ptr<mqtt_client> client = std::make_shared<mqtt_client>();

        client->c = c;
        client->cid.buf = mg_mprintf("%.*s", cid.len, cid.buf);

        client->cid.len = cid.len;

        s_clients.push_back(client);
        ESP_LOGI("handle_mqtt_connect", "CLIENT ADD \"%.*s\"", (int)client->cid.len, client->cid.buf);

        // if (willFlag == 1)
        // {
        //     std::shared_ptr<mqtt_will> will = std::make_shared<mqtt_will>();
        //     will->c = c;

        //     will->topic.buf = mg_mprintf("%.*s", topic.len, topic.buf);
        //     will->topic.len = topic.len;

        //     will->payload.buf = mg_mprintf("%.*s", payload.len, payload.buf);
        //     will->payload.len = payload.len;

        //     will->qos = qos;
        //     will->retain = retain;
        //     s_wills.push_back(will);
        //     ESP_LOGI("handle_mqtt_connect", "WILL ADD %p [%.*s] [%.*s] %d %d",
        //              c->fd, (int)will->topic.len, will->topic.buf, (int)will->payload.len, will->payload.buf, will->qos, will->retain);
        // }

        uint8_t response[] = {0, 0};
        mg_mqtt_send_header(c, MQTT_CMD_CONNACK, 0, sizeof(response));
        mg_send(c, response, sizeof(response));
    }
    xEventGroupSetBits(ui_event_group, UI_STATUSBAR_UPDATE_BIT);
}

void handle_mqtt_subscribe(struct mg_connection *c, struct mg_mqtt_message *mm)
{
    bool reload_flag = false;
    ESP_LOGI("handle_mqtt_subscribe", "MQTT_CMD_SUBSCRIBE");
    update_last_seen(c);

    int pos = 4;
    uint8_t qos, resp[256];
    mg_str topic;
    int num_topics = 0;

    while ((pos = mg_mqtt_next_sub(mm, &topic, &qos, pos)) > 0)
    {
        std::shared_ptr<mqtt_sub> sub = std::make_shared<mqtt_sub>();
        sub->c = c;
        sub->topic.buf = mg_mprintf("%.*s", topic.len, topic.buf);
        sub->topic.len = topic.len;
        sub->qos = qos;

        // Is the topic already in the s_topics list?
        if (s_topics.find(std::string(sub->topic.buf, (int)sub->topic.len)) == s_topics.end())
        {
            // If not, and we have reached the MAX_TOPICS_COUNT, we will not add the topic
            // and send an error response to the client
            if (s_topics.size() >= MAX_TOPICS_COUNT) // Max MAX_TOPICS_COUNT topics allowed
            {
                ESP_LOGW("handle_mqtt_subscribe", "Too many topics subscribed. Max %d topics allowed.", MAX_TOPICS_COUNT);
                // Send a custom response to the client
                resp[num_topics++] = 0x80;
                continue;
            }
            else // If MAX_TOPICS_COUNT has not been reached, we will add the topic to the s_topics list
            {
                s_topics[std::string(sub->topic.buf, (int)sub->topic.len)].first = 1;
                ESP_LOGI("handle_mqtt_subscribe", "NEW TOPIC [%.*s] added to s_topics\nTriggering UI_RELOAD", (int)sub->topic.len, sub->topic.buf);
                reload_flag = true;
            }
        }
        else // If the topic is already in the s_topics list, we will increment the count
        {
            s_topics[std::string(sub->topic.buf, (int)sub->topic.len)].first++;
            ESP_LOGI("handle_mqtt_subscribe", "TOPIC [%.*s] got incremented", (int)sub->topic.len, sub->topic.buf);
        }

        // Add the subscription to the s_subs list
        s_subs.push_back(sub);
        ESP_LOGI("handle_mqtt_subscribe", "SUB ADD [%.*s]", (int)sub->topic.len, sub->topic.buf);

        // Replace '+' with '*' in the topic (no idea why but it's in the original code)
        for (size_t i = 0; i < sub->topic.len; i++)
        {
            if (sub->topic.buf[i] == '+')
                ((char *)sub->topic.buf)[i] = '*';
        }
        resp[num_topics++] = qos;
    }
    // Send the response to the client
    mg_mqtt_send_header(c, MQTT_CMD_SUBACK, 0, num_topics + 2);
    uint16_t id = mg_htons(mm->id);
    mg_send(c, &id, 2);
    mg_send(c, resp, num_topics);

    if (reload_flag) // If a reload flag was set, we will set the UI_RELOAD_BIT to trigger an UI reload
    {
        xEventGroupSetBits(ui_event_group, UI_RELOAD_BIT);
    }
}

void handle_mqtt_unsubscribe(struct mg_connection *c, struct mg_mqtt_message *mm)
{
    bool reload_flag = false;
    ESP_LOGI("handle_mqtt_unsubscribe", "MQTT_CMD_UNSUBSCRIBE");

    update_last_seen(c);

    int pos = 4;
    struct mg_str topic;

    while ((pos = mg_mqtt_next_unsub(mm, &topic, pos)) > 0)
    {
        // Remove the subscription from the s_subs list
        for (auto sub = s_subs.begin(); sub != s_subs.end();)
        {
            if (c != (*sub)->c) // If the connection does not match with our client, continue
            {
                sub++;
                continue;
            }
            if (mg_strcmp((*sub)->topic, topic) != 0) // If the topic does not match, continue
                continue;
            ESP_LOGI("handle_mqtt_unsubscribe", "DELETE SUB %p [%.*s]", c->fd, (int)(*sub)->topic.len, (*sub)->topic.buf);

            // Decrement the count of the topic in the s_topics list
            s_topics[std::string((*sub)->topic.buf, (int)(*sub)->topic.len)].first--;
            // If the count is 0, remove the topic from the s_topics list
            if (s_topics[std::string((*sub)->topic.buf, (int)(*sub)->topic.len)].first <= 0)
            {
                s_topics.erase(std::string((*sub)->topic.buf, (int)(*sub)->topic.len));
                ESP_LOGI("handle_mqtt_subscribe", "TOPIC [%.*s] removed from s_topics\nTriggering UI_RELOAD", c->fd, (int)(*sub)->topic.len, (*sub)->topic.buf);
            }
            reload_flag = true; // Set the reload flag to true to trigger an UI reload

            sub = s_subs.erase(sub); // Remove the subscription from the s_subs list
        }
    }
    if (reload_flag)
    {
        xEventGroupSetBits(ui_event_group, UI_RELOAD_BIT); // if a reload flag was set, we will set the UI_RELOAD_BIT to trigger an UI reload
    }
}

void handle_mqtt_publish(struct mg_connection *c, struct mg_mqtt_message *mm)
{
    ESP_LOGI("handle_mqtt_publish", "PUBLISH");
    MG_INFO(("PUB %p [%.*s] -> [%.*s]", c->fd, (int)mm->data.len,
             mm->data.buf, (int)mm->topic.len, mm->topic.buf));
    update_last_seen(c);

    for (auto sub = s_subs.begin(); sub != s_subs.end(); sub++) // Iterate over all subscriptions
    {
        if (mg_match(mm->topic, (*sub)->topic, NULL)) // If the topic matches with the subscription, publish the message
        {

            ESP_LOGI("handle_mqtt_publish", "PUB %p [%.*s] -> [%.*s]", c->fd, (int)mm->data.len,
                     mm->data.buf, (int)mm->topic.len, mm->topic.buf);

            struct mg_mqtt_opts pub_opts;
            memset(&pub_opts, 0, sizeof(pub_opts));
            pub_opts.topic = mm->topic;
            pub_opts.message = mm->data;
            pub_opts.qos = 1, pub_opts.retain = false;

            mg_mqtt_pub((*sub)->c, &pub_opts);

            // Update the corresponding UI object
            topic_object_update(std::string((*sub)->topic.buf, (int)(*sub)->topic.len), std::string(mm->data.buf, (int)mm->data.len));
        }
    }
}

void handle_mqtt_pingreq(struct mg_connection *c)
{
    auto client = find_connection(c);

    if (client == NULL)
    {
        ESP_LOGW("handle_mqtt_pingreq", "CLIENT %p not found, sending disconnect", c->fd);
        // uint8_t response[] = {0, 0};
        // mg_mqtt_send_header(c, MQTT_CMD_DISCONNECT, 0, sizeof(response)); // Send acknowledgement
        // mg_send(c, response, sizeof(response));
        c->is_closing = 1;
        mg_mqtt_disconnect(c, NULL);
        return;
    }
    MG_INFO(("PINGREQ %p -> PINGRESP", c->fd));
    update_last_seen(c);

    // uint8_t response[] = {0, 0};
    // mg_mqtt_send_header(c, MQTT_CMD_PINGRESP, 0, sizeof(response)); // Send acknowledgement
    // mg_send(c, response, sizeof(response));
    mg_mqtt_pong(c);
}

void handle_mqtt_close(struct mg_connection *c)
{
    ESP_LOGI("handle_mqtt_close", "MG_EV_CLOSE %p", c->fd);

    // Iterate over all clients and remove the client from the s_clients list
    for (auto client = s_clients.begin(); client != s_clients.end();)
    {
        // If the connection does not match with our client, continue
        if (c != (*client)->c)
        {
            client++;
            continue;
        }
        ESP_LOGI("handle_mqtt_close", "CLIENT DELETE %p [%.*s]", (*client)->c->fd, (int)(*client)->cid.len, (*client)->cid.buf);
        // Remove the client from the s_clients list and set the client pointer to the next element returned by erase
        client = s_clients.erase(client);
    }

    // Iterate over all subscriptions and remove the subscription from the s_subs list
    for (auto sub = s_subs.begin(); sub != s_subs.end();)
    {
        if (c != (*sub)->c)
        {
            sub++;
            continue;
        }
        ESP_LOGI("handle_mqtt_close", "SUB DELETE %p [%.*s]", (*sub)->c->fd, (int)(*sub)->topic.len, (*sub)->topic.buf);

        s_topics[std::string((*sub)->topic.buf, (int)(*sub)->topic.len)].first--;
        if (s_topics[std::string((*sub)->topic.buf, (int)(*sub)->topic.len)].first <= 0)
        {
            s_topics.erase(std::string((*sub)->topic.buf, (int)(*sub)->topic.len));
            xEventGroupSetBits(ui_event_group, UI_RELOAD_BIT);
        }
        // Remove the subscription from the s_subs list and set the sub pointer to the next element returned by erase
        sub = s_subs.erase(sub);
    }
    // Iterate over all wills and remove the will from the s_wills list
    for (auto will = s_wills.begin(); will != s_wills.end();)
    {
        // If the connection does not match with our client, continue
        if (c != (*will)->c)
        {
            will++;
            continue;
        }
        // Iterate over all subscriptions and publish the will message of the disconnected client to all subscribers of these topics
        for (auto sub = s_subs.begin(); sub != s_subs.end(); sub++)
        {
            // If the topic does not match, continue
            if (mg_strcmp((*will)->topic, (*sub)->topic) != 0)
                continue;

            ESP_LOGI("handle_mqtt_close", "WILL PUB %p [%.*s] [%.*s] %d %d",
                     (*will)->c->fd, (int)(*will)->topic.len, (*will)->topic.buf,
                     (int)(*will)->payload.len, (*will)->payload.buf, (*will)->qos, (*will)->retain);

            mg_mqtt_opts pub_opts;
            memset(&pub_opts, 0, sizeof(pub_opts));
            pub_opts.topic = (*will)->topic;
            pub_opts.message = (*will)->payload;
            pub_opts.qos = (*will)->qos;
            pub_opts.retain = (*will)->retain;

            mg_mqtt_pub((*sub)->c, &pub_opts);
        }
        ESP_LOGI("handle_mqtt_close", "WILL DELETE %p [%.*s] [%.*s] %d %d",
                 (*will)->c->fd, (int)(*will)->topic.len, (*will)->topic.buf,
                 (int)(*will)->payload.len, (*will)->payload.buf, (*will)->qos, (*will)->retain);
        // Remove the will from the s_wills list and set the will pointer to the next element returned by erase
        will = s_wills.erase(will);
    }
    xEventGroupSetBits(ui_event_group, UI_STATUSBAR_UPDATE_BIT);
}

static void fn(struct mg_connection *c, int ev, void *ev_data)
{

    std::lock_guard<std::recursive_mutex> lock(mqtt_server_mutex);

    if (ev == MG_EV_MQTT_CMD)
    {

        mg_mqtt_message *mm = (struct mg_mqtt_message *)ev_data;
        switch (mm->cmd)
        {
        case MQTT_CMD_CONNECT:
            handle_mqtt_connect(c, mm);
            break;
        case MQTT_CMD_SUBSCRIBE:
            handle_mqtt_subscribe(c, mm);
            break;
        case MQTT_CMD_UNSUBSCRIBE:
            handle_mqtt_unsubscribe(c, mm);
            break;
        case MQTT_CMD_PUBLISH:
            handle_mqtt_publish(c, mm);
            break;
        case MQTT_CMD_PINGREQ:
            handle_mqtt_pingreq(c);
            break;
        default:
            break;
        }

        ESP_LOGI("fn done", "\nClient Vector Count: %d\nSub Vector Count: %d\nWill Vector Count: %d", s_clients.size(), s_subs.size(), s_wills.size());
        helper_functions::printMemory();
        std::cout << "---------------------------\n";

        std::cout << "\n\n\n\n";
    }
    else if (ev == MG_EV_CLOSE)
    {
        handle_mqtt_close(c);
        ESP_LOGI("fn done", "\nClient Vector Count: %d\nSub Vector Count: %d\nWill Vector Count: %d", s_clients.size(), s_subs.size(), s_wills.size());
        helper_functions::printMemory();
        std::cout << "---------------------------\n";
        // ESP_ERROR_CHECK(heap_trace_stop());
        // heap_trace_dump();
        std::cout << "\n\n\n\n";
    }
}

void mqtt_server(void *pvParameters)
{

    constexpr const char * const TAG = "mqtt_server";
    ESP_LOGI(TAG, "Start");
    struct mg_mgr mgr;
    mg_log_set(3); // Set to log level to LL_ERROR
    mg_mgr_init(&mgr);

    // ESP_ERROR_CHECK(heap_trace_init_standalone(trace_record, NUM_RECORDS));

    mg_mqtt_listen(&mgr, s_listen_on, fn, NULL); // Create MQTT listener

    while (1)
    {
        mg_mgr_poll(&mgr, 0);
        vTaskDelay(pdTICKS_TO_MS(30));

        static int counter = 0;
        counter += pdTICKS_TO_MS(30);

        if (counter >= CHECK_INTERVAL_IN_SECONDS * 1000)
        {
            check_timeouts();
            counter = 0;
        }
    }
    // ESP_ERROR_CHECK(heap_trace_stop());
    // heap_trace_dump();

    while (1)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "finish");
    mg_mgr_free(&mgr);
}
