#include "file_io.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>


int open_file(){

        int rc = sqlite3_open(topics_file, &db);
    
        if (rc != SQLITE_OK) {
                
                syslog("MQTT: failed to open file of received topics: %s\n", sqlite3_errmsg(db));
                sqlite3_close(db);
        }

        return rc;
}

void close_file(){

        sqlite3_close(db);
}


int write_topic_to_file(struct message *msg_info){

        int rc = sqlite3_exec(db, sql_init, 0, 0, NULL);

        if (rc != SQLITE_OK )
                return rc;

        char sql[MQTT_MSG_SIZE+MQTT_TOPIC_NAME_SIZE+100];

        time_t date = time(0); 
        sprintf(sql, "INSERT INTO TOPICS_DATA (Topic, Message, Date) "  \
         "VALUES (\"%s\", \"%s\", \"%s\");", msg_info->topic, msg_info->msg, \
         ctime(&date));
     
        rc = sqlite3_exec(db, sql, 0, 0, NULL);
        
        return rc;
}
