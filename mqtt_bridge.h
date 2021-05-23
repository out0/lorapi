#ifndef _MQTT_BRIGDE_H
#define _MQTT_BRIGDE_H

#include "mosquittopp.h"
#include "sx1276.h"
#include "lw.h"

class MQTTBridge : public mosqpp::mosquittopp {
    private:
        char *host;
        int port;
        int message_id;
        int subscribeMessageID;
        char *upstreamTopic;
        char *downstreamSubscribeTopic;
        void (*onConnectPtr)(char *, int);
        void (*onMessageReceivedPtr)(char *, int len);
        virtual void on_connect(int rc);
        virtual void on_message(const struct mosquitto_message *message);
    public:
    	MQTTBridge(char *host, int port, char *upstreamTopic, char *downstreamSubscribeTopic, void (*onConnectHandler)(char *, int), void (*onMessageReceived)(char *, int len));
		~MQTTBridge();
		void publishToMQTT (char *payload);
};

#endif