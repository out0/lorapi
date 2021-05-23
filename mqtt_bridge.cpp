#include "mqtt_bridge.h"
#include <string.h>

MQTTBridge::MQTTBridge(char *host, int port, char *upstreamTopic, char *downstreamSubscribeTopic, void (*onConnectHandler)(char *, int), void (*onMessageReceived)(char *, int len))
{
    this->message_id = 0;
    this->host = strdup(host);
    this->port = port;
    this->username_pw_set("user1", "abc123");
    this->onConnectPtr = onConnectHandler;
    this->onMessageReceivedPtr = onMessageReceived;
    this->upstreamTopic = upstreamTopic;
    this->downstreamSubscribeTopic = downstreamSubscribeTopic;
    connect(host, port, 60);
    subscribeMessageID = 1000;
    //subscribeMessageID++;
}
MQTTBridge::~MQTTBridge() {
    delete this->host;
    this->onConnectPtr = NULL;
    this->onMessageReceivedPtr = NULL;
    disconnect();
}
void MQTTBridge::publishToMQTT(char *payload)
{
    message_id++;
    publish(&message_id, upstreamTopic, strlen(payload), payload, 1);
}

void MQTTBridge::on_connect(int rc)
{
    this->subscribe(&subscribeMessageID, downstreamSubscribeTopic, 1);
    this->onConnectPtr(host, port);
}
void MQTTBridge::on_message(const struct mosquitto_message *message)
{
    char *msg = (char *)malloc(message->payloadlen);
    memcpy(msg, message->payload, message->payloadlen);
    this->onMessageReceivedPtr(msg, message->payloadlen);
}