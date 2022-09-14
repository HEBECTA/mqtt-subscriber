#ifndef FILE_IO_H
#define FILE_IO_H

#define TOPIC_MESSAGE_BUFFER_SIZE 250
#define TOPIC_NAME_MAX_SIZE 100

#include <stdio.h>
#include <time.h>
#include <sqlite3.h>
#include "mqtt_sub.h"

static const char topics_file[] = "/log/mqtt_topics_data.db";

static const char *sql_init = "CREATE TABLE if not exists TOPICS_DATA ("
        "ID                     INTEGER PRIMARY KEY,"
        "Topic                  INT,"
        "Message                TEXT,"
        "Date                   TEXT);";


int open_file();

void close_file();

int write_topic_to_file(struct message *msg_info);

static sqlite3 *db;

#endif