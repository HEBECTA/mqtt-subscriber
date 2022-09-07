#include "mqtt_sub.h"
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>

#include "uci_data.h"

void free_message(struct message msg_info){

        printf("before clean msg\n");

        if ( msg_info.topic != NULL ){

                free(msg_info.topic);
                msg_info.topic = NULL;
        }

        if ( msg_info.msg != NULL ){

                free(msg_info.msg);
                msg_info.msg = NULL;
        }

        msg_info.received = 0;
        msg_info.topic_len = 0;
        msg_info.msg_len = 0;

        printf("after clean msg\n");
}

void subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos){

        syslog(LOG_NOTICE, "Subscribed (mid: %d): QoS %d", mid, granted_qos[0]);

        for(int i=1; i<qos_count; i++)
                syslog(LOG_NOTICE, ", %d", granted_qos[i]);
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message){

        struct message *msg_info = (struct message *) obj;

        //memset(msg_info->topic, 0, msg_info->topic_len);
        //memset(msg_info->msg, 0, msg_info->msg_len);

        // increase buff size if new message is larger than previous ones
        int local_topic_len = strlen(message->topic);
        int local_message_len = strlen((char*) message->payload);

        if ( msg_info->topic_len < local_topic_len ){

                if ( msg_info->topic  != NULL ){

                        free(msg_info->topic);
                }

                msg_info->topic = (char *) malloc(sizeof(char) * (local_topic_len + 1));

                if ( msg_info->topic == NULL ){

                        syslog(LOG_ERR, "MQTT, failed to copy topic name\n");
                        msg_info->received = 0;
                        return;
                }

                msg_info->topic_len = local_topic_len;
        }

        strcpy(msg_info->topic, message->topic);

        if ( msg_info->msg_len < local_message_len ){

                if ( msg_info->msg  != NULL ){

                        free(msg_info->msg);
                }

                msg_info->msg = (char *) malloc(sizeof(char) * (local_message_len + 1));

                if ( msg_info->msg == NULL ){

                        free(msg_info->topic);
                        msg_info->topic_len = 0;
                        syslog(LOG_ERR, "MQTT, failed to copy received message\n");
                        msg_info->received = 0;
                        return;
                }

                msg_info->msg_len = local_message_len;
        }

        strcpy(msg_info->msg, (char*) message->payload);

        msg_info->received = 1;

        /*
	mosquitto_topic_matches_sub("router/1122393453/uptime", message->topic, &match);
	if (match) {
		printf("got message for uptime topic\n");
	}*/
}
/*
void log_callback(struct mosquitto *mosq, void *obj, int level, const char *str){

        syslog(LOG_NOTICE, "Log MQtt: %s\n", str);
}
*/
struct mosquitto *mqtt_init_subscribe(int topics_nmb, struct topic *topics, struct message *msg_info, struct arguments options){

        int rc = 0;

        // client id ????
        struct mosquitto *mosq = mosquitto_new(NULL, true, (void *)msg_info);

        if ( mosq == NULL ){
                syslog(LOG_ERR, "MQTT: failed initialize mosquitto\n");
                return NULL;
        }

        mosquitto_subscribe_callback_set(mosq, subscribe_callback);
        mosquitto_message_callback_set(mosq, message_callback);
        //mosquitto_log_callback_set(mosq, log_callback);

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
                        break;
                }

                topics = topics->next_topic;
        }

        if ( rc )
                goto EXIT_MQTT_INIT_ERROR;

        return mosq;

EXIT_MQTT_INIT_ERROR:

        mosquitto_destroy(mosq);

        return NULL;
}