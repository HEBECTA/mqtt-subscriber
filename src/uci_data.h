#ifndef UCI_DATA_H
#define UCI_DATA_H
#include <uci.h>
#include "topic_event.h"
#include "curl_email.h"

#define CONFIG_PATH "/etc/config/"
#define TOPIC_EVENT_CONFIG_FILE "subscriber"
#define USER_GROUPS_CONFIG_FILE "user_groups"
#define TOPIC_CONFIG_SECTION "subscriber_info"
#define USER_GROUPS_SECTION "email"
#define TOPIC_OPTION "topic"
#define EVENT_CONFIG_SECTION "event"
#define EVENT_PARAMETER_OPTION "parameter"
#define EVENT_VALUE_TYPE_OPTION "value_type"
#define EVENT_COMPARISON_OPTION "comparison"
#define EVENT_VALUE_OPTION "value"
#define EVENT_EMAIL_OPTION "emailgroup"
#define EVENT_REC_EMAIL_OPTION "recipEmail"

#define EMAIL_SMTP_PORT_OPTION "smtp_port"
#define EMAIL_SMTP_IP_OPTION "smtp_ip"
#define EMAIL_OPTION "senderemail"
#define EMAIL_CREDENTIALS_OPTION "credentials"
#define EMAIL_USERNAME_OPTION "username"
#define EMAIL_PASSWORD_OPTION "password"


static struct uci_context* init_uci(const char *file, struct uci_package **pkg);

bool event_belongs_to_topic(struct uci_section *s, const char *topic);

int scan_topics_events(struct topic **topics);

static int scan_events(struct uci_context *ctx, struct uci_package *pkg, struct topic *topics);

struct smtp_info *scan_email(const char *user_group);

static int get_events_nmb(struct uci_package *pkg);

//int write_topics();

int write_events();



#endif