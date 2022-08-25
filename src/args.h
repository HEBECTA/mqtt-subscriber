#ifndef ARGS_H
#define ARGS_H

#include <argp.h>

const char *argp_program_version;
const char *argp_program_bug_address;

extern char doc[];

extern char args_doc[];

extern struct argp_option options[];

struct arguments
{
  char *host;
  int port;
};

error_t
parse_opt (int key, char *arg, struct argp_state *state);

struct argp argp;

int args_parse(int argc, char *argv[], struct arguments *args);

#endif