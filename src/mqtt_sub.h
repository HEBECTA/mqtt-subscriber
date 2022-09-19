#ifndef MQTT_SUB_H
#define MQTT_SUB_H

struct topic;

#include <mosquitto.h>
#include "topic_event.h"
#include "args.h"

#define RECONNECT_WAIT_TIME 5
#define RECONNECT_NUMBER 5

#define QoS0 0
#define QoS1 1
#define QoS2 2


#define MQTT_TOPIC_NAME_SIZE 1024
#define MQTT_MSG_SIZE 2048

struct message{

        char *topic;
        char *msg;
        int received;
};

int init_message_buffer(struct message *msg_info);

void subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);

struct mosquitto *mqtt_init_subscribe(int topics_nmb, struct topic *topics, struct message *msg_info, struct arguments options);

void free__message_buffer(struct message msg_ingo);

void mqtt_free(struct mosquitto *mosq);

#endif