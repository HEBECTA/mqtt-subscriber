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

int scan_events(struct uci_context *ctx, struct uci_package *pkg, struct event **evs, int *events_nmb,const char **topics, int topics_nmb){

        bool unsuccessfull_scan = false;
        int events_n = 0;

        struct uci_element *e;

        uci_foreach_element( &pkg->sections, e){

                struct uci_section *s = uci_to_section(e);
                if ( strcmp("event", s->type) == 0 ){

                        char receivers[MAX_NAME_NMB][MAX_NAME_SIZE];

                        struct event *event = (struct event *) malloc(sizeof(struct event));

                        if ( event == NULL  || unsuccessfull_scan) {

                                for (int i = 0; i < events_n; ++i)
                                        free(evs[i]);

                                return ENOMEM; // ???
                        }

                        ++events_n;

                        struct uci_option *o = uci_lookup_option(ctx, s, "topic");

                        if ( o == NULL ){
                                unsuccessfull_scan = true;
                                break;
                        }


                        event->topic_id = get_topic_id(topics, o->v.string, topics_nmb);

                        if ( event->topic_id == NO_SUCH_TOPIC ){
                                unsuccessfull_scan = true;
                                break;
                        }

                        //      P A R A M E T E R

                        o = uci_lookup_option(ctx, s, "parameter");

                        if ( o == NULL ){
                                unsuccessfull_scan = true;
                                break;
                        }

                        event->parameter = (char *) malloc(sizeof(char) * strlen(o->v.string) + 1);

                        if ( event->parameter == NULL ){
                                unsuccessfull_scan = true;
                                break;
                        }

                        strcpy(event->parameter, o->v.string);

                        //      V A L U E    T Y P E

                        o = uci_lookup_option(ctx, s, "value_type");

                        if ( o == NULL ){
                                unsuccessfull_scan = true;
                                break;
                        }

                        if ( strcmp("digit", o->v.string) )
                                event->isDigit = true;

                        else if (strcmp("string", o->v.string))
                                event->isDigit = false;

                        else{
                                unsuccessfull_scan = true;
                                break;
                        }

                        //      C O M P A R I S O N

                        o = uci_lookup_option(ctx, s, "comparison");

                        if ( o == NULL ){
                                unsuccessfull_scan = true;
                                break;
                        }

                        strcpy(event->comparison, o->v.string);

                        //      V A L U E    

                        o = uci_lookup_option(ctx, s, "value");

                        if ( o == NULL ){
                                unsuccessfull_scan = true;
                                break;
                        }

                        event->comparison = (char *) malloc(sizeof(char) * strlen(o->v.string) + 1);

                        if ( event->comparison == NULL ){
                                unsuccessfull_scan = true;
                                break;
                        }

                        strcpy(event->comparison, o->v.string);

                        //      E M A I L   

                        o = uci_lookup_option(ctx, s, "email");

                        if ( o == NULL ){
                                unsuccessfull_scan = true;
                                break;
                        }

                        event->email = (char *) malloc(sizeof(char) * strlen(o->v.string) + 1);

                        if ( event->email == NULL ){
                                unsuccessfull_scan = true;
                                break;
                        }

                        strcpy(event->email, o->v.string);

                        //      R E C E I V I N G       E M A I L S

                        o = uci_lookup_option(ctx, s, "receiver");

                        if ( o == NULL ){
                                unsuccessfull_scan = true;
                                break;
                        }

                        struct uci_element *l;

                        int receivers_nmb = 0;

                        uci_foreach_element(&o->v.list, l){

                                if ( l->name ){

                                        if ( strlen(l->name ) + 1 > MAX_NAME_SIZE ){
                                                unsuccessfull_scan = true;
                                                break;
                                        }

                                        strcpy(receivers[receivers_nmb], l->name);
                                        ++receivers_nmb;
                                }
                        }

                        if ( receivers_nmb > 0 ){

                                event->receiving_emails = (char **) malloc(sizeof(char *)*receivers_nmb);

                                if ( event->receiving_emails == NULL ){
                                        unsuccessfull_scan = true;
                                        break;
                                }

                                for (int i = 0; i < receivers_nmb; ++i ){

                                        event->receiving_emails[i] = (char *) malloc(sizeof(char) * strlen(receivers[i]) + 1);

                                        if ( event->receiving_emails[i] == NULL ){

                                                
                                        }
                                }
                        }
                }
        }

        return 0;
}

int get_topic_id(const char **topics, const char *topic, int topics_nmb){

        for (int i = 0; i < topics_nmb; ++i){

                if ( strcmp(topics[i], topic) == 0 ){

                        return i;
                }
        }

        return NO_SUCH_TOPIC;
}

void free_topics(char **topics, int topics_nmb){


}

void free_events(struct event *events, int events_nmb){


}
