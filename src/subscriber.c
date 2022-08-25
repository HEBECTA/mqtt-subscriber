#include <stdio.h>
#include <mosquitto.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <uci.h>
#include "uci_data.h"

void sub_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos){

        int i;

        printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
        for(i=1; i<qos_count; i++){
                printf(", %d", granted_qos[i]);
        }
        printf("\n");
}

void msg_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message){

        bool match = 0;
	printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

	mosquitto_topic_matches_sub("router/1122393453/uptime", message->topic, &match);
	if (match) {
		printf("got message for uptime topic\n");
	}
}
/*
void conncet_callback(struct mosquitto *mosq, void *obj, int rc){

        printf("connect callback, rc=%d\n", result);

        int i;
        if(!result){
                     
                mosquitto_subscribe(mosq, NULL, "router/1122393453/uptime", 2);
        }else{
                fprintf(stderr, "Connect failed\n");
        }
}
*/

int main(int argc, char *argv[]){

        int rc = EXIT_SUCCESS;

        //struct arguments args = {"-", 0};

        openlog(NULL, LOG_PID, LOG_USER);

        //rc = args_parse(&args);

        if (rc){
                syslog(LOG_ERR, "Failed to parse program arguments\n");
                goto EXIT_PROGRAM;
        }

        struct uci_context *ctx = NULL;
        struct uci_package *package = NULL;

        ctx = init_uci("subscriber", &package);
        if (ctx == NULL) {
		syslog(LOG_ERR, "unable to init uci\n");
		rc = EPERM;
                goto EXIT_PROGRAM;
	}

        int topics_n;
        char **topics;

        rc = scan_topics(ctx, package, &topics, &topics_n);
        if ( rc ){
                syslog(LOG_ERR, "unable to scan topics from config file\n");
                goto EXIT_PROGRAM;
        }

        for (int i = 0; i < topics_n; ++i){

                printf("topic = %s\n", topics[i]);
        }

        int events_n;
        struct events *events = NULL;

        scan_events(ctx, package, &events, &events_n, topics, topics_n);


        free_topics(topics);
        free_events(events);

        
/*
        
        sct = NULL;
        sct = uci_lookup_section(ctx, package, "event");
        if ( sct == NULL ){

                printf("uci_lookup_section: no section event found\n");
                return -1;
        }

        struct uci_element *l = NULL;
        l = uci_lookup_list(&package->sections, "event");
        if (l == NULL ){

                printf("uci_lookup_list: no section event found\n");
                return -1;
        }

        if ( l->type == UCI_TYPE_LIST ){

                printf("list of section events \n");
                return -1;
        }

        else if ( l->type == UCI_TYPE_STRING ){

                printf("only one section event found, name = %s\n", l->name);
                return -1;
        }*/
/*
        if ( UCI_OK != uci_lookup_ptr(ctx, &p, "subscriber.subscriber_info.topic", true)){
                printf("uci_lookup_ptr error\n");
                return -1;
        }

        printf("%s\n", p.o->v.string);
*/
        //uci_free_context(ctx);

        //printf("uci map n_sections = %u\n", uci_map.n_sections);

        printf("package element name = %s\n", package->e.name);

        return rc;

        mosquitto_lib_init();

        // client id ????
        struct mosquitto *mosq = mosquitto_new(NULL, true, NULL);

        if ( mosq == NULL ){
                syslog(LOG_ERR, "Failed initialize mosquitto\n");
                rc = EPERM;
                goto EXIT_PROGRAM;
        }

        //rc = mosquitto_connect_callback_set(mosq, conncet_callback);
        mosquitto_message_callback_set(mosq, msg_callback);
        mosquitto_subscribe_callback_set(mosq, sub_callback);

        //mosquitto_will_set();

        //rc = mosquitto_username_pw_set(mosq, "username", "password");

        //rc = mosquitto_tls_set(mosq, );

        rc = mosquitto_connect(mosq, "localhost", 1883, 60);

        rc = mosquitto_subscribe(mosq, NULL, "router/1122393453/uptime", 0);

        rc = mosquitto_loop_forever(mosq, -1, 1);

EXIT_PROGRAM:

        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();

        return rc;
}