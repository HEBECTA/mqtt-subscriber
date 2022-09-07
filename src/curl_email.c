#include "curl_email.h"

#include <stdio.h>
#include <stdlib.h>
#include "uci_data.h"


static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  const char *data;
  size_t room = size * nmemb;
 
  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }
 
  //data = &payload_text[upload_ctx->bytes_read];
  data = &upload_ctx->msg[upload_ctx->bytes_read];
 
  if(data) {
    size_t len = strlen(data);
    if(room < len)
      len = room;
    memcpy(ptr, data, len);
    upload_ctx->bytes_read += len;
 
    return len;
  }
 
  return 0;
}

int send_email(struct smtp_info *sender_info, const char *receiver, const char *subject, const char *message)
{ 
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  char *email_message = NULL;

  curl = curl_easy_init();

  if(curl) {

    curl_easy_setopt(curl, CURLOPT_USERNAME, sender_info->username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, sender_info->password);

    curl_easy_setopt(curl, CURLOPT_URL, sender_info->smtp_domain);

    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

    curl_easy_setopt(curl, CURLOPT_CAINFO, CERTIFICATE_PATH);

    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, sender_info->sending_email);

    recipients = curl_slist_append(recipients, receiver);

    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    email_message = (char *) malloc(sizeof(char) * (strlen(message) + 
    strlen(receiver) + strlen(sender_info->sending_email) + strlen(subject) + 40));

    if ( email_message == NULL ){

        res = CURLE_FAILED_INIT;
        goto EXIT_SEND_EMAIL;
    }

    sprintf(email_message,
    "To: <%s> \r\n"
    "From: <%s> \r\n"
    "Subject: %s\r\n"
    "\r\n" 
    "%s\r\n"
    "\r\n"
    , receiver, sender_info->sending_email, subject, message);

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);

    struct upload_status upload_data;
    upload_data.msg = email_message;
    upload_data.bytes_read = 0;

    curl_easy_setopt(curl, CURLOPT_READDATA, (void *)&upload_data);

    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
 
    res = curl_easy_perform(curl);
 
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
EXIT_SEND_EMAIL:

    if ( email_message != NULL )
      free(email_message);

    if ( recipients != NULL )
      curl_slist_free_all(recipients);
 

    curl_easy_cleanup(curl);
  }

  else
    res = CURLE_FAILED_INIT;
 
  return (int)res;
}

void free_smtp_info(struct smtp_info *smtp_info){

  if ( smtp_info->sending_email != NULL )
    free(smtp_info->sending_email);
  if ( smtp_info->smtp_domain != NULL )
    free(smtp_info->smtp_domain);
  if ( smtp_info->username != NULL )
    free(smtp_info->username);
  if ( smtp_info->password != NULL )
    free(smtp_info->password);

  free(smtp_info);
}
