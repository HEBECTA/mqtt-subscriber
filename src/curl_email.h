#ifndef CURL_EMAIL_H
#define CURL_EMAIL_H

#include <stdio.h>
#include <string.h>
//#include <curl/curl.h>
 
struct smtp_info {

  char *sending_email;
  char *smtp_domain;
  char *username;
  char *password;
  int port;
};
/*
static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp);

struct upload_status {
  size_t bytes_read;
  char *msg;
};

int send_email(const char *user_group, const char *receiver, const char *subject, const char *message);
*/
#endif