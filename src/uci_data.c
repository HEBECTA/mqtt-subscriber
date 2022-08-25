#include "uci_data.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

struct uci_context* init_uci(const char *file, struct uci_package **pkg){

        struct uci_context *ctx = NULL;
        ctx = uci_alloc_context();
        if (ctx == NULL) {
		
                return ctx;
	}

        // fix path
        int rc = uci_set_confdir(ctx, "/etc/config/");
        if ( rc ){
                uci_free_context(ctx);
                return NULL;
        }

        rc = uci_load(ctx, "subscriber", pkg);
        if ( rc ){
                uci_free_context(ctx);
                return NULL;
        }

        return ctx;
}


int scan_topics(struct uci_context *ctx, struct uci_package *pkg, char ***topics, int *topics_nmb){

        char topic_names[MAX_NAME_NMB][MAX_NAME_SIZE];

        int topics_n = 0;

        struct uci_section *sct = uci_lookup_section(ctx, pkg, "subscriber_info");

        if ( sct == NULL )
                return ENODATA;

        struct uci_option *o = uci_lookup_option(ctx, sct, "topic");

        if ( o == NULL )
                return ENODATA;
        
        if ( o->type == UCI_TYPE_LIST ){

                struct uci_element *l;

                uci_foreach_element(&o->v.list, l){

                        if ( l->name ){

                                if ( strlen(l->name ) + 1 > MAX_NAME_SIZE )
                                        return ENAMETOOLONG; // ENOBUFS

                                strcpy(topic_names[topics_n], l->name);
                                ++topics_n;
                        }
                }
        }       

        else if (o->type == UCI_TYPE_STRING)
                return ENODATA;

        else
                return ENODATA;

        *topics = (char **) malloc(sizeof(char *) * topics_n );
        if ( *topics == NULL )
                return ENOMEM;

        for (int i = 0; i < topics_n; ++i){

                (*topics)[i] = (char *) malloc(sizeof(char) * strlen(topic_names[i]) + 1 );

                if ((*topics)[i] == NULL ){

                        for (int k = 0; k < i; ++k )
                                free((*topics)[k]);

                        return ENOMEM;
                }

                strcpy((*topics)[i], topic_names[i]);
        }

        *topics_nmb = topics_n;

        return 0;
}

int scan_events(struct uci_context *ctx, struct uci_package *pkg, struct event **evs, int *events_nmb, char **topics, int topics_nmb){

        int rc = 0;

        int events_n = get_events_nmb(pkg);

        if ( events_n == 0 )
                return 0;

        *evs = (struct event *) malloc(sizeof(struct event) * events_n);

        if ( evs == NULL )
                return ENOMEM;

        int events_counter = 0;

        struct uci_element *e;

        uci_foreach_element( &pkg->sections, e){

                struct uci_section *s = uci_to_section(e);
                if ( strcmp("event", s->type) == 0 ){

                        int receivers_counter = 0;
                        char receiving_emails[MAX_NAME_NMB][MAX_NAME_SIZE];

                        struct event *event = (struct event *) malloc(sizeof(struct event));

                        if ( event == NULL ){

                                rc = ENOMEM;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        //      T O P I C

                        struct uci_option *o = uci_lookup_option(ctx, s, "topic");

                        if ( o == NULL ){

                                free(event);
                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        event->topic_id = get_topic_id(topics, o->v.string, topics_nmb);

                        if ( event->topic_id == NO_SUCH_TOPIC ){

                                free(event);
                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        //      P A R A M E T E R

                        o = uci_lookup_option(ctx, s, "parameter");

                        if ( o == NULL ){

                                free(event);
                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( strlen(o->v.string) > MAX_NAME_SIZE + 1 ){

                                free(event);
                                rc = ENOBUFS;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        event->parameter = (char *) malloc(sizeof(char) * strlen(o->v.string) + 1);

                        if ( event->parameter == NULL ){

                                free(event);
                                rc = ENOMEM;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        strcpy(event->parameter, o->v.string);

                        //      V A L U E    T Y P E

                        o = uci_lookup_option(ctx, s, "value_type");

                        if ( o == NULL ){
                                free(event->parameter);
                                free(event);
                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( strcmp("digit", o->v.string) )
                                event->isDigit = true;

                        else if (strcmp("string", o->v.string))
                                event->isDigit = false;

                        else{

                                free(event->parameter);
                                free(event);
                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        //      C O M P A R I S O N

                        o = uci_lookup_option(ctx, s, "comparison");

                        if ( o == NULL ){

                                free(event->parameter);
                                free(event);
                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( strlen(o->v.string) > 2 ){ // ???? 2

                                free(event->parameter);
                                free(event);
                                rc = ENOBUFS;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        strcpy(event->comparison, o->v.string);

                        //      V A L U E    

                        o = uci_lookup_option(ctx, s, "value");

                        if ( o == NULL ){

                                free(event->parameter);
                                free(event);
                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( strlen(o->v.string) > MAX_NAME_SIZE + 1 ){

                                free(event->parameter);
                                free(event);
                                rc = ENOBUFS;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        event->expected_value = (char * ) malloc(sizeof(char) * strlen(o->v.string) + 1);

                        if ( event->expected_value == NULL ){

                                free(event->parameter);
                                free(event);
                                rc = ENOMEM;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        strcpy(event->expected_value, o->v.string);

                        //      E M A I L   

                        o = uci_lookup_option(ctx, s, "email");

                        if ( o == NULL ){

                                free(event->parameter);
                                free(event->expected_value);
                                free(event);
                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        if ( strlen(o->v.string) > MAX_NAME_SIZE + 1 ){

                                free(event->parameter);
                                free(event->expected_value);
                                free(event);
                                rc = ENOBUFS;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        event->email = (char *) malloc(sizeof(char) * strlen(o->v.string) + 1);

                        if ( event->email == NULL ){

                                free(event->parameter);
                                free(event->expected_value);
                                free(event);
                                rc = ENOMEM;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        strcpy(event->email, o->v.string);

                        //      R E C E I V I N G       E M A I L S

                        o = uci_lookup_option(ctx, s, "receiver");

                        if ( o == NULL ){

                                free(event->parameter);
                                free(event->expected_value);
                                free(event->email);
                                free(event);
                                rc = EINVAL;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        struct uci_element *l;

                        uci_foreach_element(&o->v.list, l){

                                if ( l->name ){

                                        if ( strlen(l->name ) + 1 > MAX_NAME_SIZE ){

                                                free(event->parameter);
                                                free(event->expected_value);
                                                free(event->email);
                                                free(event);
                                                rc = ENOBUFS;
                                                goto EXIT_SCAN_EVENT_ERROR;
                                        }
                                        
                                        strcpy(receiving_emails[receivers_counter], l->name);
                                        ++receivers_counter;
                                }
                        }

                        event->receivers = receivers_counter;

                        printf("%d\n", receivers_counter);

                        if ( receivers_counter > 0 ){

                                event->receiving_emails = (char **) malloc(sizeof(char *)*receivers_counter);

                                if ( event->receiving_emails == NULL ){

                                        free(event->parameter);
                                        free(event->expected_value);
                                        free(event->email);
                                        free(event);
                                        rc = ENOMEM;
                                        goto EXIT_SCAN_EVENT_ERROR;
                                }

                                for (int i = 0; i < receivers_counter; ++i ){

                                        event->receiving_emails[i] = (char *) malloc(sizeof(char) * strlen(receiving_emails[i]) + 1);

                                        if ( event->receiving_emails == NULL ){

                                                free(event->parameter);
                                                free(event->expected_value);
                                                free(event->email);
                                                free(event);

                                                for (int k = 0; k < i; ++k)
                                                        free(event->receiving_emails[k]);

                                                rc = ENOMEM;
                                                goto EXIT_SCAN_EVENT_ERROR;
                                        }

                                        strcpy(event->receiving_emails[i], receiving_emails[i]);
                                }
                        }

                        else{

                                ++events_counter;
                                goto EXIT_SCAN_EVENT_ERROR;
                        }

                        (*evs)[events_counter] = *event;

                        ++events_counter;

                        if ( events_counter > events_n )
                                goto EXIT_SCAN_EVENT_ERROR;
                }
        }

        *events_nmb = events_counter;

        return 0;

EXIT_SCAN_EVENT_ERROR:

        for (int i = 0; i < events_counter; ++i){

                free((*evs)[i].parameter);
                free((*evs)[i].expected_value);
                free((*evs)[i].email);

                for (int k = 0; k < (*evs)[i].receivers; ++k)
                        free((*evs)[i].receiving_emails[k]);
                
                free((*evs)[i].receiving_emails);
        }

        free(*evs);

        return rc;
}

int get_topic_id(char **topics, char *topic, int topics_nmb){

        for (int i = 0; i < topics_nmb; ++i){

                if ( strcmp(topics[i], topic) == 0 ){

                        return i;
                }
        }

        return NO_SUCH_TOPIC;
}

static int get_events_nmb(struct uci_package *pkg){

        int events_nmb = 0;

        struct uci_element *e;

        uci_foreach_element( &pkg->sections, e){

                struct uci_section *s = uci_to_section(e);

                if ( strcmp("event", s->type) == 0 )
                        ++events_nmb;
        }

        return events_nmb;
}

void free_topics(char **topics, int topics_nmb){

        for (int i = 0; i < topics_nmb; ++i )
                free(topics[i]);

        free(topics);
}

void free_events(struct event *events, int events_nmb){

        for (int i = 0; i < events_nmb; ++i ){

                free(events[i].parameter);
                free(events[i].expected_value);
                free(events[i].email);

                for (int k = 0; k < events[i].receivers; ++k)
                        free(events[i].receiving_emails[k]);

                free(events[i].receiving_emails);
        }

        free(events);
}

void print_events(struct event *events, int events_nmb){

        for (int i = 0; i < events_nmb; ++i ){

                printf("topic %d\n", events[i].topic_id);
                printf("parameter %s\n", events[i].parameter);
                printf("isDigit %d\n", events[i].isDigit);
                printf("comparison %s\n", events[i].comparison);
                printf("expected_value %s\n", events[i].expected_value);
                printf("email %s\n", events[i].email);
                printf("receivers %d\n", events[i].receivers);

                for (int k = 0; k < events[i].receivers; ++k)
                        printf("receiving_emails %s\n", events[i].receiving_emails[k]);
        }

        printf("\n");
        //printf("\n"); ????????????????
}
