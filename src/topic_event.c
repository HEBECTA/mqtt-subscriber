#include "topic_event.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <json-c/json.h>

struct topic *get_topic_by_name(struct topic *topics, const char *topic_name){

        while (topics != NULL ){

                if ( strcmp(topics->name, topic_name) == 0 )
                        return topics;

                topics = topics->next_topic;
        }

        return NULL;
}

void free_topics_events(struct topic *topics){

        if ( topics != NULL ){

                struct topic *iterator = topics;
                struct topic *prev_node;

                while ( iterator->next_topic != NULL ){

                        prev_node = iterator;
                        iterator = iterator->next_topic;
                        if ( prev_node->ev_list != NULL )
                                free_event_list(prev_node->ev_list);
                        if ( prev_node->name != NULL )
                                free(prev_node->name);
                        free(prev_node);
                }

                free_event_list(iterator->ev_list);
                free(iterator);
        }
}

static void free_event_list(struct event *events){

        while (events != NULL ){

                free(events->parameter);
                free(events->expected_value);
                free(events->email);
                free_email_list(events->receiv_emails_list);

                struct event *prev_event = events;
                events = events->next_event;

                free(prev_event);
        }
}

static void free_email_list(struct email *emails){

        while (emails != NULL ){

                free(emails->email_name);
   
                struct email *prev_email = emails;
                emails = emails->next_email;

                free(prev_email);
        }
}

void print_events(struct topic *topics){

        while ( topics != NULL ){

                printf("topic %s\n\n", topics->name);

                struct event *event_list_iterator = topics->ev_list;

                while ( event_list_iterator!= NULL ){

                        if ( strlen(event_list_iterator->parameter) )
                                printf("parameter %s\n", event_list_iterator->parameter);
                        printf("isDigit %d\n", event_list_iterator->isDigit);
                        printf("comparison %s\n", event_list_iterator->comparison);
                        printf("expected_value %s\n", event_list_iterator->expected_value);
                        printf("sending email %s\n", event_list_iterator->email);

                        struct email *email_list_iterator = event_list_iterator->receiv_emails_list;

                        while ( email_list_iterator != NULL ){

                                printf("receiving_email %s\n", email_list_iterator->email_name);
                                email_list_iterator = email_list_iterator->next_email;
                        }

                        event_list_iterator = event_list_iterator->next_event;
                }

                printf("\n");

                topics = topics->next_topic;
        }
}

void print_topics(struct topic *topics){

        struct topic *topics_list_iterator = topics;

        while ( topics_list_iterator != NULL ){

                printf("topic %s\n", topics_list_iterator->name);
                topics_list_iterator = topics_list_iterator->next_topic;
        }
    
        printf("\n");
}

int send_matched_events_emails(struct event *topics_events, struct message *msg_info){

        //      find which topic was received

        struct topic *matchedTopic = get_topic_by_name(topics_events, msg_info->topic);

        if ( matchedTopic == NULL )
                return ENOMSG;

        //      loop all topic events

        struct event *matchedEvent = topic_message_matches_event(matchedTopic->ev_list, msg_info->msg);
        while ( matchedEvent != NULL ){

                //      if event is matched, send emails
                if ( send_emails(matchedEvent, msg_info) )
                        return ECOMM;
                
                matchedEvent = topic_message_matches_event(matchedEvent->next_event, msg_info->msg);
        }

        return 0;
}

struct event *topic_message_matches_event(struct event *matchedEvent, const char *msg){


        struct json_object *jobj = NULL;
        
        jobj = json_tokener_parse(msg);

        if ( jobj == NULL )
                return NULL;

        //else
	        //printf("jobj from str:\n---\n%s\n---\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));

        // it needed ?
        struct event *iterator = matchedEvent;

        while ( iterator != NULL ){

                struct json_object* param = json_object_object_get(jobj, iterator->parameter);

                if ( param == NULL ){

                        return NULL;
                }

                enum json_type param_type = json_object_get_type(param);

                if ( param_type == NULL ){

                        return NULL;
                }

                if ( (iterator->isDigit && (param_type == json_type_int || param_type == json_type_double)) ||
                !iterator->isDigit && json_type_string ){

                        if (compare_values(json_object_get_string(param), iterator->expected_value, iterator->comparison))
                                return iterator;
                }
/*
                else
                        printf("type dont match\n");*/

                iterator = iterator->next_event;
        }

        return NULL;
}

static int compare_values(const char *msg, const char *expected_value, const char *comparison){
/*
        if (msg[0] == '"'){

                printf("string quotes\n");
        }*/

        if ( strcmp(comparison, "==") == 0 ){

                if ( strcmp(msg, expected_value) == 0 )
                        return 1;
        }

        else if ( strcmp(comparison, "<") == 0){

                if ( strcmp(msg, expected_value) < 0 )
                        return 1;
        }

        else if ( strcmp(comparison, ">") == 0){

                if ( strcmp(msg, expected_value) > 0 )
                        return 1;
        }

        else if ( strcmp(comparison, "!=") == 0){

                if ( strcmp(msg, expected_value))
                        return 1;
        }

        else if ( strcmp(comparison, "<=") == 0){

                if ( strcmp(msg, expected_value) <= 0 )
                        return 1;
        }

        else if ( strcmp(comparison, ">=") == 0){

                if ( strcmp(msg, expected_value) >= 0 )
                        return 1;
        }

        return 0;
}
