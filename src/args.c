#include "args.h"
#include <string.h>
#include <stdlib.h>  

const char *argp_program_version =
  "logs";
const char *argp_program_bug_address =
  "<log@logs.org>";

char doc[] =
  "a program with options to print logs";

char args_doc[] = "ARG1 ARG2";

struct argp_option options[] = {
  {"host",   'h', "host", 0,
   "host adress" },
  {"port",   'p', "port", 0,
   "port number" },
  {"topic",   't', "topic", 0,
   "topic to subscribe" },
   {0}
};

struct argp argp = { options, parse_opt, NULL, doc };


error_t
parse_opt (int key, char *arg, struct argp_state *state)
{

  struct arguments *arguments = state->input;

  switch (key)
    {
        case 'h':
                arguments->host = arg;
        break;
        case 'p':
                arguments->port = atoi(arg);
        break;

        default:
                return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

int args_parse(int argc, char *argv[], struct arguments *args){

        int rc  = argp_parse (&argp, argc, argv, ARGP_NO_EXIT, 0, args);

        const char *empty = "-"; 

        if ( !strcmp(args->host, empty) || args->port == 0 )
                rc = EINVAL;

        return rc;
}