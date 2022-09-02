#ifndef MQTT_SUB_H
#define MQTT_SUB_H

#include <mosquitto.h>
#include "topic_event.h"
#include "args.h"

#define RECONNECT_WAIT_TIME 5
#define RECONNECT_NUMBER 5
#define MAX_TOPIC_NAME_LEN 100
#define MESSSAGE_BUFF_SIZE 250

#define QoS0 0
#define QoS1 1
#define QoS2 2


struct message{

        char topic[MAX_TOPIC_NAME_LEN];
        char msg[MESSSAGE_BUFF_SIZE];
        int received;
};

void subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);

void conncet_callback(struct mosquitto *mosq, void *obj, int rc);

void log_callback(struct mosquitto *mosq, void *obj, int level, const char *str);

struct mosquitto *mqtt_init_subscribe(int topics_nmb, struct topic *topics, struct message *msg_info, struct arguments options);

#endif