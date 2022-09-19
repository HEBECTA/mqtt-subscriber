#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "mqtt_sub.h"
#include "topic_event.h"
#include "uci_data.h"
#include "file_io.h"
#include "curl_email.h"
#include "args.h"

int prog_run;

void sig_handler(int signum){

  prog_run = 0;

  syslog(LOG_NOTICE, "MQTT: Received a SIGINT signal. Terminating program\n");
}

int main(int argc, char *argv[]){

        signal(SIGINT, sig_handler);

        prog_run = 1;
        struct mosquitto *mqtt_ctx = NULL;
        struct topic *topics_events = NULL;
        int topics_n = 0;
        struct message msg_info = {NULL, NULL, 0};
        struct arguments options = {"-", 0, "-", "-", "-"};
        //struct arguments options = {"192.168.1.1", 1883, "tester", "tester", "/etc/certificates/ca.cert.pem"};

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

        rc = init_message_buffer(&msg_info);
        if ( rc ){

                syslog(LOG_ERR, "MQTT: failed to initialize buffer for mqtt messages\n");
                goto EXIT_PROGRAM;
        }

        //      M Q     T E L E M E T R Y        T R A N S P O R T

        mqtt_ctx  = mqtt_init_subscribe(topics_n, topics_events, &msg_info, options);
        if ( mqtt_ctx == NULL )
                goto EXIT_PROGRAM;

        while ( prog_run ){

                if ( mosquitto_loop(mqtt_ctx, -1, 1) == MOSQ_ERR_SUCCESS ){

                        if ( !msg_info.received )
                                continue;
                                
                        if ( !prog_run )
                                break;

                        msg_info.received = 0;

                        rc = send_matched_events_emails(topics_events, &msg_info);
                        if ( rc )
                                syslog(LOG_ERR, "MQTT: Failed to send email");

                        rc = write_topic_to_file(&msg_info);
                        if ( rc )
                                syslog(LOG_ERR, "MQTT: failed to write received message to file");
                }
        }

EXIT_PROGRAM:

        free__message_buffer(msg_info);

        free_topics_events(topics_events);

        close_file();
        closelog();

        mqtt_free(mqtt_ctx);

        return rc;
}