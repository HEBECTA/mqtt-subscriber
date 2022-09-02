#include "mqtt_sub.h"
//#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#include "uci_data.h"

void subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos){

        syslog(LOG_NOTICE, "Subscribed (mid: %d): QoS %d", mid, granted_qos[0]);

        for(int i=1; i<qos_count; i++)
                syslog(LOG_NOTICE, ", %d", granted_qos[i]);
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message){

        struct message *msg_info = (struct message *) obj;

        //bool match = 0;
	//printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

        if ( strcpy(msg_info->topic, message->topic) == 0 ){

                syslog(LOG_ERR, "MQTT, failed to copy topic name\n");
                return;
        }

        if ( strcpy(msg_info->msg, (char*) message->payload) == 0 ){

                syslog(LOG_ERR, "MQTT, failed to copy received message\n");
                return;
        }

        msg_info->received = 1;

        /*
	mosquitto_topic_matches_sub("router/1122393453/uptime", message->topic, &match);
	if (match) {
		printf("got message for uptime topic\n");
	}*/
}

void log_callback(struct mosquitto *mosq, void *obj, int level, const char *str){

        syslog(LOG_NOTICE, "Log MQtt: %s\n", str);
}

struct mosquitto *mqtt_init_subscribe(int topics_nmb, struct topic *topics, struct message *msg_info, struct arguments options){

        int rc = 0;

        mosquitto_lib_init();

        // client id ????
        struct mosquitto *mosq = mosquitto_new(NULL, true, (void *)msg_info);

        if ( mosq == NULL ){
                syslog(LOG_ERR, "MQTT: failed initialize mosquitto\n");
                return NULL;
        }

        mosquitto_subscribe_callback_set(mosq, subscribe_callback);
        mosquitto_message_callback_set(mosq, message_callback);
        mosquitto_log_callback_set(mosq, log_callback);

        rc = mosquitto_username_pw_set(mosq, "tester", "tester");
        if ( rc != MOSQ_ERR_SUCCESS ){

                syslog(LOG_ERR, "MQTT: failed to set username and password\n");
                return NULL;
        }

        rc = mosquitto_tls_set(mosq, "/etc/certificates/cert.cert.pem", NULL,
        NULL, NULL, NULL);
        if ( rc != MOSQ_ERR_SUCCESS ){

                syslog(LOG_ERR, "MQTT: failed to set tls certificates\n");
                return NULL;
        }


        for (int i = 0; i < RECONNECT_NUMBER; ++i){

                rc = mosquitto_connect(mosq, "192.168.1.1", 1883, 60);
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
                return NULL;
        }

        for (int i = 0; i < topics_nmb; ++i ){

                rc = mosquitto_subscribe(mosq, NULL, topics[i].name, QoS0);
                if ( rc )
                        syslog(LOG_ERR, "MQTT: Failed to subscribe topic = %s\n", topics[i].name);
        }

        return mosq;
}