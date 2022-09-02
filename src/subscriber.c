#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "topic_event.h"
#include "uci_data.h"
#include "file_io.h"
#include "mqtt_sub.h"
#include "curl_email.h"
#include "args.h"

//#include "serial_ubus.h"

int prog_run;

void sig_handler(int signum){

  prog_run = 0;

  syslog(LOG_NOTICE, "Received a SIGINT signal. Terminating program\n");
}

int main(int argc, char *argv[]){

        signal(SIGINT, sig_handler);

        prog_run = 1;
        struct mosquitto *mosq = NULL;
        struct topic *topics_events = NULL;
        int topics_n = 0;
        struct message msg_info;
        struct arguments options = {"-", 0, "-", "-", "-"};

        int rc = EXIT_SUCCESS;

        //      System log 
        openlog(NULL, LOG_PID, LOG_USER);

        //      File which stores received mqtt topics
        rc = open_file();
        if ( rc ){

                syslog(LOG_ERR, "MQTT: Failed to open a file with topics\n");
                goto EXIT_PROGRAM;  
        }

        //      P R O G R A M           A R G U M E N T S

        for (int i = 0; i < argc; ++i ){

                printf("%s\n", argv[i]);
        }

        rc = args_parse(argc, argv, &options);
        if (rc){
                syslog(LOG_ERR, "MQTT: Failed to parse program arguments\n");
                goto EXIT_PROGRAM;
        }

        //      R E C E I V E   C O N F I G S

        rc = scan_topics_events(&topics_events);
        if ( rc ){
                syslog(LOG_ERR, "MQTT: failed to scan topics and events from config file\n");
                goto EXIT_PROGRAM;
        }

        //      M Q     T E L E M E T R Y        T R A N S P O R T

        mosq  = mqtt_init_subscribe(topics_n, topics_events, &msg_info, options);
        if ( mosq == NULL )
                goto EXIT_PROGRAM;

        while ( prog_run ){

                if ( mosquitto_loop(mosq, -1, 1) == MOSQ_ERR_SUCCESS ){

                        if ( msg_info.received ){

                                msg_info.received = 0;

                                printf("message received, topic = %s\n", msg_info.topic);

                                printf("message %s\n", msg_info.msg);

                                int topic_id = get_topic_id(topics_events, topics_n, msg_info.topic);

                                if ( topic_id != NO_SUCH_TOPIC ){

                                        struct event *matchedEvent = topic_message_matches_event(topics_events[topic_id].ev_list, msg_info.msg);

                                        while ( matchedEvent != NULL ){

                                                syslog(LOG_NOTICE, "MQTT: Received subscribed topic %s which matches event", msg_info.topic);

                                                for (int i = 0; i < matchedEvent->receivers; ++i){
/*
                                                        rc = send_email(matchedEvent->email, matchedEvent->receiving_emails[i], msg_info.topic, msg_info.msg);
                                                        if ( rc )
                                                                syslog(LOG_ERR, "Failed to send email to %s\n", matchedEvent->receiving_emails[i]);*/
                                                }

                                                matchedEvent = topic_message_matches_event(matchedEvent->next_event, msg_info.msg);
                                        }

                                        Topic topic;
                                        time_t date = time(0); 
                                        set_topic(&topic, msg_info.topic, msg_info.msg, QoS0, date);

                                        write_file(&topic);
                                        
                                        continue;
                                }
                        }

                        if ( !prog_run )
                                break;
                }
        }



EXIT_PROGRAM:

//while(1);

        //ubus_free(ctx);
	//uloop_done();

        if ( topics_events != NULL )
                free_topics_events(topics_events, topics_n);

        close_file();
        closelog();

        if ( mosq != NULL )
                mosquitto_destroy(mosq);

        mosquitto_lib_cleanup();

        return rc;
}