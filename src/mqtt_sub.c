#include "mqtt_sub.h"
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>

#include "uci_data.h"

int init_message_buffer(struct message *msg_info){

        msg_info->topic = (char *) malloc(sizeof(char) * (MQTT_TOPIC_NAME_SIZE + 1));

        if ( msg_info->topic == NULL )
                return ENOMEM;

        msg_info->msg = (char *) malloc(sizeof(char) * (MQTT_MSG_SIZE + 1));

        if ( msg_info->msg == NULL ){

                free(msg_info->topic);
                return ENOMEM;
        }

        return 0;
}

void free__message_buffer(struct message msg_info){

        if ( msg_info.topic != NULL )
                free(msg_info.topic);
        
        if ( msg_info.msg != NULL )
                free(msg_info.msg);
        
        msg_info.received = 0;
}

void subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos){

        syslog(LOG_NOTICE, "MQTT: Subscribed (mid: %d): QoS %d", mid, granted_qos[0]);

        for(int i=1; i<qos_count; i++)
                syslog(LOG_NOTICE, ", %d", granted_qos[i]);
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message){

        struct message *msg_info = (struct message *) obj;

        int received_topic_len = strlen(message->topic);
        int received_message_len = strlen((char*) message->payload);

        if ( MQTT_TOPIC_NAME_SIZE < received_topic_len || MQTT_MSG_SIZE < received_message_len ){

               syslog(LOG_ERR, "MQTT: Received message is to large for buffer");
               return;
        }

        strcpy(msg_info->topic, message->topic);
        strcpy(msg_info->msg, (char*) message->payload);

        msg_info->received = 1;
}

struct mosquitto *mqtt_init_subscribe(int topics_nmb, struct topic *topics, struct message *msg_info, struct arguments options){

        mosquitto_lib_init();

        int rc = 0;

        struct mosquitto *mosq = mosquitto_new(NULL, true, (void *)msg_info);

        if ( mosq == NULL ){
                syslog(LOG_ERR, "MQTT: failed initialize mosquitto\n");
                return NULL;
        }

        mosquitto_subscribe_callback_set(mosq, subscribe_callback);
        mosquitto_message_callback_set(mosq, message_callback);

        if ( strcmp(options.username, "-") && strcmp(options.password, "-") ){

                rc = mosquitto_username_pw_set(mosq, options.username, options.password);
                if ( rc != MOSQ_ERR_SUCCESS ){

                        syslog(LOG_ERR, "MQTT: failed to set username and password\n");
                        goto EXIT_MQTT_INIT_ERROR;
                }
        }

        if ( strcmp(options.cert_file_path, "-") ){

                rc = mosquitto_tls_set(mosq, options.cert_file_path, NULL,
                NULL, NULL, NULL);
                if ( rc != MOSQ_ERR_SUCCESS ){

                        syslog(LOG_ERR, "MQTT: failed to set tls certificates\n");
                        goto EXIT_MQTT_INIT_ERROR;
                }
        }


        for (int i = 0; i < RECONNECT_NUMBER; ++i){

                rc = mosquitto_connect(mosq, options.host, options.port, 60);
                if ( rc ){
                        syslog(LOG_ERR, "MQTT attempt: %d; Failed connect to broker\n", i + 1 );
                        sleep(RECONNECT_WAIT_TIME);
                }

                else{
                        syslog(LOG_NOTICE, "MQTT: Successfuly connected to broker\n");
                        break;
                }
        }

        if ( rc ){
                syslog(LOG_ERR, "MQTT: Finished all connection attemtps. Failed connect to broker\n");
                goto EXIT_MQTT_INIT_ERROR;
        }

        while ( topics != NULL ){

                rc = mosquitto_subscribe(mosq, NULL, topics->name, QoS0);
                if ( rc ){
                        syslog(LOG_ERR, "MQTT: Failed to subscribe topic = %s\n", topics->name);
                        goto EXIT_MQTT_INIT_ERROR;
                }

                topics = topics->next_topic;
        }

        return mosq;

EXIT_MQTT_INIT_ERROR:

        mosquitto_destroy(mosq);

        return NULL;
}

void mqtt_free(struct mosquitto *mosq){

        if ( mosq != NULL )
                mosquitto_destroy(mosq);

        mosquitto_lib_cleanup();
}