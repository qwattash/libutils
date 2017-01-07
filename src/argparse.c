/*
 * Simple argument parser library
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "list.h"
#include "argparse.h"
#include "log.h"

/* logger */
#ifdef ENABLE_LOGGING
static struct logger_handle _logger = {
#ifdef DEBUG
  .level = LOG_DEBUG,
#else /* ! DEBUG */
  .level = LOG_ERR,
#endif /* ! DEBUG */
  .prefix = "[argparse] ",
};
#define logger &_logger
#endif

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
 * list walker callback arguments to
 * compare and find an element in the handle.args and
 * handle.pos_args lists.
 * Used by argparse_cmp_arg.
 */
struct argparse_cmp_args {
  /* [in] long name of the option associated to the argument. */
  const char *name;
  /* [out] pointer to the first matching item in the list with
   * the given name.
   */
  struct argparse_item *data;
};

/*
 * list walker callback arguments to
 * compare and find an element in the handle.options and
 * handle.pos_options lists.
 * if either the shortname or longname match, the option
 * is returned in the cmp_args.data pointer.
 * Used by argparse_cmp_opt.
 */
struct argparse_opt_cmp_args {
  /* [in] short name of the option */
  char shortname;
  /* [in] long name of the option */
  const char *longname;
  /* [out] pointer to the first matching item in the list with
   * the given name.
   */
  struct argparse_option *data;
};

/*
 * list walker callback arguments to find a
 * subcommand given its name
 */
struct argparse_find_subcommand_args {
  /* [in] name of the subcommand */
  const char *name;
  /* [out] matching subcommand */
  struct argparse_handle *match;
};

/*
 * list walker callback arguments to compare
 * and find any required argument that has not been
 * given by the user
 */
struct argparse_check_req_opt_args {
  argparse_t ap;
  struct argparse_option *opt;
};

/*
 * list walker callback arguments to find an argument 
 * associated with a given option
 */
struct argparse_find_arg_for_opt_args {
  struct argparse_option *opt;
  struct argparse_item *item;
};

/*
 * list walker callback arguments to add flags that
 * are "false" to the arguments list
 */
struct argparse_add_unset_flag_args {
  argparse_t ap;
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

/* static functions */
static int argparse_subcommand_ctor(void **itmdata, void *data);
static int argparse_subcommand_dtor(void *data);
static int argparse_options_ctor(void **itmdata, void *data);
static int argparse_options_dtor(void *data);
static int argparse_item_dtor(void *data);
static int argparse_destroy_subcmd(void *data, void *args);
static int argparse_cmp_arg(void *data, void *args);
static int argparse_cmp_opt(void *data, void *args);
static int argparse_find_arg_for_opt(void *data, void *args);
static int argparse_check_req_opt(void *data, void *args);
static int argparse_add_unset_flag(void *data, void *args);
static int argparse_help_subcommand(void *data, void *args);
static int argparse_help_posoption(void *data, void *args);
static int argparse_help_option(void *data, void *args);
static void argparse_subcommand_helpmsg(argparse_t ap);
static int argparse_helpmsg_cbk(void *data, void *args);
static void argparse_copyout_arg(struct argparse_item *item,
				 void *buf, int bufsize);
static int argparse_parseopt(struct argparse_item *item, char *arg);
static int argparse_parse_posarg(struct argparse_parser_state *st);
static int argparse_parse_arg_named(struct argparse_parser_state *st);
static int argparse_check_required(struct argparse_parser_state *st);
static int argparse_fixup_flags(struct argparse_parser_state *st);
static int argparse_find_subcommand(void *data, void *args);
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
  if (error)
    goto err_sub;
  error = list_init(&sub->args, NULL, argparse_item_dtor);
  if (error)
    goto err_args;
  error = list_init(&sub->pos_args, NULL, argparse_item_dtor);
  if (error)
    goto err_posargs;
  error = list_init(&sub->options, argparse_options_ctor,
		    argparse_options_dtor);
  if (error)
    goto err_options;
  error = list_init(&sub->pos_options, argparse_options_ctor,
		    argparse_options_dtor);
  if (error)
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
  int error = 0;

  /* NOTE: sub->subcommands is destroyed by argparse_destroy_subcmd */
  xlog_debug(logger, "Destroy %s\n", sub->subcommand_name);
  error = list_destroy(sub->args);
  error |= list_destroy(sub->pos_args);
  error |= list_destroy(sub->options);
  error |= list_destroy(sub->pos_options);
  if (sub->subcommand_name != NULL)
    free(sub->subcommand_name);
  if (sub->subcommand_help != NULL)
    free(sub->subcommand_help);
  /* NOTE bin_name is assumed to come from argv, so
   * do not deallocate.
   */
  if (error)
    return ARGPARSE_ERROR;
  free(sub);
  return ARGPARSE_OK;
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
    return ARGPARSE_ERROR;
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
  return ARGPARSE_OK;
  
 err_help:  
  free(opt_item->name);
 err_name:
  free(opt_item);
  return ARGPARSE_ERROR;
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
 * Callback that deallocates subcommands in a subcommand list
 * *args is unused
 */
static int
argparse_destroy_subcmd(void *data, void *args)
{
  struct argparse_handle *sub = (struct argparse_handle *)data;
  int error;
  
  error = list_walk(sub->subcommands, argparse_destroy_subcmd, NULL);
  error |= list_destroy(sub->subcommands);
  return error;
}

/**
 * Callback for searching an argument in an argparse_item list
 */
static int
argparse_cmp_arg(void *data, void *args)
{
  struct argparse_item *itm = data;
  struct argparse_cmp_args *cmp_args = args;

  if (itm->opt == NULL)
    return -1; /* XXX: change to error.h */
  
  if (strcmp(itm->opt->name, cmp_args->name) == 0) {
    cmp_args->data = data;
    return 1; /* stop iteration */
  }
  return ARGPARSE_OK;
}

/**
 * Callback for searching an argument in an argparse_options list
 */
static int
argparse_cmp_opt(void *data, void *args)
{
  struct argparse_option *opt = data;
  struct argparse_opt_cmp_args *cmp_args = args;

  bool match_longname = (cmp_args->longname != NULL &&
			strlen(cmp_args->longname) > 0 &&
			strcmp(opt->name, cmp_args->longname) == 0);
  bool match_shortname =  (opt->shortname != '\0' &&
			   opt->shortname == cmp_args->shortname);
  if (match_longname || match_shortname) {
    cmp_args->data = data;
    return 1; /* stop iteration */
  }
  return ARGPARSE_OK;
}

/**
 * Callback for finding an argument associated with a given option
 */
static int
argparse_find_arg_for_opt(void *data, void *args)
{
  struct argparse_item *itm = data;
  struct argparse_find_arg_for_opt_args *find_args = args;
  
  if (itm->opt == find_args->opt) {
    find_args->item = itm;
    return 1;
  }
  return ARGPARSE_OK;
}

/**
 * Callback for searching for missing required arguments.
 * When a required option is not specified in the arglist the
 * req_opt_args->opt is set to that option an the iteration is stopped.
 * if all required options are specified the req_opt_args->opt field is
 * set to NULL.
 */
static int
argparse_check_req_opt(void *data, void *args)
{
  struct argparse_option *opt = data;
  struct argparse_check_req_opt_args *check_args = args;
  struct argparse_find_arg_for_opt_args find_args;
  int err;

  check_args->opt = NULL;

  /* if this option is required, check that there is at least one argument
   * for that option.
   */
  if (opt->required) {
    find_args.opt = opt;
    find_args.item = NULL;
    /* find first occurrenct of argument with opt as parsed option */
    err = list_walk(check_args->ap->args, argparse_find_arg_for_opt,
		    &find_args);
    /* if the argument is found, err is 0 and 
     * find_args->item != NULL 
     */
    if (err)
      return -1; /* abort */
    if (find_args.item == NULL) {
      /* no args found, stop iterating */
      check_args->opt = opt;
      return 1;
    }
  }
  return ARGPARSE_OK; /* keep iterating */
}

/**
 * Callback that adds a "false" flag argument for a FLAG option if
 * no argument for that option is already set.
 */
static int
argparse_add_unset_flag(void *data, void *args)
{
  struct argparse_option *opt = data;
  struct argparse_add_unset_flag_args *walk_args = args;
  struct argparse_find_arg_for_opt_args find_args;
  struct argparse_item *item;
  int err;

  if (opt->type == T_FLAG) {
    find_args.opt = opt;
    find_args.item = NULL;
    err = list_walk(walk_args->ap->args, argparse_find_arg_for_opt,
		    &find_args);
    if (err)
      return -1; /* abort */
    if (find_args.item == NULL) {
      /* create missing argument */
      item = malloc(sizeof(struct argparse_item));
      if (item == NULL)
	return ARGPARSE_ERROR;
      item->opt = opt;
      item->int_arg = 0;
      err = list_push(walk_args->ap->args, item);
      if (err) {
	free(item);
	return -1;
      }
    }
  }
  return ARGPARSE_OK;
}

/**
 * Callback that prints the help message for an option to stderr
 */
static int
argparse_help_option(void *data, void *args)
{
  struct argparse_option *opt = data;

  if (opt->shortname == '\0')
    xlog_msg(logger, "\t--%s\t\t%s\n", opt->name, opt->help);
  else
    xlog_msg(logger, "\t-%c,--%s\t\t%s\n", opt->shortname,
	     opt->name, opt->help);
  return ARGPARSE_OK;
}

/**
 * Callback that prints the help message for a positional 
 * argument to stderr
 */
static int
argparse_help_posoption(void *data, void *args)
{
  struct argparse_option *opt = data;

  xlog_msg(logger, "\t%s\t\t\t%s\n", opt->name, opt->help);
  return ARGPARSE_OK;
}

/**
 * Callback that prints the help message for all
 * the subcommands in a subcommand list
 */
static int
argparse_help_subcommand(void *data, void *args)
{
  struct argparse_handle *ap = data;

  xlog_msg(logger, "\t%s\t\t%s\n", ap->subcommand_name, ap->subcommand_help);
  return ARGPARSE_OK;
}

/**
 * Callback that finds a subcommand with a 
 * matching name
 */
static int
argparse_find_subcommand(void *data, void *args)
{
  struct argparse_handle *ap = data;
  struct argparse_find_subcommand_args *find_args = args;
  if (strcmp(ap->subcommand_name, find_args->name) == 0) {
    /* found */
    find_args->match = ap;
    return 1; /* stop iteration */
  }
  return ARGPARSE_OK;
}

/**
 * Display help message for a subcommand, with command name,
 * subcommands summary, options and arguments.
 */
static void
argparse_subcommand_helpmsg(argparse_t ap)
{
  if (ap->subcommand_name == NULL)
    /* root subcommand */
    xlog_msg(logger, "Usage: %s [options] [subcommands] [arguments]\n",
	     ap->bin_name);
  else
    xlog_msg(logger, "Command: %s [options] [subcommands] [arguments]\n%s\n",
	     ap->subcommand_name, ap->subcommand_help);
  /* print help for each option */
  xlog_msg(logger, "Subcommands:\n");
  list_walk(ap->subcommands, argparse_help_subcommand, NULL);
  xlog_msg(logger, "Options:\n");
  list_walk(ap->options, argparse_help_option, NULL);
  xlog_msg(logger, "\nArguments:\n");
  list_walk(ap->pos_options, argparse_help_posoption, NULL);
  xlog_msg(logger, "\n\n");
}

/**
 * Callback for walking the subcommand tree.
 * Reuse the same function that prints help
 * for the root subcommand recursively.
 */
static int
argparse_helpmsg_cbk(void *data, void *args)
{
  argparse_helpmsg((struct argparse_handle *)data);
  return ARGPARSE_OK;
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
  argparse_subcommand_helpmsg(ap);
  list_walk(ap->subcommands, argparse_helpmsg_cbk, NULL);
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

  if (pap == NULL)
    return -1;

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
  int error = 0;
  /* error is used as an accumulator to check
   * that all destructors return zero
   */
  assert(ap != NULL);
  xlog_debug(logger, "Destroy root %s\n", ap->subcommand_name);
  error = list_walk(ap->subcommands, argparse_destroy_subcmd, NULL);
  error |= list_destroy(ap->subcommands);
  error |= argparse_subcommand_dtor(ap);
  return error;
}

/*
 * Reset parser state
 */
int
argparse_reset(argparse_t ap)
{
  int rc;
  list_iter_t subcommands;
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

  subcommands = list_iter(ap->subcommands);
  if (subcommands == NULL) {
    xlog_err(logger, "Can not create subcommands iterator for %s\n",
	     ap->subcommand_name);
    return ARGPARSE_ERROR;
  }
  while (! list_iter_end(subcommands)) {
    sub = list_iter_item(subcommands);
    if (sub == NULL) {
      xlog_err(logger, "Invalid subcommand\n");
      return ARGPARSE_ERROR;
    }
    rc = argparse_reset(sub);
    if (rc == ARGPARSE_ERROR) {
      list_iter_free(subcommands);
      return rc;
    }
    list_iter_next(subcommands);
  }
  list_iter_free(subcommands);
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
  struct argparse_cmp_args cmp_args;
  int error;

  assert(ap != NULL);

  cmp_args.name = name;
  cmp_args.data = NULL;
  
  error = list_walk(ap->args, argparse_cmp_arg, &cmp_args);
  if (error)
    return (error);
  if (cmp_args.data == NULL)
    return ARGPARSE_NOARG;

  argparse_copyout_arg(cmp_args.data, arg, bufsize);
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

  item = list_getitem(ap->pos_args, idx);
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
  opt = list_getitem(ap->pos_options, ap->curr_posarg);

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
  struct argparse_opt_cmp_args cmp_args;
  struct argparse_item *item;
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
  
  if (is_longopt) {
    cmp_args.shortname = '\0';
    cmp_args.longname = arg;
  }
  else {
    cmp_args.shortname = *arg;
    cmp_args.longname = NULL;
  }
  cmp_args.data = NULL;
  
  error = list_walk(ap->options, argparse_cmp_opt, &cmp_args);
  if (error < 0)
    return (error);
  if (cmp_args.data == NULL) {
    xlog_err(logger, "Invalid option -%s\n", arg);
    argparse_helpmsg(st->root_cmd);
    return ARGPARSE_ERROR;
  }

  item = malloc(sizeof(struct argparse_item));
  if (item == NULL)
    return ARGPARSE_ERROR;
  item->opt = cmp_args.data;

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

static int
argparse_check_required(struct argparse_parser_state *st)
{
  int err, num_opts;
  struct argparse_check_req_opt_args cmp_args;
  struct argparse_option *opt;
  struct argparse_handle *ap = st->current_cmd;
  
  num_opts = list_length(ap->pos_options);
  xlog_debug(logger, "check required opts for %s\n", ap->subcommand_name);
  if (ap->curr_posarg != num_opts) {
    if (ap->curr_posarg < num_opts) {
      opt = list_getitem(ap->pos_options, ap->curr_posarg);
      xlog_err(logger, "Missing argument %s\n", opt->name);
      argparse_helpmsg(st->root_cmd);
    }
    return ARGPARSE_ERROR;
  }
  
  cmp_args.ap = ap;
  cmp_args.opt = NULL;

  err = list_walk(ap->options, argparse_check_req_opt, &cmp_args);
  if (err == 0) {
    if (cmp_args.opt == NULL)
      return ARGPARSE_OK;
    else {
      /* missing item found */
      xlog_err(logger, "Missing required argument %s\n", cmp_args.opt->name);
      argparse_helpmsg(st->root_cmd);
      return ARGPARSE_ERROR;
    }
  }
  else
    return ARGPARSE_ERROR;

  /* not reached */
}

/**
 * Add unset flags to the arguments list
 */
static int
argparse_fixup_flags(struct argparse_parser_state *st)
{
  int err;
  struct argparse_add_unset_flag_args walk_args;
  struct argparse_handle *ap = st->current_cmd;

  xlog_debug(logger, "fixup flags for %s\n", ap->subcommand_name);
  walk_args.ap = ap;

  /* iterate over flag options, if an argument is not specified for them,
   * create one
   */
  err = list_walk(ap->options, argparse_add_unset_flag, &walk_args);
  if (err < 0)
    return ARGPARSE_ERROR;
  return ARGPARSE_OK;
}

/**
 * Switch current parser when a positional argument is found
 */
static int
argparse_next_subcmd(struct argparse_parser_state *st)
{
  int error;
  struct argparse_find_subcommand_args args;

  args.name = st->argv[st->index++];
  args.match = NULL;

  error = list_walk(st->current_cmd->subcommands,
		    argparse_find_subcommand,
		    &args);
  xlog_debug(logger, "subcommand switch %s -> %s @ %p\n",
	     st->current_cmd->subcommand_name, args.name, args.match);
  if (error < 0 || args.match == NULL) {
    xlog_err(logger, "Invalid command %s\n", args.name);
    return ARGPARSE_ERROR;
  }
  error = list_push(st->subcmd_stack, st->current_cmd);
  if (error)
    return ARGPARSE_ERROR;
  st->current_cmd = args.match;

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
