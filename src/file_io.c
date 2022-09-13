#include "file_io.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int callback(void *data, int argc, char **argv, char **azColName){
        
   for(int i = 0; i<argc; i++)
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   
   printf("\n");

   return 0;
}

void set_topic(Topic *ptr_tp, const char *tp, const char *msg, time_t date){

        strncpy(ptr_tp->message, msg, TOPIC_MESSAGE_BUFFER_SIZE);

        strncpy(ptr_tp->topic, tp, TOPIC_NAME_MAX_SIZE);

        ptr_tp->date = date;
}

int open_file(){

        int rc = sqlite3_open(topics_file, &db);
    
        if (rc != SQLITE_OK) {
                
                //fprintf(stderr, "open_log cannot open log file: %s\n", sqlite3_errmsg(db));
                sqlite3_close(db);
                
                return 1;
        }

        return 0;
}

void close_file(){

        sqlite3_close(db);
}

int write_file(const Topic *ptr_tp){

        char *err_msg = 0;

        int rc = sqlite3_exec(db, sql_init, 0, 0, &err_msg);

        if (rc != SQLITE_OK ) {
                
                //fprintf(stderr, "Failed to create table\n");
                //fprintf(stderr, "SQL error: %s\n", err_msg);
                sqlite3_free(err_msg);
        } 

        char sql[TOPIC_MESSAGE_BUFFER_SIZE+TOPIC_NAME_MAX_SIZE+100];

        sprintf(sql, "INSERT INTO TOPICS_DATA (Topic, Message, Date) "  \
         "VALUES (\"%s\", \"%s\", \"%s\");", ptr_tp->topic, ptr_tp->message, \
         ctime(&ptr_tp->date));
     
        rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
        
        if (rc != SQLITE_OK ) {
                
                //fprintf(stderr, "Failed to write into table\n");
                //fprintf(stderr, "SQL error: %s\n", err_msg);
                //sqlite3_free(err_msg);
        } 
        
        return 0;
}

int print_topic(const char *topic_name){

        char *zErrMsg = 0;

        char sql[TOPIC_NAME_MAX_SIZE+20];

        if (topic_name == NULL )
                strcpy(sql, "SELECT * from TOPICS_DATA;");
        

        else 
                sprintf(sql, "SELECT * from TOPICS_DATA WHERE Topic = \"%s\";", topic_name);
               
        int rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

        if( rc != SQLITE_OK ) {

                //fprintf(stderr, "print_log: SQL error: %s\n", zErrMsg);
                //sqlite3_free(zErrMsg);
                return 1;
        }

        return 0;
}

int delete_topic(const char *topic_name){

        

        return 0;
}

int write_topic_to_file(struct message *msg_info){

        Topic topic;
        time_t date = time(0); 
        set_topic(&topic, msg_info->topic, msg_info->msg, date);

        write_file(&topic);

        return 0;
}
