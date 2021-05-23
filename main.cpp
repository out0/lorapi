#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <queue>
#include <mutex>

#include "sx1276.h"
#include "lw.h"
#include "str2hex.h"
#include "base64.h"
#include "cmd_opts.h"
#include "mqtt_bridge.h"

using namespace std;

const uint8_t nwkskey[16] = {0xA3, 0xFB, 0xB7, 0x18, 0xA9, 0xCB, 0x4E, 0xB1, 0x2A, 0x3C, 0xEE, 0x54, 0xBA, 0xF8, 0xF2, 0xD9};
const uint8_t appskey[16] = {0xAE, 0x41, 0xF3, 0x1B, 0x03, 0x3C, 0x53, 0x0D, 0x94, 0x22, 0xA8, 0xE0, 0x89, 0xD1, 0xB4, 0x79};

#define BRIDGE_SENSOR_TOPIC "/sensors/0"
#define BRIDGE_ACTUATOR_TOPIC "/actuators/0"

void die(const char *s)
{
    perror(s);
    exit(1);
}
lw_key_grp_t *setSessionKeys(const uint8_t *nwksKey, const uint8_t *appsKey);
lw_frame_t *decryptFrame(lw_key_grp_t *keys, char *data);

bool isConnectedToMQTT = false;

void onConnectToMQTT(char *host, int port)
{
    std::cout << "connected to the Broker at " << host << ":" << port << std::endl;
    isConnectedToMQTT = true;
}

sx1276 *radio;
lw_key_grp_t *keys;
queue<char *> *cache;
queue<char *> *downlinkCache;

mutex resourcesAccess;

void execRawTesting()
{
    cout << "Executing in RAW-TEST mode" << endl;
    cout << "This node will send back any LoRa phy-payload it receives" << endl;
    while (1)
    {
        char *data = radio->receivePacket();
        if (data != NULL)
        {
            int n = strlen(data);
            printf("data received: %s - len %d\n", data, n);
            uint8_t *bufferOrig = new uint8_t[512];
            uint8_t len = str2hex(data, bufferOrig, 450);

            printf("sending back %d bytes..", len);
            switch (radio->sendPacket(bufferOrig, len, 0, 1000, 2))
            {
            case -1:
                printf("failed to switch do LoraMode\n");
                break;
            case -2:
                printf("timeout\n");
                break;
            default:
                delay(100);
                printf("ok\n");
                break;
            }

            delete data;
        }
    }
}
void execPingPongMode()
{
    cout << "Executing in PING-PONG mode" << endl;
    cout << "This node will send back any LoRaWAN payload it receives" << endl;

    while (1)
    {
        char *data = radio->receivePacket();
        if (data != NULL)
        {
            printf("data received: %s\n", data);

            lw_frame_t *frame = NULL;
            frame = decryptFrame(keys, data);
            lw_log(frame, frame->buf, frame->len);
            cout << "\n\n";
            delete data;

            if (frame != NULL)
                printf("pkt decrypted\n");
            else
            {
                printf("invalid packet");
                continue;
            }

            frame->mhdr.bits.mtype = 3;
            for (int i = 0; i < 8; i++)
                frame->deveui[i] = radio->gatewayID[i];

            frame->node->dlport = 1;
            frame->pl.mac.fcnt++;

            //setting frame header to unconfirmed uplink
            frame->mhdr.data = 0x60;
            //
            frame->pl.mac.fctrl.dl.ack = 0;
            frame->pl.mac.fctrl.dl.adr = 0;
            frame->pl.mac.fctrl.dl.fpending = 0;
            frame->pl.mac.fctrl.dl.foptslen = 0;

            frame->pl.mac.fport = 1;
            //frame->pl.mac.devaddr.data = 0x26031433;

            for (int i = 0; i < 8; i++)
                frame->deveui[i] = radio->gatewayID[i];

            frame->node->dlrxwin = CLASS_A_RX2;

            /*for (int i = 0; i < 16; i++)
                frame->node->appskey[i] = appskey[i];
            for (int i = 0; i < 16; i++)
                frame->node->nwkskey[i] = appskey[i];*/

            uint8_t *buffer = new uint8_t[512];
            int len = 0;
            lw_pack(frame, buffer, &len);
            delete frame;
            cout << "size: " << len << endl;

            //char *in = "YDMUAyYAAAABP51PaUYHVahgB0t41/3R6hY=";
            //int len = strlen(in);
            //b64_to_bin(in, strlen(in), buffer, 512) ;

            char *newData = new char[512];
            bin_to_b64(buffer, len, newData, 512);
            cout << "new frame: " << newData << endl;

            frame = decryptFrame(keys, newData);
            lw_log(frame, frame->buf, frame->len);
            cout << "\n\n";

            switch (radio->sendPacket(buffer, len, 0, 1000, 2))
            {
            case -1:
                printf("failed to switch do LoraMode\n");
                break;
            case -2:
                printf("timeout\n");
                break;
            default:
                printf("ok\n");
                delay(10);
                break;
            }
            switch (radio->sendPacket(buffer, len, 1900, 1000, 2))
            {
            case -1:
                printf("failed to switch do LoraMode\n");
                break;
            case -2:
                printf("timeout\n");
                break;
            default:
                printf("ok\n");
                delay(10);
                break;
            }
        }
    }
}
void onMQTTMessageReceived(char *message, int len)
{
    resourcesAccess.lock();
    downlinkCache->push(message);
    cout << "data received from MQTT. Queueing to transmit back to the device later" << endl;
    resourcesAccess.unlock();
}
void *execBridgeMode_Lora(void *ptr)
{
    while (1)
    {
        char *data = radio->receivePacket();
        if (data == NULL)
            continue;

        printf("LORA data received: %s. Analyzing...", data);

        lw_frame_t *frame = NULL;

        frame = decryptFrame(keys, data);
        delete data;

        if (frame != NULL)
            printf("ok\n");
        else
        {
            printf("invalid packet");
            continue;
        }

        //lw_log(frame, frame->buf, frame->len);
        char *mqttPayload = new char[frame->pl.mac.flen + 1];
        for (int i = 0; i < frame->pl.mac.flen; i++)
        {
            mqttPayload[i] = frame->pl.mac.fpl[i];
        }
        mqttPayload[frame->pl.mac.flen] = 0x0;

        //do we have anything to report back?
        resourcesAccess.lock();
        if (!downlinkCache->empty())
        {
            char *downlinkPayload = downlinkCache->front();
           

            frame->mhdr.bits.mtype = 3;
            for (int i = 0; i < 8; i++)
                frame->deveui[i] = radio->gatewayID[i];

            frame->node->dlport = 1;
            frame->pl.mac.fcnt++;

            //setting frame header to unconfirmed uplink
            frame->mhdr.data = 0x60;
            //
            frame->pl.mac.fctrl.dl.ack = 0;
            frame->pl.mac.fctrl.dl.adr = 0;
            frame->pl.mac.fctrl.dl.fpending = 0;
            frame->pl.mac.fctrl.dl.foptslen = 0;

            frame->pl.mac.fport = 1;
            //frame->pl.mac.devaddr.data = 0x26031433;

            for (int i = 0; i < 8; i++)
                frame->deveui[i] = radio->gatewayID[i];

            frame->node->dlrxwin = CLASS_A_RX2;

            /*for (int i = 0; i < 16; i++)
                frame->node->appskey[i] = appskey[i];
            for (int i = 0; i < 16; i++)
                frame->node->nwkskey[i] = appskey[i];*/

            delete downlinkPayload;
            cout << "all set, packing" << endl;
                
            uint8_t *buffer = new uint8_t[512];
            int len = 0;
            lw_pack(frame, buffer, &len);
            delete frame;

            cout << "Sending MQTT actuator data (window 1)..." << endl;
            switch (radio->sendPacket(buffer, len, 300, 1000, 1))
            {
            case -1:
                printf("failed to switch do LoraMode\n");
                break;
            case -2:
                printf("timeout\n");
                break;
            default:
                printf("ok\n");
                break;
            }
            cout << "Sending MQTT actuator data (window 2)..." << endl;
            switch (radio->sendPacket(buffer, len, 1400, 1000, 2))
            {
            case -1:
                printf("failed to switch do LoraMode\n");
                break;
            case -2:
                printf("timeout\n");
                break;
            default:
                printf("ok\n");
                downlinkCache->pop();
                break;
            }

            delete buffer;
        }

        cout << "Publishing new data to MQTT" << endl;
        cache->push(mqttPayload);
        resourcesAccess.unlock();
    }
}
void *execBridgeMode_MQTT(void *ptr)
{
    mosqpp::lib_init();
    MQTTBridge *brigde = new MQTTBridge("10.0.0.4", 1883, BRIDGE_SENSOR_TOPIC, BRIDGE_ACTUATOR_TOPIC, onConnectToMQTT, onMQTTMessageReceived);

    while (1)
    {
        if (isConnectedToMQTT)
        {
            resourcesAccess.lock();
            if (!cache->empty())
            {
                char *mqttPayload = cache->front();
                brigde->publishToMQTT(mqttPayload);
                cache->pop();
                delete mqttPayload;
            }
            resourcesAccess.unlock();
        }

        brigde->loop();
        delay(50);
    }

    mosqpp::lib_cleanup();
}
void execBridgeMode()
{
    cout << "Executing in BRIDGE mode" << endl;
    cout << "This node will brigde MQTT and LoRA" << endl;

    pthread_t mqtt, lora;
    cache = new queue<char *>();
    downlinkCache = new queue<char *>();

    int i = pthread_create(&lora, NULL, execBridgeMode_Lora, NULL);
    int j = pthread_create(&mqtt, NULL, execBridgeMode_MQTT, NULL);

    pthread_join(lora, NULL);
    pthread_join(mqtt, NULL);
}

int main(int argc, char **argv)
{

    tArgs *options = getCommandOptions(argc, argv);

    keys = setSessionKeys(nwkskey, appskey);
    radio = new sx1276(0, 6, 0, 2,
                       options->rxFreq > 0 ? options->rxFreq : 915000000,
                       options->txFreq > 0 ? options->txFreq : 923300000,
                       options->sf > 0 ? options->sf : 10,
                       options->bw > 0 ? options->bw : 125,
                       options->cr > 0 ? options->cr : 4);

    uint32_t lasttime = 0;
    struct timeval nowtime;

    switch (options->mode)
    {
    case EXEC_MODE_BRIDGE:
        execBridgeMode();
        break;
    case EXEC_MODE_PINGPONG:
        execPingPongMode();
        break;
    case EXEC_MODE_RAWTEST:
        execRawTesting();
        break;
    }
    return (0);
}

lw_key_grp_t *setSessionKeys(const uint8_t *nwksKey, const uint8_t *appsKey)
{

    lw_key_grp_t *keys = new lw_key_grp_t();
    keys->nwkskey = new uint8_t[16];
    keys->flag.bits.nwkskey = 1;
    keys->appskey = new uint8_t[16];
    keys->flag.bits.appskey = 1;
    keys->appkey = new uint8_t[16];
    keys->flag.bits.appkey = 0;

    for (int i = 0; i < 16; i++)
    {
        keys->nwkskey[i] = nwkskey[i];
        keys->appskey[i] = appsKey[i];
        keys->appkey[i] = 0x0;
    }

    return keys;
}
lw_frame_t *decryptFrame(lw_key_grp_t *keys, char *data)
{
    lw_frame_t *frame = new lw_frame_t();
    uint8_t *buf = new uint8_t[256];

    lw_init(US915);
    lw_set_key(keys);

    int hlen = str2hex(data, buf, 256);

    if ((hlen > 256) || (hlen <= 0))
    {
        cout << "wrong packet" << endl;
        return NULL;
    }

    int ret = lw_parse(frame, buf, hlen);

    if (ret == LW_OK)
    {
        return frame;
    }
    else
    {
        cout << "DATA MESSAGE PARSE error(" << ret << ")" << endl;
        return NULL;
    }
}
