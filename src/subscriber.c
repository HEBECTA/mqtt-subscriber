#include <stdio.h>
#include <mosquitto.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <uci.h>
#include <time.h>


#include "uci_data.h"
#include "file_io.h"
#include "serial_ubus.h"

#define UBUS_CONNECT_MAX_FAILS 3

int prog_run;

struct link{
        //char topic[100];
        char msg[250];
        int received;
        Topic topic;
};

void subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos){

        int i;

        printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
        for(i=1; i<qos_count; i++){
                printf(", %d", granted_qos[i]);
        }
        printf("\n");
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message){

        struct link *l = (struct link *) obj;

        bool match = 0;
	printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

        Topic topic;
        time_t date = time(0); 
        set_topic(&topic, message->topic, (char*) message->payload, 0, date);

        l->topic = topic;
        l->received = 1;

	mosquitto_topic_matches_sub("router/1122393453/uptime", message->topic, &match);
	if (match) {
		printf("got message for uptime topic\n");
	}
}

void conncet_callback(struct mosquitto *mosq, void *obj, int rc){

        printf("connect callback, rc=%d\n", rc);

        int i;
        if(!rc){
                printf("successful connection\n");
                //mosquitto_subscribe(mosq, NULL, "router/1122393453/uptime", 2);
        }else{
                fprintf(stderr, "Connect failed\n");
        }
}


int main(int argc, char *argv[]){

        // all var in one place ??
        char serial[100];

        prog_run = 1;

        int rc = EXIT_SUCCESS;

        //struct arguments args = {"-", 0};

        openlog(NULL, LOG_PID, LOG_USER);
        rc = open_file();
        if ( rc ){

                syslog(LOG_ERR, "Failed to open a file\n");
                goto EXIT_PROGRAM;  
        }

        //rc = args_parse(&args);

        if (rc){
                syslog(LOG_ERR, "Failed to parse program arguments\n");
                goto EXIT_PROGRAM;
        }

        //      R E C E I V E   C O N F I G S

        struct uci_context *uci_ctx = NULL;
        struct uci_package *package = NULL;

        uci_ctx = init_uci("subscriber", &package);
        if (uci_ctx == NULL) {
		syslog(LOG_ERR, "unable to init uci\n");
		rc = EPERM;
                goto EXIT_PROGRAM;
	}

        int topics_n;
        char **topics = NULL;

        rc = scan_topics(uci_ctx, package, &topics, &topics_n);
        if ( rc ){
                syslog(LOG_ERR, "unable to scan topics from config file\n");
                goto EXIT_PROGRAM;
        }

        for (int i = 0; i < topics_n; ++i){

                printf("topic = %s\n", topics[i]);
        }

        int events_n;
        struct event *events = NULL;

        rc = scan_events(uci_ctx, package, &events, &events_n, topics, topics_n);
        if ( rc ){
                syslog(LOG_ERR, "unable to scan events from config file\n");
                //goto EXIT_PROGRAM;
        }

        //      U B U S     M N F I N F O       G E T           S E R I A L    

        struct ubus_context *ubus_ctx;
        uint32_t id;

        rc = connect_ubus(&id, &ubus_ctx);

        if ( connect_ubus(&id, &ubus_ctx) ){
                syslog(LOG_ERR, "Failed to connect to ubus\n");
                goto EXIT_PROGRAM;
        }

        for (int i = 0; i < UBUS_CONNECT_MAX_FAILS; ++i){

                rc = ubus_invoke(ubus_ctx, id, "get", NULL, board_cb, (void *)serial, 3000);

                if ( rc ){

                        syslog(LOG_ERR, "Attempt: %d; Failed to invoke info from system. Max attemtps: %d\n", i+1, UBUS_CONNECT_MAX_FAILS);

                        sleep(3);

                        if ( i == UBUS_CONNECT_MAX_FAILS ){

                                syslog(LOG_ERR, "Failed to invoke get from mninfo\n");
                                goto EXIT_PROGRAM;
                        }


                        continue;
                }

                break;
        }

        printf("MY SERIAL NUMBER %s\n", serial);

        //      M Q     T E L E M E T R Y        T R A N S P O R T

        mosquitto_lib_init();

        struct events evs;
        evs.events = events;
        evs.events_n = events_n;

        struct link l;
        l.received = 0;

        

        // client id ????
        struct mosquitto *mosq = mosquitto_new(NULL, true, (void *)&l);

        if ( mosq == NULL ){
                syslog(LOG_ERR, "Failed initialize mosquitto\n");
                rc = EPERM;
                goto EXIT_PROGRAM;
        }

        mosquitto_connect_callback_set(mosq, conncet_callback);
        mosquitto_subscribe_callback_set(mosq, subscribe_callback);
        mosquitto_message_callback_set(mosq, message_callback);

        //rc = mosquitto_username_pw_set(mosq, "user", "pass");

        //rc = mosquitto_tls_set(mosq, );

        rc = mosquitto_connect(mosq, "localhost", 1883, 60);

        //rc = mosquitto_subscribe_multiple(mosq, NULL, topics_n, topics, 0, 0, NULL);

        for (int i = 0; i < topics_n; ++i ){

                char buff[150];
                sprintf(buff, "router/%s/%s", serial, topics[i]);
                rc = mosquitto_subscribe(mosq, NULL, buff, 0);
        }

        while ( prog_run ){

                if ( mosquitto_loop(mosq, -1, 1) == MOSQ_ERR_SUCCESS  ){

                        if ( l.received ){

                                l.received = 0;

                                write_topic(&l.topic);
                                print_topic(NULL);
                        }
                }
        }

        //rc = mosquitto_loop_forever(mosq, -1, 1);

        //print_events(events, events_n);

        free_topics(topics, topics_n);
        topics = NULL;
        free_events(events, events_n);
        events = NULL;

        
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
/*
        return rc;

        
*/
EXIT_PROGRAM:

        if ( topics == NULL )
                printf("topics null\n");

        if ( !uci_ctx )
                uci_free_context(uci_ctx);

        if ( !ubus_ctx )
                ubus_free(ubus_ctx);

        close_file();
        closelog();

        //mosquitto_destroy(mosq);
        //mosquitto_lib_cleanup();

        return rc;
}