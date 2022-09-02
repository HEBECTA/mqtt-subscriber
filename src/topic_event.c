#include "topic_event.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct topic *get_topic_id(struct topic *topics, const char *topic_name){

        while (topics != NULL ){

                if ( strcmp(topics->name, topic_name) == 0 )
                        return topics;

                topics = topics->next_topic;
        }

        return NULL;
}

void free_topics_events(struct topic *topics){

        if ( *topics != NULL ){

                struct topic *iterator = *topics;
                struct topic *prev_node;

                while ( iterator->next_topic != NULL ){

                        prev_node = iterator;
                        iterator = iterator->next_topic;
                        free_event_list(prev_node->ev_list);
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

                for (int k = 0; k < events->receivers; ++k)
                        free(events->receiving_emails[k]);

                free(events->receiving_emails);

                events = events->next_event;
                free(events); // ??????
        }
}

void print_events(struct topic *topics){

        while ( topics != NULL ){

                printf("topic %s\n\n", topics->name);

                struct event *iterator = topics->ev_list;

                while ( iterator!= NULL ){

                        if ( strlen(iterator->parameter) )
                                printf("parameter %s\n", iterator->parameter);
                        printf("isDigit %d\n", iterator->isDigit);
                        printf("comparison %s\n", iterator->comparison);
                        printf("expected_value %s\n", iterator->expected_value);
                        printf("email %s\n", iterator->email);
                        printf("receivers %d\n", iterator->receivers);

                        for (int o = 0; o < iterator->receivers; ++o)
                                printf("receiving_emails %s\n", iterator->receiving_emails[o]);

                        iterator = iterator->next_event;
                }

                printf("\n");

                topics = topics->next_topic;
        }
}

void print_topics(struct topic *topics){

        while ( topics != NULL ){

                printf("topic %s\n", topics->name);
                topics = topics->next_topic;
        }
    
        printf("\n");
}

struct event *topic_message_matches_event(struct event *event, const char *msg){

        // it needed ?
        struct event *iterator = event;

        while ( iterator != NULL ){

                char *val = msg;

                if ( strlen(iterator->parameter) )
                        val = parameter(msg, iterator->parameter);

                if ( val == NULL ){

                       iterator = iterator->next_event;
                        continue; 
                }

                if ( isNumber(val) == iterator->isDigit ){

                        if ( compare_values(val, iterator->expected_value, iterator->comparison) )
                                return iterator;
                }

                iterator = iterator->next_event;
        }

        return NULL;
}

static int isNumber(const char *str){

        for (int i = 0; str[i]!= '\0'; i++){

                if (isdigit(str[i]) == 0)
                        return 0;
        }

        return 1;
}

static char *parameter(const char *msg, const char *parameter){

        char *param_ptr = strstr(msg, parameter);

        if (param_ptr == NULL)
                return NULL;

	while ( *param_ptr != ':' ){

                if ( *param_ptr == '\0' )
                        return NULL;

                param_ptr += sizeof(char);
        }
                
        if ( *param_ptr == '\0' )
                return NULL;

        param_ptr += sizeof(char);

	while ( *param_ptr == ' ' || *param_ptr == '\t' ){

                if ( *param_ptr == '\0' )
                        return NULL;

                param_ptr += sizeof(char);
        }

	int single_quote = 0;

	if ( *param_ptr == '\''){

		single_quote = 1;

		param_ptr += sizeof(char);

		if ( *param_ptr == '\0' )
                	return NULL;
	}

	char *end = param_ptr;
	int size = 0;
	
	if ( single_quote ){

		while ( *end != '\'' ){

			end += sizeof(char);
			++size;

			if ( *end == '\0' )
				return NULL;
		}
	}

	else {

		while ( *end != ' ' && *end != '\t' && *end != '}'){
	
			end += sizeof(char);
			++size;

			if ( *end == '\0' )
				return NULL;
		}
	}

        char *param = (char *) malloc(sizeof(char) * size + 1);

        if ( param == NULL )
                return NULL;

        strncpy(param, param_ptr, size);

        param[size] = '\0';

        return param;
}

static int compare_values(const char *msg, const char *expected_value, const char *comparison){

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
