#pragma once

#include "mongoose/mongoose.h"
#include <vector>
#include "mutex"
#include "memory"
#include <map>
#include "chrono"
#include "wifi/wifi_dpp.h"

void mqtt_server(void *params);

// A client
struct mqtt_client
{
  mg_connection *c;
  mg_str cid;
  std::chrono::steady_clock::time_point last_seen;

  ~mqtt_client()
  {
    if (cid.buf)
    {
      free(cid.buf);
    }
  }
};

// a subscription of a client
struct mqtt_sub
{
  mg_connection *c;
  mg_str topic;
  uint8_t qos;
  ~mqtt_sub()
  {
    if (topic.buf)
    {
      free(topic.buf);
    }
  }
};

// a will message of a client to be published when the client disconnects
struct mqtt_will
{
  mg_connection *c;
  mg_str topic;
  mg_str payload;
  uint8_t qos;
  uint8_t retain;
  ~mqtt_will()
  {
    if (topic.buf)
    {
      free(topic.buf);
    }
    if (payload.buf)
    {
      free(payload.buf);
    }
  }
};

// represents all clients hold by the broker
// access only after acquiring mqtt_server_mutex!
extern std::vector<std::shared_ptr<mqtt_client>> s_clients;

// represents aall subscriptions hold by the broker
// access only after acquiring mqtt_server_mutex!
extern std::vector<std::shared_ptr<mqtt_sub>> s_subs;

// represents all will messages hold by the broker
// access only after acquiring mqtt_server_mutex!
extern std::vector<std::shared_ptr<mqtt_will>> s_wills;

// represents all topics hold by the broker each persisting of a topic, the number of subscribers and the last message
// access only after acquiring mqtt_server_mutex!
extern std::map<std::string, std::pair<int, std::string>> s_topics;