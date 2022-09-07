#ifndef FILE_IO_H
#define FILE_IO_H

#define TOPIC_MESSAGE_BUFFER_SIZE 250
#define TOPIC_NAME_MAX_SIZE 100

#include <stdio.h>
#include <time.h>
#include <sqlite3.h>

typedef enum QoS {QoS0, QoS1, QoS2} QoS;

// log ? usr/topics ??
static const char topics_file[] = "/log/mqtt_topics_data.db";

static const char *sql_init = "CREATE TABLE if not exists TOPICS_DATA ("
        "ID                     INTEGER PRIMARY KEY,"
        "Topic                  INT,"
        "Message                TEXT,"
        "QoS                    INT,"
        "Date                   TEXT);";

// rename Message
typedef struct Topic_sql {

        char topic[TOPIC_NAME_MAX_SIZE];
        char message[TOPIC_MESSAGE_BUFFER_SIZE];
        enum QoS qoS;
        time_t date;
} Topic;

void set_topic(Topic *ptr_tp, const char *tp, const char *msg, QoS qoS, time_t date);

int open_file();

void close_file();

// pointer param ???
int write_file(const Topic *ptr_tp);

int print_topic(const char *topic_name);

int delete_topic(const char *topic_name);

static sqlite3 *db;


#endif