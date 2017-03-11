/**
 * @file
 * Simple argument parser library implementation.
 * See argparse.h for API specification.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "libutils/error.h"
#include "libutils/list.h"
#include "libutils/argparse.h"
#include "libutils/log.h"

/* logger */
static log_handle(_logger);
static log_handle(_stdlogger);
static bool argparse_initialized = false;
#define logger &_logger
#define stdlogger &_stdlogger

struct argparse_item;
struct argparse_option;
struct argparse_handle;

/* item of the parsed list of args */
struct argparse_item {
  struct argparse_option *opt;
  union {
    long int int_arg;
    char *str_arg;
  };
};

struct argparse_option {
  bool required;
  char *name;
  char shortname;
  argtype_t type;
  char *help;
};

struct argparse_option_input {
  bool required;
  const char *name;
  char shortname;
  argtype_t type;
  const char *help;
};

struct argparse_handle {
  char *subcommand_name; /* name of this subcommand */
  char *subcommand_help; /* help string for the subcommand */
  list_t subcommands; /* nested subcommands */
  list_t args; /* parsed named args */
  list_t pos_args; /* parsed positional args */
  list_t options; /* named arguments defined */
  list_t pos_options; /* positional arguements defined */
  int curr_posarg; /* last positional argument found */
  char *bin_name; /* name of the executable extracted from argv[0] */
  argconsumer_t cbk; /* post-parsing callback */
  void *userdata; /* user-defined callback arguments */
};

/*
 * Parser state machine internal state
 */
struct argparse_parser_state {
  /* argument vector to parse */
  char **argv;
  /* expected number of arguments */
  int argc;
  /* index of the current argument */
  int index;
  /* stack of subcommands that are taken */
  list_t subcmd_stack;
  /* current subcommand */
  struct argparse_handle *current_cmd;
  /* signal that the last subcommand has been reached */
  bool last_subcmd;
  /* root of the parser tree */
  struct argparse_handle *root_cmd;
};

#define HELP_INDENT_BLOCK "\t"

/* static functions */
static int argparse_subcommand_ctor(void **itmdata, void *data);
static int argparse_subcommand_dtor(void *data);
static int argparse_options_ctor(void **itmdata, void *data);
static int argparse_options_dtor(void *data);
static int argparse_item_dtor(void *data);

static void argparse_help_subcommand(struct argparse_handle *ap,
				     int nest_level);
static void argparse_copyout_arg(struct argparse_item *item,
				 void *buf, int bufsize);
static int argparse_parseopt(struct argparse_item *item, char *arg);
static int argparse_parse_posarg(struct argparse_parser_state *st);
static int argparse_parse_arg_named(struct argparse_parser_state *st);
static int argparse_check_required(struct argparse_parser_state *st);
static int argparse_fixup_flags(struct argparse_parser_state *st);
static int argparse_next_subcmd(struct argparse_parser_state *st);
static int argparse_prev_subcmd(struct argparse_parser_state *st);
static int argparse_finalize_subcmd(struct argparse_parser_state *st);
static int argparse_finalize_pop_subcmd(struct argparse_parser_state *st);

/**
 * Subcommand constructor.
 * Constraints:
 * - input subcommand struct is heap allocated
 * - ownership of the input allocation is taken by the subcommand
 * handling, *data must not be freed while in the subcommand list
 */
static int
argparse_subcommand_ctor(void **itmdata, void *data)
{
  struct argparse_handle *sub = (struct argparse_handle *)data;
  int error;

  error = list_init(&sub->subcommands, argparse_subcommand_ctor,
		    argparse_subcommand_dtor);
  if (error == UTILS_ERROR)
    goto err_sub;
  error = list_init(&sub->args, NULL, argparse_item_dtor);
  if (error == UTILS_ERROR)
    goto err_args;
  error = list_init(&sub->pos_args, NULL, argparse_item_dtor);
  if (error == UTILS_ERROR)
    goto err_posargs;
  error = list_init(&sub->options, argparse_options_ctor,
		    argparse_options_dtor);
  if (error == UTILS_ERROR)
    goto err_options;
  error = list_init(&sub->pos_options, argparse_options_ctor,
		    argparse_options_dtor);
  if (error == UTILS_ERROR)
    goto err_posopts;
  sub->curr_posarg = 0;
  sub->bin_name = NULL;
  *itmdata = sub;
  return ARGPARSE_OK;
  
 err_posopts:
  list_destroy(sub->options);
 err_options:
  list_destroy(sub->pos_args);
 err_posargs:
  list_destroy(sub->args);
 err_args:
  list_destroy(sub->subcommands);
 err_sub:
  return ARGPARSE_ERROR;
}

/**
 * Destructor for the subcommands list
 */
static int
argparse_subcommand_dtor(void *data)
{
  struct argparse_handle *sub = (struct argparse_handle *)data;
  bool have_error;
  int error;

  /* NOTE: sub->subcommands is destroyed by argparse_destroy_subcmd */
  xlog_debug(logger, "Destroy %s\n", sub->subcommand_name);
  error = list_destroy(sub->args);
  if (error != UTILS_OK)
    have_error = true;
  error = list_destroy(sub->pos_args);
  if (error != UTILS_OK)
    have_error = true;
  error = list_destroy(sub->options);
  if (error != UTILS_OK)
    have_error = true;
  error = list_destroy(sub->pos_options);
  if (error != UTILS_OK)
    have_error = true;
  error = list_destroy(sub->subcommands);
  if (error != UTILS_OK)
    have_error = true;
  if (sub->subcommand_name != NULL)
    free(sub->subcommand_name);
  if (sub->subcommand_help != NULL)
    free(sub->subcommand_help);
  /* NOTE bin_name is assumed to come from argv, so
   * do not deallocate.
   */
  if (have_error)
    return UTILS_ERROR;
  free(sub);
  return UTILS_OK;
}

/**
 * Option list constructor and destructors
 * Constraints:
 * - ownership of the input data is not taken by the list
 * the caller should deallocate *data after it is consumed by the ctor
 */
static int
argparse_options_ctor(void **itmdata, void *data)
{
  struct argparse_option_input *opt = data;
  struct argparse_option *opt_item;

  opt_item = malloc(sizeof(struct argparse_option));
  if (opt_item == NULL)
    return UTILS_ERROR;
  /* copy non-pointer fields */
  opt_item->type = opt->type;
  opt_item->shortname = opt->shortname;
  opt_item->required = opt->required;

  opt_item->name = malloc(strlen(opt->name) + 1);
  if (opt_item->name == NULL)
    goto err_name;
  strcpy(opt_item->name, opt->name);

  opt_item->help = malloc(strlen(opt->help) + 1);
  if (opt_item->help == NULL)
    goto err_help;
  strcpy(opt_item->help, opt->help);

  *itmdata = opt_item;
  return UTILS_OK;
  
 err_help:  
  free(opt_item->name);
 err_name:
  free(opt_item);
  return UTILS_ERROR;
}

/**
 * Destructor for options and pos_options lists
 */
static int
argparse_options_dtor(void *data)
{
  struct argparse_option *opt = data;
  
  if (opt->name)
    free(opt->name);
  if (opt->help)
    free(opt->help);
  free(data);
  return ARGPARSE_OK;
}

/**
 * Destructor for args and pos_args lists
 */
static int
argparse_item_dtor(void *data)
{
  struct argparse_item *itm = data;
  if (itm->opt->type == T_STRING && itm->str_arg)
    free(itm->str_arg);
  free(data);
  return ARGPARSE_OK;
}

/**
 * Display help for a subcommand recursively
 * 
 * @param[in] ap: the subcommand to show
 * @param[in] nest_level: indentation level
 */
static void
argparse_help_subcommand(struct argparse_handle *ap, int nest_level)
{
  list_iter_struct_t iter;
  struct argparse_option *opt;
  struct argparse_handle *sub;
  int i;

#define PRINT_INDENT(n)				\
  for (i = 0; i < n; i++)			\
    xlog_msg(stdlogger, HELP_INDENT_BLOCK)

  PRINT_INDENT(nest_level);

  if (ap->subcommand_name == NULL)
    /* root parser */
    xlog_msg(stdlogger, "Usage: %s [options] [subcommands] [arguments]\n",
	     ap->bin_name);
  else {
    xlog_msg(stdlogger, "%s [options] [subcommands] [arguments]\n",
	     ap->subcommand_name);
    PRINT_INDENT(nest_level);
    xlog_msg(stdlogger, "%s\n", ap->subcommand_help);
  }
  

    PRINT_INDENT(nest_level);
    xlog_msg(stdlogger, "Options:\n");
    for (list_iter_init(ap->options, &iter); ! list_iter_end(&iter);
	 list_iter_next(&iter)) {
      opt = list_iter_data(&iter);
      PRINT_INDENT(nest_level + 1);
      if (opt->shortname == '\0')
	xlog_msg(stdlogger, "--%s\t\t%s\n", opt->name, opt->help);
      else
	xlog_msg(stdlogger, "-%c,--%s\t\t%s\n", opt->shortname,
		 opt->name, opt->help);
    }
    xlog_msg(stdlogger, "\n");
    PRINT_INDENT(nest_level);
    xlog_msg(stdlogger, "Arguments:\n");
    for (list_iter_init(ap->pos_options, &iter); ! list_iter_end(&iter);
	 list_iter_next(&iter)) {
      opt = list_iter_data(&iter);
      PRINT_INDENT(nest_level + 1);
      xlog_msg(stdlogger, "%s\t\t\t%s\n", opt->name, opt->help);
    }
    xlog_msg(stdlogger, "\n");
    PRINT_INDENT(nest_level);
    xlog_msg(stdlogger, "Subcommands:\n");
    for (list_iter_init(ap->subcommands, &iter); ! list_iter_end(&iter);
	 list_iter_next(&iter)) {
      sub = list_iter_data(&iter);
      argparse_help_subcommand(sub, nest_level + 1);
    }
    xlog_msg(stdlogger, "\n\n");
#undef PRINT_INDENT
}

/**
 * Display help for all subcommands given the
 * root subcommand. This prints command, subcommands,
 * options and arguments for each subcommand in the
 * subcommand tree
 */
void
argparse_helpmsg(argparse_t ap)
{
  argparse_help_subcommand(ap, 0);
}

/**
 * Initialise an argparse handle for a new parser
 */
int
argparse_init(argparse_t *pap, const char *help, argconsumer_t cbk,
	      void *userdata)
{
  int error;
  argparse_t ap;

  if (!argparse_initialized) {
    /* Initialize the logger if not set up already */
    log_init(logger, NULL);
    log_init(stdlogger, NULL);
#ifdef DEBUG
    log_option_set(logger, LOG_OPT_LEVEL, LOG_OPT_LEVEL_DEBUG);
#else /* ! DEBUG */
    log_option_set(logger, LOG_OPT_LEVEL, LOG_OPT_LEVEL_ERR);
#endif /* ! DEBUG */
    log_option_set(stdlogger, LOG_OPT_LEVEL, LOG_OPT_LEVEL_ERR);
    log_option_set(logger, LOG_OPT_PREFIX, "argparse");
    log_option_set(stdlogger, LOG_OPT_PREFIX, "");
    log_option_set(stdlogger, LOG_OPT_MSG_FMT, "%s%s");
  }

  if (pap == NULL)
    return ARGPARSE_ERROR;

  ap = malloc(sizeof(struct argparse_handle));
  if (ap == NULL)
    return ARGPARSE_ERROR;
  *pap = ap;

  /* use subcommand_ctor to do the common initialization */
  error = argparse_subcommand_ctor((void **)pap, ap);
  if (error)
    return ARGPARSE_ERROR;
  ap->cbk = cbk;
  ap->userdata = userdata;
  ap->subcommand_name = NULL; /* top-level argparser */
  ap->subcommand_help = malloc(strlen(help) + 1);
  if (ap->subcommand_help == NULL) {
    argparse_subcommand_dtor(ap);
    list_destroy(ap->subcommands);
    *pap = NULL;
    return ARGPARSE_ERROR;
  }
  strcpy(ap->subcommand_help, help);
  return ARGPARSE_OK;
}

/*
 * Deallocate a parser
 */
int
argparse_destroy(argparse_t ap)
{
  int error;

  assert(ap != NULL);

  xlog_debug(logger, "Destroy root %s\n", ap->subcommand_name);
  error = argparse_subcommand_dtor(ap);

  if (error == UTILS_ERROR)
    return ARGPARSE_ERROR;
  return ARGPARSE_OK;
}

/*
 * Reset parser state
 */
int
argparse_reset(argparse_t ap)
{
  int rc;
  list_iter_struct_t subcommands;
  struct argparse_handle *sub;

  assert(ap != NULL);
  xlog_debug(logger, "Reset parser %s\n", ap->subcommand_name);
  while (list_length(ap->args) > 0) {
    rc = list_delete(ap->args, 0);
    if (rc != ARGPARSE_OK) {
      xlog_err(logger, "Can not reset parsed arg %d\n",
	       list_length(ap->args));
      return ARGPARSE_ERROR;
    }
  }
  while (list_length(ap->pos_args) > 0) {
    rc = list_delete(ap->pos_args, 0);
    if (rc != ARGPARSE_OK) {
      xlog_err(logger, "Can not reset parsed posarg%d\n",
	       list_length(ap->pos_args));
      return ARGPARSE_ERROR;
    }
  }
  ap->curr_posarg = 0;


  for (list_iter_init(ap->subcommands, &subcommands);
       ! list_iter_end(&subcommands); list_iter_next(&subcommands)) {
    sub = list_iter_data(&subcommands);
    if (sub == NULL) {
      xlog_err(logger, "Invalid subcommand\n");
      return ARGPARSE_ERROR;
    }
    rc = argparse_reset(sub);
    if (rc == ARGPARSE_ERROR)
      return ARGPARSE_ERROR;
  }
  return ARGPARSE_OK;
}

/*
 * Create new subcommand
 */
argparse_t
argparse_subcmd_add(argparse_t ap, const char *name, const char *help,
		    argconsumer_t cbk, void *userdata)
{
  struct argparse_handle *sub;
  int error;

  assert(name != NULL);
  assert(help != NULL);

  sub = malloc(sizeof(struct argparse_handle));
  if (sub == NULL)
    return NULL;
  
  sub->subcommand_name = malloc(strlen(name) + 1);
  if (sub->subcommand_name == NULL)
    goto err_name;
  strcpy(sub->subcommand_name, name);
  
  sub->subcommand_help = malloc(strlen(help) + 1);
  if (sub->subcommand_help == NULL)
    goto err_help;
  strcpy(sub->subcommand_help, help);

  sub->cbk = cbk;
  sub->userdata = userdata;
  
  error = list_push(ap->subcommands, sub);
  if (error)
    goto err_help;
  
  return sub;

 err_help:
  free(sub->subcommand_name);
 err_name:
  free(sub);
  return NULL;
}

/**
 * Add argument option to the parser
 */
int
argparse_arg_add(argparse_t ap, const char *name, char shortname,
		 argtype_t type, const char *help, bool required)
{
  struct argparse_option_input opt;
  int error;

  assert(ap != NULL);

  opt.name = name;
  opt.help = help;
  opt.shortname = shortname;
  opt.type = type;
  opt.required = required;

  error = list_push(ap->options, &opt);
  return error;
}

/**
 * Add positional argument to the parser
 * XXX: the argument type should be treated differently when the type is flag
 * or should flags be disallowed for posargs (disallow for now)
 */
int
argparse_posarg_add(argparse_t ap, const char *name,
		    argtype_t type, const char *help)
{
  struct argparse_option_input opt;
  int error;

  assert(ap != NULL);

  if (type == T_FLAG)
    return -1;
  
  opt.name = name;
  opt.help = help;
  opt.shortname = '\0';
  opt.type = type;
  opt.required = true;

  error = list_append(ap->pos_options, &opt);
  return (error);
}

/**
 * copy argument from internal structure to user-defined buffer
 */
static void
argparse_copyout_arg(struct argparse_item *item, void *buf, int bufsize)
{
  switch (item->opt->type) {
  case T_STRING:
    strncpy((char *)buf, item->str_arg, bufsize);
    // force last buffer byte to be null
    ((char *)buf)[bufsize - 1] = '\0';
    break;
  case T_INT:
    *(int *)buf = item->int_arg;
    break;
  case T_FLAG:
    if (item->int_arg == 0)
      *(bool *)buf = false;
    else
      *(bool *)buf = true;
    break;
  }
}

/**
 * Get value of option with given name
 */
int
argparse_arg_get(argparse_t ap, const char *name, void *arg, int bufsize)
{
  list_iter_struct_t iter;
  struct argparse_item *itm;

  assert(ap != NULL);

  for (list_iter_init(ap->args, &iter); !list_iter_end(&iter);
       list_iter_next(&iter)) {
    itm = list_iter_data(&iter);
    if (itm->opt == NULL)
      return ARGPARSE_ERROR;
    
    if (strcmp(itm->opt->name, name) == 0)
      break;
  }
  if (list_iter_end(&iter))
    return ARGPARSE_NOARG;
  argparse_copyout_arg(itm, arg, bufsize);
  return ARGPARSE_OK;
}

/**
 * Get n-th positional argument
 */
int
argparse_posarg_get(argparse_t ap, int idx, void *arg, int bufsize)
{
  struct argparse_item *item;

  assert(ap != NULL);

  item = list_get(ap->pos_args, idx);
  if (item == NULL)
    return ARGPARSE_NOARG;

  argparse_copyout_arg(item, arg, bufsize);
  return ARGPARSE_OK;
}

/**
 * Genric parse function, generates an argument item given
 * a properly initialised argparse_item from an argparse_option
 * the type and opt fields must be set
 */
static int
argparse_parseopt(struct argparse_item *item, char *arg)
{
  int error = 0;
  char *int_arg_end = NULL;
  
  switch(item->opt->type) {
  case T_STRING:
    item->str_arg = malloc(strlen(arg) + 1);
    if (item->str_arg == NULL) {
      error = ARGPARSE_ERROR;
      break;
    }
    strcpy(item->str_arg, arg);    
    break;
  case T_INT:
    item->int_arg = strtol(arg, &int_arg_end, 10);
    if (int_arg_end == arg) {
      xlog_err(logger, "Invalid type of %s for argument %s (int)\n",
	       arg, item->opt->name);
      error = ARGPARSE_ERROR;
    }
    break;
  case T_FLAG:
    item->int_arg = 1;
    break;
  default:
    error = ARGPARSE_ERROR;
  }
  return (error);
}

static int
argparse_parse_posarg(struct argparse_parser_state *st)
{
  struct argparse_item *item;
  struct argparse_option *opt;
  int error;
  char *arg;
  struct argparse_handle *ap = st->current_cmd;

  arg = st->argv[st->index++];
  opt = list_get(ap->pos_options, ap->curr_posarg);

  if (opt == NULL) {
    xlog_err(logger, "Extra positional argument %s\n", arg);
    argparse_helpmsg(st->root_cmd);
    return ARGPARSE_ERROR;
  }
  
  item = malloc(sizeof(struct argparse_item));
  if (item == NULL)
    return ARGPARSE_ERROR;
  item->opt = opt;

  error = argparse_parseopt(item, arg);
  if (error) {
    argparse_helpmsg(st->root_cmd);
    goto err_parse;
  }
  error = list_append(ap->pos_args, item);
  if (error)
    goto err_push;

  ap->curr_posarg += 1;
  return ARGPARSE_OK;
  
 err_parse:
 err_push:
  free(item);
  return (error);
}

/**
 * Parse a named option argument.
 * The argument may have a long or short name and may
 * have a value or be flag.
 */
static int
argparse_parse_arg_named(struct argparse_parser_state *st)
{
  struct argparse_item *item;
  struct argparse_option *opt;
  list_iter_struct_t iter;
  int error, is_longopt;
  char *arg;
  struct argparse_handle *ap = st->current_cmd;

  /* remove leading '-' from option */
  arg = st->argv[st->index++] + 1;
  /* check for long option */
  if (*arg == '-') {
    arg += 1;
    is_longopt = 1;
  }
  else
    is_longopt = 0;

  if (*arg == '\0') {
    xlog_err(logger, "Invalid option -\n");
    argparse_helpmsg(st->root_cmd);
    return ARGPARSE_ERROR;
  }

  for (list_iter_init(ap->options, &iter); !list_iter_end(&iter);
       list_iter_next(&iter)) {
    opt = list_iter_data(&iter);
    if (is_longopt) {
      if (strcmp(opt->name, arg) == 0)
	break;
    }
    else {
      if (opt->shortname == *arg)
	break;
    }
  }
  if (list_iter_end(&iter)) {
    xlog_err(logger, "Invalid option -%s\n", arg);
    argparse_helpmsg(st->root_cmd);
    return ARGPARSE_ERROR;
  }

  item = malloc(sizeof(struct argparse_item));
  if (item == NULL)
    return ARGPARSE_ERROR;
  item->opt = opt;

  /* get next argument, holding the value for the option being parsed */
  if (item->opt->type == T_FLAG) {
    arg = NULL;
  }
  else {
    if (st->index >= st->argc) {
      xlog_err(logger, "Expected value for --%s\n", item->opt->name);
      argparse_helpmsg(st->root_cmd);
      goto err_parse;
    }
    arg = st->argv[st->index++];
  }

  error = argparse_parseopt(item, arg);
  if (error) {
    argparse_helpmsg(st->root_cmd);
    goto err_parse;
  }
      
  error = list_push(ap->args, item);
  if (error)
    goto err_push;

  return ARGPARSE_OK;
  
 err_parse:
 err_push:
  free(item);
  return ARGPARSE_ERROR;
}

/**
 * Check that all required arguments have been specified
 * for the current parser
 */
static int
argparse_check_required(struct argparse_parser_state *st)
{
  int num_opts;
  list_iter_struct_t iter_args;
  list_iter_struct_t iter_opts;
  struct argparse_option *opt;
  struct argparse_item *itm;
  struct argparse_handle *ap = st->current_cmd;
  
  num_opts = list_length(ap->pos_options);
  xlog_debug(logger, "check required opts for %s\n", ap->subcommand_name);
  if (ap->curr_posarg != num_opts) {
    if (ap->curr_posarg < num_opts) {
      opt = list_get(ap->pos_options, ap->curr_posarg);
      xlog_err(logger, "Missing argument %s\n", opt->name);
      argparse_helpmsg(st->root_cmd);
    }
    return ARGPARSE_ERROR;
  }

  for (list_iter_init(ap->options, &iter_opts); ! list_iter_end(&iter_opts);
       list_iter_next(&iter_opts)) {
    opt = list_iter_data(&iter_opts);
    /*  check that there is at least one argument for a required option */
    if (opt->required) {
      for (list_iter_init(ap->args, &iter_args); ! list_iter_end(&iter_args);
	   list_iter_next(&iter_args)) {
	itm = list_iter_data(&iter_args);
	if (itm->opt == opt)
	  break;
      }
      if (list_iter_end(&iter_args))
	break;
    }
  }
  if (! list_iter_end(&iter_opts)) {
    /* if the search stopped, we found a required option without args */
    xlog_err(logger, "Missing required argument %s\n", opt->name);
    argparse_helpmsg(st->root_cmd);
    return ARGPARSE_ERROR;
  }
  return ARGPARSE_OK;
}

/**
 * Add unset flags to the arguments list
 */
static int
argparse_fixup_flags(struct argparse_parser_state *st)
{
  int err;
  list_iter_struct_t iter_args;
  list_iter_struct_t iter_opts;
  struct argparse_option *opt;
  struct argparse_item *item;
  struct argparse_handle *ap = st->current_cmd;

  xlog_debug(logger, "fixup flags for %s\n", ap->subcommand_name);

  /* iterate over flag options, if an argument is not specified for them,
   * create one
   */
  for (list_iter_init(ap->options, &iter_opts); ! list_iter_end(&iter_opts);
       list_iter_next(&iter_opts)) {
    opt = list_iter_data(&iter_opts);
    if (opt->type == T_FLAG) {
      for (list_iter_init(ap->args, &iter_args); ! list_iter_end(&iter_args);
	   list_iter_next(&iter_args)) {
	item = list_iter_data(&iter_args);
	if (item->opt == opt)
	  break;
      }
      if (list_iter_end(&iter_args)) {
	/* create missing argument */
	item = malloc(sizeof(struct argparse_item));
	if (item == NULL)
	  return ARGPARSE_ERROR;
	item->opt = opt;
	item->int_arg = 0;
	err = list_push(ap->args, item);
	if (err) {
	  free(item);
	  return ARGPARSE_ERROR;
	}
      }
    }
  }
  return ARGPARSE_OK;
}

/**
 * Switch current parser when a positional argument is found
 */
static int
argparse_next_subcmd(struct argparse_parser_state *st)
{
  list_iter_struct_t iter;
  struct argparse_handle *ap = st->current_cmd;
  char *next_name;
  struct argparse_handle *next;
  int error;

  next_name = st->argv[st->index++];

  for (list_iter_init(ap->subcommands, &iter); ! list_iter_end(&iter);
       list_iter_next(&iter)) {
    next = list_iter_data(&iter);
    if (strcmp(next->subcommand_name, next_name) == 0)
      break;
  }
  if (list_iter_end(&iter)) {
    /* not found */
    xlog_err(logger, "Invalid command %s\n", next_name);
    return ARGPARSE_ERROR;
  }
  xlog_debug(logger, "subcommand switch %s -> %s @ %p\n",
	     ap->subcommand_name, next_name, next);
  error = list_push(st->subcmd_stack, ap);
  if (error)
    return ARGPARSE_ERROR;
  st->current_cmd = next;;

  return ARGPARSE_OK;
}

/**
 * Switch current parser to the previous parser found in
 * the stack when the last argument for the current parser
 * has been consumed.
 */
static int
argparse_prev_subcmd(struct argparse_parser_state *st)
{
  struct argparse_handle *curr_ap;

  if (list_length(st->subcmd_stack) == 0)
    /* already at top level parser */
    return ARGPARSE_OK;
  
  curr_ap = list_pop(st->subcmd_stack);
  if (curr_ap == NULL) {
    /* something wrong happened, break */
    xlog_err(logger, "Error popping subcommand from stack\n");
    return ARGPARSE_ERROR;
  }

  xlog_debug(logger, "pop back cmd %s\n", curr_ap->subcommand_name);
  st->current_cmd = curr_ap;
  return ARGPARSE_OK;
}

/**
 * finalize the arguments of a subcommand
 * after its last positional argument has
 * been parsed.
 * Call the user-defined callback to extract
 * the arguments.
 */
static int
argparse_finalize_subcmd(struct argparse_parser_state *st)
{
  int error;
  
  error = argparse_fixup_flags(st);
  if (error)
    return error;
  error = argparse_check_required(st);
  if (error)
    return error;
  /* invoke user callback to fetch arguments */
  if (st->current_cmd->cbk != NULL) {
    error = st->current_cmd->cbk(st->current_cmd, st->current_cmd->userdata);
    if (error)
      return error;
  }
  return ARGPARSE_OK;
}

/** 
 * Perform finalization operations after the last argument
 * for the current subcommand is parsed.
 */
static int
argparse_finalize_pop_subcmd(struct argparse_parser_state *st)
{
  int error;
  
  xlog_debug(logger, "Finalize subcommand %s\n", st->current_cmd->subcommand_name);
  error = argparse_finalize_subcmd(st);
  if (error)
    return error;
  error = argparse_prev_subcmd(st);
  if (error)
    return error;
  return ARGPARSE_OK;
}

int
argparse_parse(argparse_t ap, int argc, char *argv[])
{
  struct argparse_parser_state state;
  struct argparse_handle *curr_ap;
  int error;
  char *arg;

  assert(ap != NULL);

  state.argv = argv;
  state.argc = argc;
  state.index = 1;
  state.root_cmd = ap;
  error = list_init(&state.subcmd_stack, NULL, NULL);
  if (error)
    goto err;
  state.current_cmd = ap;
  state.last_subcmd = false;
  ap->bin_name = argv[0];

  while (state.index < argc) {
    curr_ap = state.current_cmd;
    arg = argv[state.index];
    xlog_debug(logger, "pick %s\n", arg);
    /* index is updated in the parser functions */
    if (arg[0] == '-') {
      xlog_debug(logger, "named arg %s for %s\n",
		 arg, curr_ap->subcommand_name);
      error = argparse_parse_arg_named(&state);
      if (error)
	goto err_dealloc;
    }
    else if (!state.last_subcmd &&
	     list_length(curr_ap->subcommands) > 0) {
      /* switch to next subcommand */
      xlog_debug(logger, "subcommand switch %s for %s\n",
		 arg, curr_ap->subcommand_name);
      error = argparse_next_subcmd(&state);
    }
    else {
      if (list_length(curr_ap->subcommands) == 0)
	state.last_subcmd = true;
      
      xlog_debug(logger, "posarg %s for %s\n", arg, curr_ap->subcommand_name);
      error = argparse_parse_posarg(&state);
      if (error)
	goto err_dealloc;

      if (list_length(curr_ap->pos_options) == curr_ap->curr_posarg) {
	/* this is the last posarg, finalize */
	error = argparse_finalize_pop_subcmd(&state);
	/* need to update this since prev_subcmd changed the current parser */
	curr_ap = state.current_cmd;
      }
    }

    if (error)
      goto err_dealloc;
  }
  
  while (state.current_cmd != state.root_cmd) {
    error = argparse_finalize_pop_subcmd(&state);
    if (error) {
      goto err_dealloc;
    }
  }

  /* make sure that we are exiting the loop in a consistent state */
  assert(state.current_cmd == state.root_cmd);
  /* finalize top-level parser */
  error = argparse_finalize_subcmd(&state);
  if (error)
    goto err_dealloc;

  list_destroy(state.subcmd_stack);
  return ARGPARSE_OK;

 err_dealloc:
  list_destroy(state.subcmd_stack);
 err:
  return ARGPARSE_ERROR;
}
