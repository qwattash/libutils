/*
 * Simple argument parser library
 */

#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stdbool.h>

/*
 * TO DO
 * add argparse_config_get argparse_config_set
 * to change behaviour:
 * - treat subcommands as optional, a subcommand
 * can be skipped: if so when an argument does not
 * match the expected keyword of a subcommand, treat it
 * as a posarg for the current subcommand and set last_subcmd = true
 * to indicate that we are not going to parse other subcommands.
 * Note that due to the modularity of the finalization step that is
 * run upon subcommand completion, required arguments for the skipped
 * subcommands are never checked, this is the intended behaviour.
 */

/*
 * Opaque handle representing an argument parser
 */
struct argparse_handle;

enum argparse_argtype {
  T_INT,
  T_STRING,
  T_FLAG,
};

typedef struct argparse_handle * argparse_t;
typedef enum argparse_argtype argtype_t;
typedef int (*argconsumer_t)(const argparse_t, void *);

/*
 * Error codes
 */
#define ARGPARSE_OK 0
#define ARGPARSE_ERROR 1
#define ARGPARSE_NOARG 2

/* maximum string argument length */
#define ARGPARSE_STR_MAX 64

/*
 * Initialise a top-level argparse handle for a new parser
 * @param[in,out] ap pointer to an argparse handle
 * @param[in] help help string for the top-level parser
 * @param[in] cbk callback invoked when top-level options
 * @param[in] userdata user-defined data passed to the callback
 * have been parsed.
 * @return zero on success, error code on failure
 */
int argparse_init(argparse_t *ap, const char *help, argconsumer_t cbk,
		  void *userdata);

/*
 * Deallocate a parser
 * @param[in,out] ap argparse handle
 * @return zero on success, error code on failure
 */
int argparse_destroy(argparse_t ap);

/*
 * Reset the parser state to parse another argument vector
 * @param[in,out] ap argparse handle
 * @return ARGPARSE_OK on success, error code on failure
 */
int argparse_reset(argparse_t ap);

/*
 * Add argument option to the parser
 * @param[in,out] ap argparse handle
 * @param[in] name long name of the option
 * @param[in] shortname character to switch the option
 * @param[in] type option type to parse
 * @param[in] help help string
 * @param[in] required make option mandatory
 */
int argparse_arg_add(argparse_t ap, const char *name, char shortname,
		     argtype_t type, const char *help, bool required);

/*
 * Add positional argument to the parser, the call order determines
 * the position of the argument, the first positional argument added
 * is the last expected one during the parsing.
 * @param[in,out] ap argparse handle
 * @param[in] name long name of the option
 * @param[in] type option type to parse
 * @param[in] help help string
 */
int argparse_posarg_add(argparse_t ap, const char *name,
			argtype_t type, const char *help);

/*
 * Get argument value for a parser
 * if the argument was not specified E_NOARG is returned, if
 * there are no more unnamed args E_NO_MORE_ARGS is returned
 */
int argparse_arg_get(argparse_t ap, const char *name,
		     void *arg, int bufsize);
int argparse_posarg_get(argparse_t ap, int idx,
			void *arg, int bufsize);

/*
 * Parse given args
 */
int argparse_parse(argparse_t ap, int argc, char *argv[]);

/*
 * Show help message
 */
void argparse_helpmsg(argparse_t ap);

 /*
 * Subcommands are matched before the rest of the positional
 * arguments, so the positional arguments added to the main
 * parser are considered after every nested subcommand has been
 * parsed.
 * Subcommands are always required, in the sense that each nested
 * layer of subcommand consumes a positional argument.
 */

/*
 * Create new subcommand
 * @param[in,out] ap argparse handle
 * @param[in] name the subcommand name
 * @param[in] cbk callback invoked when options to the
 * subcommand have been parsed;
 * @param[in] userdata user-defined data passed to the callback
 * @return subcommand handle
 *
 * The callback is designed to simplify the machinery for
 * picking the parsed arguments from subcommands.
 */
argparse_t argparse_subcmd_add(argparse_t ap, const char *name,
			       const char *help, argconsumer_t cbk,
			       void *userdata);

#endif
