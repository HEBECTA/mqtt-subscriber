#ifndef UCI_DATA_H
#define UCI_DATA_H
#include <uci.h>

#define MAX_NAME_SIZE 50
#define MAX_NAME_NMB 50

#define NO_SUCH_TOPIC -1

struct event {

        int topic_id;
        char *parameter;
        bool isDigit;
        char comparison[2];
        char *expected_value;
        char *email;
        int receivers;
        char **receiving_emails;
};

struct events {

        struct event *events;
        int events_n;
};

struct uci_context* init_uci(const char *file, struct uci_package **pkg);

bool event_belongs_to_topic(struct uci_section *s, const char *topic);

int scan_topics(struct uci_context *ctx, struct uci_package *pkg, char ***topics, int *topics_nmb);

int scan_events(struct uci_context *ctx, struct uci_package *pkg, struct event **evs, int *events_nmb, char **topics, int topics_nmb);

static int get_events_nmb(struct uci_package *pkg);

int write_topics();

int write_events();

void free_topics(char **topics, int topics_nmb);

void free_events(struct event *events, int events_nmb);

int get_topic_id(char **topics, char *topic, int topics_nmb);

void print_events(struct event *events, int events_nmb);

#endif