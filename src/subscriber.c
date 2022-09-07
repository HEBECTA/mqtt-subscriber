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

int prog_run;

void sig_handler(int signum){

  prog_run = 0;

  syslog(LOG_NOTICE, "Received a SIGINT signal. Terminating program\n");
}

int main(int argc, char *argv[]){

        signal(SIGINT, sig_handler);

        prog_run = 1;
        struct mosquitto *mqtt_ctx = NULL;
        struct topic *topics_events = NULL;
        int topics_n = 0;
        struct message msg_info = {NULL, 0, NULL, 0, 0};
        struct arguments options = {"-", 0, "-", "-", "-"};
        //struct arguments options = {"192.168.1.1", 1883, "tester", "tester", "/etc/certificates/cert.cert.pem"};

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

        //print_events(topics_events);

        //      M Q     T E L E M E T R Y        T R A N S P O R T

        mosquitto_lib_init();

        mqtt_ctx  = mqtt_init_subscribe(topics_n, topics_events, &msg_info, options);
        if ( mqtt_ctx == NULL )
                goto EXIT_PROGRAM;

        while ( prog_run ){

                if ( mosquitto_loop(mqtt_ctx, -1, 1) == MOSQ_ERR_SUCCESS ){

                        if ( msg_info.received ){

                                msg_info.received = 0;

                                //printf("message received, topic = %s\n", msg_info.topic);

                                //printf("message %s\n", msg_info.msg);

                                struct topic *topic = get_topic_by_name(topics_events, msg_info.topic);

                                if ( topic != NULL ){

                                        struct event *matchedEvent = topic_message_matches_event(topic->ev_list, msg_info.msg);

                                        while ( matchedEvent != NULL ){

                                                struct smtp_info *sender_info = scan_email(matchedEvent->email);
                                                if ( sender_info == NULL ){

                                                        syslog(LOG_ERR, "Failed to find sending email %s in user_groups", matchedEvent->email);
                                                        continue;
                                                }

                                                syslog(LOG_NOTICE, "MQTT: Received subscribed topic %s which matches event", msg_info.topic);

                                                struct email *email_list_iterator = matchedEvent->receiv_emails_list;
                                                while ( email_list_iterator != NULL ){

                                                        //printf("sending email to %s, matched value %s\n", email_list_iterator->email_name, matchedEvent->expected_value);

                                                        rc = send_email(sender_info, email_list_iterator->email_name, msg_info.topic, msg_info.msg);
                                                        if ( rc )
                                                                syslog(LOG_ERR, "Failed to send email to %s\n", email_list_iterator->email_name);

                                                        email_list_iterator = email_list_iterator->next_email;
                                                }

                                                free_smtp_info(sender_info);

                                                matchedEvent = topic_message_matches_event(matchedEvent->next_event, msg_info.msg);
                                        }

                                        Topic topic;
                                        time_t date = time(0); 
                                        set_topic(&topic, msg_info.topic, msg_info.msg, QoS0, date);

                                        write_file(&topic);
                                }
                        }

                        //free_message(msg_info); // ??? illegal isntruction

                        if ( !prog_run )
                                break;
                }
        }

EXIT_PROGRAM:

        free_message(msg_info);

        if ( topics_events != NULL )
                free_topics_events(topics_events);

        close_file();
        closelog();

        if ( mqtt_ctx != NULL )
                mosquitto_destroy(mqtt_ctx);

        mosquitto_lib_cleanup();

        return rc;
}