#include "uci_data.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

static struct uci_context* init_uci(const char *file, struct uci_package **pkg){

        struct uci_context *ctx = NULL;
        ctx = uci_alloc_context();
        if (ctx == NULL) {
		
                return NULL;
	}

        int rc = uci_set_confdir(ctx, CONFIG_PATH);
        if ( rc ){
                uci_free_context(ctx);
                return NULL;
        }

        rc = uci_load(ctx, file, pkg);
        if ( rc ){
                uci_free_context(ctx);
                return NULL;
        }

        return ctx;
}


int scan_topics_events(struct topic **topics){

        int rc = 0;

        *topics = NULL;
        struct uci_context *uci_ctx = NULL;
        struct uci_package *package = NULL;

        uci_ctx = init_uci(TOPIC_EVENT_CONFIG_FILE, &package);
        if (uci_ctx == NULL)
                return ENODATA;
        
        struct uci_section *sct = uci_lookup_section(uci_ctx, package, TOPIC_CONFIG_SECTION);

        if ( sct == NULL ){

                rc = ENODATA;
                goto EXIT_ERROR_SCAN_TOPICS_EVENTS;
        }

        struct uci_option *o = uci_lookup_option(uci_ctx, sct, TOPIC_OPTION);

        if ( o == NULL ){

                rc = ENODATA;
                goto EXIT_ERROR_SCAN_TOPICS_EVENTS;
        }
        
        if ( o->type == UCI_TYPE_LIST ){

                struct uci_element *l;

                struct topic **iterator = topics;

                uci_foreach_element(&o->v.list, l){

                        if ( l->name ){

                                struct topic *node = (struct topic *) malloc(sizeof(struct topic));

                                if ( node == NULL ){

                                        rc = ENODATA;
                                        goto EXIT_ERROR_SCAN_TOPICS_EVENTS;
                                }

                                node->name = (char *) malloc(sizeof(char) * (strlen(l->name) + 1));
                                if ( node->name == NULL ){

                                        rc = ENOMEM;
                                        goto EXIT_ERROR_SCAN_TOPICS_EVENTS;
                                }

                                strcpy(node->name, l->name);

                                node->next_topic = NULL;
                                node->ev_list = NULL;

                                *iterator = node;

                                iterator = &(*iterator)->next_topic;
                        }
                }
        }       

        else{
                rc = ENODATA;
                goto EXIT_ERROR_SCAN_TOPICS_EVENTS;
        }

        rc = scan_events(uci_ctx, package, *topics);
        if ( rc ){
                syslog(LOG_ERR, "Failed to scan events");
                rc = 0;
                goto EXIT_ERROR_SCAN_TOPICS_EVENTS;
        }

        return rc;

EXIT_ERROR_SCAN_TOPICS_EVENTS:

        free_topics_events(*topics);

        uci_free_context(uci_ctx);

        return rc;
}

static int scan_events(struct uci_context *ctx, struct uci_package *pkg, struct topic *topics){

        int rc = 0;
        struct event *event_node = NULL;
        struct uci_element *e;

        uci_foreach_element( &pkg->sections, e){

                struct uci_section *s = uci_to_section(e);
                if ( strcmp(EVENT_CONFIG_SECTION, s->type) == 0 ){

                        int receivers_counter = 0;

                        //      T O P I C

                        struct uci_option *o = uci_lookup_option(ctx, s, TOPIC_OPTION);

                        if ( o == NULL )
                                return EINVAL;

                        event_node = (struct event *) malloc(sizeof(struct event));

                        if ( event_node == NULL )
                                return ENOMEM;

                        event_node->parameter = NULL;
                        event_node->expected_value = NULL;
                        event_node->email = NULL;
                        event_node->receiv_emails_list = NULL;
                        event_node->next_event = NULL;

                        struct topic *topic = get_topic_by_name(topics, o->v.string);

                        if ( topic == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        //      P A R A M E T E R

                        o = uci_lookup_option(ctx, s, EVENT_PARAMETER_OPTION);

                        int parameter_flag = 1;

                        if ( o == NULL )
                                parameter_flag = 0;
                                
                       
                        if ( parameter_flag )
                                event_node->parameter = (char *) malloc(sizeof(char) * (strlen(o->v.string) + 1));

                        else 
                                event_node->parameter = (char *) malloc(sizeof(char) * 2);

                        if ( event_node->parameter == NULL ){

                                rc = ENOMEM;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( parameter_flag )
                                strcpy(event_node->parameter, o->v.string);

                        else
                                event_node->parameter[0] = '\0';

                        //      V A L U E    T Y P E

                        o = uci_lookup_option(ctx, s, EVENT_VALUE_TYPE_OPTION);

                        if ( o == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( strcmp("digit", o->v.string) == 0 )
                                event_node->isDigit = true;

                        else if (strcmp("string", o->v.string) == 0)
                                event_node->isDigit = false;

                        else{
                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        //      C O M P A R I S O N

                        o = uci_lookup_option(ctx, s, EVENT_COMPARISON_OPTION);

                        if ( o == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( strlen(o->v.string) > 2 ){

                                rc = ENOBUFS;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        strcpy(event_node->comparison, o->v.string);

                        //      V A L U E    

                        o = uci_lookup_option(ctx, s, EVENT_VALUE_OPTION);

                        if ( o == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        event_node->expected_value = (char * ) malloc(sizeof(char) * (strlen(o->v.string) + 1));

                        if ( event_node->expected_value == NULL ){

                                rc = ENOMEM;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        strcpy(event_node->expected_value, o->v.string);

                        //      E M A I L   

                        o = uci_lookup_option(ctx, s, EVENT_EMAIL_OPTION);

                        if ( o == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        event_node->email = (char *) malloc(sizeof(char) * (strlen(o->v.string) + 1));

                        if ( event_node->email == NULL ){

                                rc = ENOMEM;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        strcpy(event_node->email, o->v.string);

                        //      R E C E I V I N G       E M A I L S

                        o = uci_lookup_option(ctx, s, EVENT_REC_EMAIL_OPTION);

                        if ( o == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        struct email **email_list_iterator = &(event_node->receiv_emails_list);

                        struct uci_element *l;

                        uci_foreach_element(&o->v.list, l){

                                if ( l->name ){

                                        struct email *email_node = (struct email *) malloc(sizeof(struct email));

                                        if ( email_node == NULL ){

                                                rc = ENOMEM;
                                                goto EXIT_SCAN_EVENT_ERROR;
                                        }

                                        email_node->email_name = l->name;

                                        email_node->next_email = NULL;

                                        *email_list_iterator = email_node;
                                        email_list_iterator = &(*email_list_iterator)->next_email;

                                        ++receivers_counter;
                                }
                        }

                        if ( receivers_counter < 1 ){

                                rc = ENODATA;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( topic->ev_list == NULL )
                                topic->ev_list = event_node;

                        else{

                                struct event *iterator = topic->ev_list;

                                while ( iterator->next_event != NULL )
                                        iterator = iterator->next_event;

                                iterator->next_event = event_node;
                        }
                }
        }

        return 0;


EXIT_SCAN_EVENT_ERROR:

        if (event_node->parameter != NULL )
                free(event_node->parameter);

        if (event_node->expected_value != NULL )
                free(event_node->expected_value);

        if (event_node->email != NULL )
                free(event_node->email);

        if (event_node->receiv_emails_list != NULL ){

                struct email *email_list_iterator = event_node->receiv_emails_list;
                struct email *prev_email = email_list_iterator;

                while ( email_list_iterator != NULL ){

                        email_list_iterator = email_list_iterator->next_email;
                        free(prev_email);
                        prev_email = email_list_iterator;
                }
        }

        return rc;
}

struct smtp_info *scan_email(const char *user_group){

        struct uci_context *uci_ctx = NULL;
        struct uci_package *package = NULL;

        int port = 0;
        int port_len = 0;

        uci_ctx = init_uci(USER_GROUPS_CONFIG_FILE, &package);
        if (uci_ctx == NULL)
                return NULL;

        struct smtp_info *smtp_info = NULL;

        struct uci_element *e;

        uci_foreach_element( &package->sections, e){

                struct uci_section *s = uci_to_section(e);
                if ( strcmp(USER_GROUPS_SECTION, s->type) == 0 ){

                        struct uci_option *o = uci_lookup_option(uci_ctx, s, "name");
                        if ( o == NULL )
                                goto EXIT_SCAN_EMAIL_ERROR;
                        
                        if ( strcmp(o->v.string, user_group) == 0 ){

                                smtp_info = (struct smtp_info *) malloc(sizeof(struct smtp_info));

                                if ( smtp_info == NULL )
                                        goto EXIT_SCAN_EMAIL_ERROR;

                                smtp_info->sending_email = NULL;
                                smtp_info->smtp_domain = NULL;
                                smtp_info->username = NULL;
                                smtp_info->password = NULL;

                                o = uci_lookup_option(uci_ctx, s, EMAIL_SMTP_PORT_OPTION);
                                if ( o == NULL )
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                

                                port = atoi(o->v.string);

                                if ( !port )
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                

                                o = uci_lookup_option(uci_ctx, s, EMAIL_SMTP_IP_OPTION);
                                if ( o == NULL )
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                

                                int n = port;
                                do {
                                n /= 10;
                                ++port_len;
                                } while (n != 0);

                                smtp_info->smtp_domain = (char *) malloc(sizeof(char) * (strlen("smtp://") + strlen(o->v.string) + strlen(":") + port_len) + 1);

                                if ( smtp_info->smtp_domain == NULL )
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                
                                sprintf(smtp_info->smtp_domain, "smtp://%s:%d", o->v.string, port);

                                o = uci_lookup_option(uci_ctx, s, EMAIL_OPTION);
                                if ( o == NULL )
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                

                                smtp_info->sending_email = (char *) malloc(sizeof(char) * (strlen(o->v.string) + 1));

                                strcpy(smtp_info->sending_email, o->v.string);

                                o = uci_lookup_option(uci_ctx, s, EMAIL_CREDENTIALS_OPTION);
                                if ( o == NULL )
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                

                                if ( strcmp(o->v.string, "1") == 0 ){

                                        o = uci_lookup_option(uci_ctx, s, EMAIL_USERNAME_OPTION);
                                        if ( o == NULL )
                                                goto EXIT_SCAN_EMAIL_ERROR;

                                        smtp_info->username = (char *) malloc(sizeof(char) * (strlen(o->v.string) + 1));
                                        strcpy(smtp_info->username, o->v.string);

                                        o = uci_lookup_option(uci_ctx, s, EMAIL_PASSWORD_OPTION);
                                        if ( o == NULL )
                                                goto EXIT_SCAN_EMAIL_ERROR;
                                        

                                        smtp_info->password = (char *) malloc(sizeof(char) * (strlen(o->v.string) + 1));
                                        strcpy(smtp_info->password, o->v.string);
                                }

                                else{
                                        syslog(LOG_ERR, "MQTT: No sending email credentials");
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                }

                                goto SUCCESSFUL_EXIT;
                        }
                }
        }

SUCCESSFUL_EXIT:

        uci_free_context(uci_ctx);

        return smtp_info;

EXIT_SCAN_EMAIL_ERROR:

        if ( smtp_info != NULL ){

                if ( smtp_info->sending_email != NULL )
                        free(smtp_info->sending_email);

                if ( smtp_info->smtp_domain != NULL )
                        free(smtp_info->smtp_domain);

                if ( smtp_info->username != NULL )
                        free(smtp_info->username);

                if ( smtp_info->password != NULL )
                        free(smtp_info->password);

                free(smtp_info);
        }

        uci_free_context(uci_ctx);

        return NULL;
}
