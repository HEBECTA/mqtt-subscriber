#ifndef CURL_EMAIL_H
#define CURL_EMAIL_H

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#define CERTIFICATE_PATH "/etc/ssl/certs/ca-certificates.crt"
 
struct smtp_info {

  char *sending_email;
  char *smtp_domain;
  char *username;
  char *password;
};

static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp);

struct upload_status {
  size_t bytes_read;
  char *msg;
};

void free_smtp_info(struct smtp_info *smtp_info);

int send_email(struct smtp_info *sender_info, const char *receiver, const char *subject, const char *message);

#endif