#pragma once
#include "mqtt_client.h"
// #include <vector>
#include <mutex>
#include <string>
#include <iostream>
#include <vector>
#include "driver/gpio.h"


static const int MQTT_STATUS_LED = GPIO_NUM_17;
static const int LED_PIN = GPIO_NUM_23;

// void ClientHandler(std::string command, std::string client_id, std::string ip, std::string topic);

struct Client
{
public:
    std::string client_id;
    std::vector<std::string> topics;
    Client(const std::string id) : client_id(id), topics({}) {}

    void print()
    {
        std::cout << "Client ID: " << client_id << std::endl;
        for (auto topic : topics)
        {
            std::cout << "\tTopic: " << topic << std::endl;
        }
    }
};

// Initialize static members

void initialise_mdns(void);

void init_mongoose();

void send_mqtt_message(std::string topic, std::string message);