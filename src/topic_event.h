#ifndef TOPIC_EVENT_H
#define TOPIC_EVENT_H


#include <stdbool.h>

#include "mqtt_sub.h"

#define MAX_TOPIC_NAME_LEN 100


struct email {

        char *email_name;
        struct email *next_email;
};

struct event {

        char *parameter;
        bool isDigit;
        char comparison[3]; 
        char *expected_value;
        char *email;
        struct email *receiv_emails_list;

        struct event *next_event;
};

struct topic {

        char *name;
        struct event *ev_list;
        struct topic *next_topic;
};

void print_events(struct topic *topics);

void print_topics(struct topic *topics);

void free_topics_events(struct topic *topics);

static void free_event_list(struct event *events);

static void free_email_list(struct email *emails);

struct topic *get_topic_by_name(struct topic *topics, const char *topic_name);

int send_matched_events_emails(struct event *topics_events, struct message *msg_info);

struct event *topic_message_matches_event(struct event *matchedEvent, const char *msg);

static int isNumber(const char *str);

static char *parameter(const char *msg, const char *parameter);

static int compare_values(const char *msg, const char *expected_value, const char *comparison);

#endif