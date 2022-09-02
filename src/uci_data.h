#ifndef UCI_DATA_H
#define UCI_DATA_H
#include <uci.h>
#include "topic_event.h"
#include "curl_email.h"

#define MAX_NAME_SIZE 50
#define MAX_NAME_NMB 50

#define CONFIG_PATH "/etc/config/"
//#define CONFIG_FILE "subscriber"
#define TOPIC_CONFIG_SECTION "subscriber_info"
#define TOPIC_OPTION "topic"
#define EVENT_CONFIG_SECTION "event"


static struct uci_context* init_uci(const char *file, struct uci_package **pkg);

bool event_belongs_to_topic(struct uci_section *s, const char *topic);

int scan_topics_events(struct topic **topics);

static int scan_events(struct uci_context *ctx, struct uci_package *pkg, struct topic *topics);

struct smtp_info *scan_email(const char *user_group);

static int get_events_nmb(struct uci_package *pkg);

//int write_topics();

int write_events();



#endif