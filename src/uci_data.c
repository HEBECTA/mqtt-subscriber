#include "uci_data.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

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

        uci_ctx = init_uci("subscriber", &package);
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

                uci_foreach_element(&o->v.list, l){

                        if ( l->name ){

                                struct topic *node = (struct topic *) malloc(sizeof(struct topic));

                                if ( node == NULL ){

                                        rc = ENODATA;
                                        goto EXIT_ERROR_SCAN_TOPICS_EVENTS;
                                }

                                //topic->name = (char *) malloc(sizeof(char) * strlen(l->name))

                                node->name = l->name;

                                node->next_topic = NULL;
                                node->ev_list = NULL;

                                if ( *topics == NULL )
                                        *topics = node;

                                else{
                                        struct topic *iterator = *topics->next_topic;

                                        while ( iterator->next_topic != NULL )
                                                iterator = iterator->next_topic;

                                        iterator->next_topic = node;
                                }
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
                //goto EXIT_ERROR_SCAN_TOPICS_EVENTS;
        }

        return rc;

EXIT_ERROR_SCAN_TOPICS_EVENTS:

        free_topics_events(topics);

        uci_free_context(uci_ctx);

        return rc;
}

static int scan_events(struct uci_context *ctx, struct uci_package *pkg, struct topic *topics){

        int rc = 0;
        struct event *node = NULL;
        struct uci_element *e;

        uci_foreach_element( &pkg->sections, e){

                struct uci_section *s = uci_to_section(e);
                if ( strcmp(EVENT_CONFIG_SECTION, s->type) == 0 ){

                        int receivers_counter = 0;
                        char receiving_emails[MAX_NAME_NMB][MAX_NAME_SIZE];

                        //      T O P I C

                        struct uci_option *o = uci_lookup_option(ctx, s, TOPIC_OPTION);

                        if ( o == NULL )
                                return EINVAL;

                        node = (struct event *) malloc(sizeof(struct event));

                        if ( node == NULL )
                                return ENOMEM;

                        node->parameter = NULL;
                        node->expected_value = NULL;
                        node->email = NULL;
                        node->receiv_emails_list = NULL:
                        node->next_event = NULL;

                        struct topic *topic = get_topic(topics, o->v.string);

                        if ( topic_id == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        //      P A R A M E T E R

                        o = uci_lookup_option(ctx, s, "parameter");

                        int parameter_flag = 1;

                        if ( o == NULL )
                                parameter_flag = 0;
                                
                       
                        if ( parameter_flag )
                                node->parameter = (char *) malloc(sizeof(char) * strlen(o->v.string) + 1);

                        else 
                                node->parameter = (char *) malloc(sizeof(char) * 1);

                        if ( node->parameter == NULL ){

                                rc = ENOMEM;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( parameter_flag )
                                strcpy(node->parameter, o->v.string);

                        else
                                node->parameter[0] = '\0';

                        //      V A L U E    T Y P E

                        o = uci_lookup_option(ctx, s, "value_type");

                        if ( o == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( strcmp("digit", o->v.string) == 0 )
                                node->isDigit = true;

                        else if (strcmp("string", o->v.string) == 0)
                                node->isDigit = false;

                        else{
                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        //      C O M P A R I S O N

                        o = uci_lookup_option(ctx, s, "comparison");

                        if ( o == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( strlen(o->v.string) > 2 ){

                                rc = ENOBUFS;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        strcpy(node->comparison, o->v.string);

                        //      V A L U E    

                        o = uci_lookup_option(ctx, s, "value");

                        if ( o == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        node->expected_value = (char * ) malloc(sizeof(char) * strlen(o->v.string) + 1);

                        if ( node->expected_value == NULL ){

                                rc = ENOMEM;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        strcpy(node->expected_value, o->v.string);

                        //      E M A I L   

                        o = uci_lookup_option(ctx, s, "emailgroup");

                        if ( o == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        node->email = (char *) malloc(sizeof(char) * strlen(o->v.string) + 1);

                        if ( node->email == NULL ){

                                rc = ENOMEM;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        strcpy(node->email, o->v.string);

                        //      R E C E I V I N G       E M A I L S

                        o = uci_lookup_option(ctx, s, "recipEmail");

                        if ( o == NULL ){

                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        struct uci_element *l;

                        uci_foreach_element(&o->v.list, l){

                                if ( l->name ){

                                        struct email *node = (struct email *) malloc(sizeof(struct email));

                                        if ( node == NULL ){

                                                rc = ENOMEM;
                                        }

                                        if ( strlen(l->name ) + 1 > MAX_NAME_SIZE ){

                                                rc = ENOBUFS;
                                                goto EXIT_SCAN_EVENT_ERROR;
                                        }
                                        
                                        strcpy(receiving_emails[receivers_counter], l->name);
                                        ++receivers_counter;
                                }
                        }

                        node->receivers = receivers_counter;

                        if ( receivers_counter > 0 ){

                                node->receiving_emails = (char **) malloc(sizeof(char *)*receivers_counter);

                                if ( node->receiving_emails == NULL ){

                                        rc = ENOMEM;
                                        goto EXIT_SCAN_EVENT_ERROR;
                                }

                                for (int i = 0; i < receivers_counter; ++i ){

                                        node->receiving_emails[i] = (char *) malloc(sizeof(char) * strlen(receiving_emails[i]) + 1);

                                        if ( node->receiving_emails[i] == NULL ){

                                                free(node->parameter);
                                                free(node->expected_value);
                                                free(node->email);
 
                                                for (int k = 0; k < i; ++k)
                                                        free(node->receiving_emails[k]);

                                                free( node );

                                                rc = ENOMEM;
                                                goto EXIT_SCAN_EVENT_ERROR;
                                        }

                                        strcpy(node->receiving_emails[i], receiving_emails[i]);
                                }
                        }

                        else{
                                rc = ;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }


                        node->next_event = NULL;

                        if ( topic.ev_list == NULL )
                                topic.ev_list = node;

                        else{

                                struct event *iterator = topic.ev_list;

                                while ( iterator->next_event != NULL )
                                        iterator = iterator->next_event;

                                iterator->next_event = node;
                        }
                }
        }

        return 0;


EXIT_SCAN_EVENT_ERROR:

        free_email( node );

        /*

        while ( topics != NULL ){

                struct event *iterator = topics->ev_list;
                struct event *prev_node;

                while ( iterator != NULL ){

                        prev_node = iterator;
                        iterator = iterator->next_event;
                        free(prev_node->parameter);
                        free(prev_node->expected_value);
                        free(prev_node->email);

                        for (int i = 0; i < prev_node->receivers; ++i)
                                free(prev_node->receiving_emails[i]);

                        free(prev_node);
                }

                free(iterator->parameter);
                free(iterator->expected_value);
                free(iterator->email);

                for (int i = 0; i < iterator->receivers; ++i)
                        free(iterator->receiving_emails[i]);

                free(iterator);
        }

        */

        return rc;
}

struct smtp_info *scan_email(const char *user_group){

        struct uci_context *uci_ctx = NULL;
        struct uci_package *package = NULL;

        uci_ctx = init_uci("user_groups", &package);
        if (uci_ctx == NULL)
                return NULL;

        struct smtp_info *smtp_info;

        struct uci_element *e;

        uci_foreach_element( &package->sections, e){

                struct uci_section *s = uci_to_section(e);
                if ( strcmp("email", s->type) == 0 ){

                        struct uci_option *o = uci_lookup_option(uci_ctx, s, "name");
                        if ( o == NULL )
                                goto EXIT_SCAN_EMAIL_ERROR;
                        
                        if ( strcmp(o->v.string, user_group) == 0 ){

                                smtp_info = (struct smtp_info *) malloc(sizeof(struct smtp_info));

                                if ( smtp_info == NULL )
                                        goto EXIT_SCAN_EMAIL_ERROR;

                                o = uci_lookup_option(uci_ctx, s, "smtp_port");
                                if ( o == NULL ){

                                        free(smtp_info);
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                }

                                smtp_info->port = atoi(o->v.string);

                                if ( !smtp_info->port ){

                                        free(smtp_info);
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                }

                                o = uci_lookup_option(uci_ctx, s, "smtp_ip");
                                if ( o == NULL ){

                                        free(smtp_info);
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                }

                                smtp_info->smtp_domain = (char *) malloc(sizeof(char) * strlen(o->v.string) + 1);

                                if ( smtp_info->smtp_domain == NULL ){

                                        free(smtp_info);
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                }

                                strcpy(smtp_info->smtp_domain, o->v.string);

                                o = uci_lookup_option(uci_ctx, s, "senderemail");
                                if ( o == NULL ){

                                        free(smtp_info->smtp_domain);
                                        free(smtp_info->username);
                                        free(smtp_info);
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                }

                                smtp_info->sending_email = (char *) malloc(sizeof(char) * strlen(o->v.string) + 1);

                                strcpy(smtp_info->sending_email, o->v.string);

                                o = uci_lookup_option(uci_ctx, s, "credentials");
                                if ( o == NULL ){

                                        free(smtp_info->smtp_domain);
                                        free(smtp_info);
                                        goto EXIT_SCAN_EMAIL_ERROR;
                                }

                                if ( strcmp(o->v.string, "1") == 0 ){

                                        o = uci_lookup_option(uci_ctx, s, "username");
                                        if ( o == NULL ){

                                                free(smtp_info->smtp_domain);
                                                free(smtp_info->username);
                                                free(smtp_info);
                                                goto EXIT_SCAN_EMAIL_ERROR;
                                        }

                                        smtp_info->username = (char *) malloc(sizeof(char) * strlen(o->v.string) + 1);
                                        strcpy(smtp_info->username, o->v.string);

                                        o = uci_lookup_option(uci_ctx, s, "password");
                                        if ( o == NULL ){

                                                free(smtp_info->smtp_domain);
                                                free(smtp_info->username);
                                                free(smtp_info);
                                                goto EXIT_SCAN_EMAIL_ERROR;
                                        }

                                        smtp_info->password = (char *) malloc(sizeof(char) * strlen(o->v.string) + 1);
                                        strcpy(smtp_info->password, o->v.string);
                                }

                                else{
                                        printf("no crdencials\n");
                                }

                                goto SUCCESSFUL_EXIT;
                        }
                }
        }

SUCCESSFUL_EXIT:

printf("good emails\n");

        uci_free_context(uci_ctx);

        return smtp_info;

EXIT_SCAN_EMAIL_ERROR:

printf("wrong emails\n");

        uci_free_context(uci_ctx);

        return NULL;
}

/*
static int get_events_nmb(struct uci_package *pkg){

        int events_nmb = 0;

        struct uci_element *e;

        uci_foreach_element( &pkg->sections, e){

                struct uci_section *s = uci_to_section(e);

                if ( strcmp(EVENT_CONFIG_SECTION, s->type) == 0 )
                        ++events_nmb;
        }

        return events_nmb;
}
*/
