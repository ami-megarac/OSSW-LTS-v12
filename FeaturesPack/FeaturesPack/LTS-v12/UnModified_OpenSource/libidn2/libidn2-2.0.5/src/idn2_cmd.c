/*
  File autogenerated by gengetopt version 2.22.6
  generated with the following command:
  gengetopt --unamed-opts --no-handle-version --no-handle-help --set-package=idn2 --input idn2.ggo --file-name idn2_cmd 

  The developers of gengetopt consider the fixed text that goes in all
  gengetopt output files to be in the public domain:
  we make no copyright claims on it.
*/

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef FIX_UNUSED
#define FIX_UNUSED(X) (void) (X) /* avoid warnings for unused params */
#endif

#include <getopt.h>

#include "idn2_cmd.h"

const char *gengetopt_args_info_purpose = "";

const char *gengetopt_args_info_usage = "Usage: idn2 [OPTION]... [STRING]...";

const char *gengetopt_args_info_versiontext = "";

const char *gengetopt_args_info_description = "";

const char *gengetopt_args_info_help[] = {
  "  -h, --help               Print help and exit",
  "  -V, --version            Print version and exit",
  "  -d, --decode             Decode (punycode) domain name",
  "  -l, --lookup             Lookup domain name (default)",
  "  -r, --register           Register label",
  "  -T, --tr46t              Enable TR46 transitional processing  (default=off)",
  "  -N, --tr46nt             Enable TR46 non-transitional processing\n                             (default=off)",
  "      --no-tr46            Disable TR46 processing  (default=off)",
  "      --usestd3asciirules  Enable STD3 ASCII rules  (default=off)",
  "      --debug              Print debugging information  (default=off)",
  "      --quiet              Silent operation  (default=off)",
    0
};

typedef enum {ARG_NO
  , ARG_FLAG
} cmdline_parser_arg_type;

static
void clear_given (struct gengetopt_args_info *args_info);
static
void clear_args (struct gengetopt_args_info *args_info);

static int
cmdline_parser_internal (int argc, char **argv, struct gengetopt_args_info *args_info,
                        struct cmdline_parser_params *params, const char *additional_error);


static char *
gengetopt_strdup (const char *s);

static
void clear_given (struct gengetopt_args_info *args_info)
{
  args_info->help_given = 0 ;
  args_info->version_given = 0 ;
  args_info->decode_given = 0 ;
  args_info->lookup_given = 0 ;
  args_info->register_given = 0 ;
  args_info->tr46t_given = 0 ;
  args_info->tr46nt_given = 0 ;
  args_info->no_tr46_given = 0 ;
  args_info->usestd3asciirules_given = 0 ;
  args_info->debug_given = 0 ;
  args_info->quiet_given = 0 ;
}

static
void clear_args (struct gengetopt_args_info *args_info)
{
  FIX_UNUSED (args_info);
  args_info->tr46t_flag = 0;
  args_info->tr46nt_flag = 0;
  args_info->no_tr46_flag = 0;
  args_info->usestd3asciirules_flag = 0;
  args_info->debug_flag = 0;
  args_info->quiet_flag = 0;
  
}

static
void init_args_info(struct gengetopt_args_info *args_info)
{


  args_info->help_help = gengetopt_args_info_help[0] ;
  args_info->version_help = gengetopt_args_info_help[1] ;
  args_info->decode_help = gengetopt_args_info_help[2] ;
  args_info->lookup_help = gengetopt_args_info_help[3] ;
  args_info->register_help = gengetopt_args_info_help[4] ;
  args_info->tr46t_help = gengetopt_args_info_help[5] ;
  args_info->tr46nt_help = gengetopt_args_info_help[6] ;
  args_info->no_tr46_help = gengetopt_args_info_help[7] ;
  args_info->usestd3asciirules_help = gengetopt_args_info_help[8] ;
  args_info->debug_help = gengetopt_args_info_help[9] ;
  args_info->quiet_help = gengetopt_args_info_help[10] ;
  
}

void
cmdline_parser_print_version (void)
{
  printf ("%s %s\n",
     (strlen(CMDLINE_PARSER_PACKAGE_NAME) ? CMDLINE_PARSER_PACKAGE_NAME : CMDLINE_PARSER_PACKAGE),
     CMDLINE_PARSER_VERSION);

  if (strlen(gengetopt_args_info_versiontext) > 0)
    printf("\n%s\n", gengetopt_args_info_versiontext);
}

static void print_help_common(void) {
  cmdline_parser_print_version ();

  if (strlen(gengetopt_args_info_purpose) > 0)
    printf("\n%s\n", gengetopt_args_info_purpose);

  if (strlen(gengetopt_args_info_usage) > 0)
    printf("\n%s\n", gengetopt_args_info_usage);

  printf("\n");

  if (strlen(gengetopt_args_info_description) > 0)
    printf("%s\n\n", gengetopt_args_info_description);
}

void
cmdline_parser_print_help (void)
{
  int i = 0;
  print_help_common();
  while (gengetopt_args_info_help[i])
    printf("%s\n", gengetopt_args_info_help[i++]);
}

void
cmdline_parser_init (struct gengetopt_args_info *args_info)
{
  clear_given (args_info);
  clear_args (args_info);
  init_args_info (args_info);

  args_info->inputs = 0;
  args_info->inputs_num = 0;
}

void
cmdline_parser_params_init(struct cmdline_parser_params *params)
{
  if (params)
    { 
      params->override = 0;
      params->initialize = 1;
      params->check_required = 1;
      params->check_ambiguity = 0;
      params->print_errors = 1;
    }
}

struct cmdline_parser_params *
cmdline_parser_params_create(void)
{
  struct cmdline_parser_params *params = 
    (struct cmdline_parser_params *)malloc(sizeof(struct cmdline_parser_params));
  cmdline_parser_params_init(params);  
  return params;
}



static void
cmdline_parser_release (struct gengetopt_args_info *args_info)
{
  unsigned int i;
  
  
  for (i = 0; i < args_info->inputs_num; ++i)
    free (args_info->inputs [i]);

  if (args_info->inputs_num)
    free (args_info->inputs);

  clear_given (args_info);
}


static void
write_into_file(FILE *outfile, const char *opt, const char *arg, const char *values[])
{
  FIX_UNUSED (values);
  if (arg) {
    fprintf(outfile, "%s=\"%s\"\n", opt, arg);
  } else {
    fprintf(outfile, "%s\n", opt);
  }
}


int
cmdline_parser_dump(FILE *outfile, struct gengetopt_args_info *args_info)
{
  int i = 0;

  if (!outfile)
    {
      fprintf (stderr, "%s: cannot dump options to stream\n", CMDLINE_PARSER_PACKAGE);
      return EXIT_FAILURE;
    }

  if (args_info->help_given)
    write_into_file(outfile, "help", 0, 0 );
  if (args_info->version_given)
    write_into_file(outfile, "version", 0, 0 );
  if (args_info->decode_given)
    write_into_file(outfile, "decode", 0, 0 );
  if (args_info->lookup_given)
    write_into_file(outfile, "lookup", 0, 0 );
  if (args_info->register_given)
    write_into_file(outfile, "register", 0, 0 );
  if (args_info->tr46t_given)
    write_into_file(outfile, "tr46t", 0, 0 );
  if (args_info->tr46nt_given)
    write_into_file(outfile, "tr46nt", 0, 0 );
  if (args_info->no_tr46_given)
    write_into_file(outfile, "no-tr46", 0, 0 );
  if (args_info->usestd3asciirules_given)
    write_into_file(outfile, "usestd3asciirules", 0, 0 );
  if (args_info->debug_given)
    write_into_file(outfile, "debug", 0, 0 );
  if (args_info->quiet_given)
    write_into_file(outfile, "quiet", 0, 0 );
  

  i = EXIT_SUCCESS;
  return i;
}

int
cmdline_parser_file_save(const char *filename, struct gengetopt_args_info *args_info)
{
  FILE *outfile;
  int i = 0;

  outfile = fopen(filename, "w");

  if (!outfile)
    {
      fprintf (stderr, "%s: cannot open file for writing: %s\n", CMDLINE_PARSER_PACKAGE, filename);
      return EXIT_FAILURE;
    }

  i = cmdline_parser_dump(outfile, args_info);
  fclose (outfile);

  return i;
}

void
cmdline_parser_free (struct gengetopt_args_info *args_info)
{
  cmdline_parser_release (args_info);
}

/** @brief replacement of strdup, which is not standard */
char *
gengetopt_strdup (const char *s)
{
  char *result = 0;
  if (!s)
    return result;

  result = (char*)malloc(strlen(s) + 1);
  if (result == (char*)0)
    return (char*)0;
  strcpy(result, s);
  return result;
}

int
cmdline_parser (int argc, char **argv, struct gengetopt_args_info *args_info)
{
  return cmdline_parser2 (argc, argv, args_info, 0, 1, 1);
}

int
cmdline_parser_ext (int argc, char **argv, struct gengetopt_args_info *args_info,
                   struct cmdline_parser_params *params)
{
  int result;
  result = cmdline_parser_internal (argc, argv, args_info, params, 0);

  if (result == EXIT_FAILURE)
    {
      cmdline_parser_free (args_info);
      exit (EXIT_FAILURE);
    }
  
  return result;
}

int
cmdline_parser2 (int argc, char **argv, struct gengetopt_args_info *args_info, int override, int initialize, int check_required)
{
  int result;
  struct cmdline_parser_params params;
  
  params.override = override;
  params.initialize = initialize;
  params.check_required = check_required;
  params.check_ambiguity = 0;
  params.print_errors = 1;

  result = cmdline_parser_internal (argc, argv, args_info, &params, 0);

  if (result == EXIT_FAILURE)
    {
      cmdline_parser_free (args_info);
      exit (EXIT_FAILURE);
    }
  
  return result;
}

int
cmdline_parser_required (struct gengetopt_args_info *args_info, const char *prog_name)
{
  FIX_UNUSED (args_info);
  FIX_UNUSED (prog_name);
  return EXIT_SUCCESS;
}


static char *package_name = 0;

/**
 * @brief updates an option
 * @param field the generic pointer to the field to update
 * @param orig_field the pointer to the orig field
 * @param field_given the pointer to the number of occurrence of this option
 * @param prev_given the pointer to the number of occurrence already seen
 * @param value the argument for this option (if null no arg was specified)
 * @param possible_values the possible values for this option (if specified)
 * @param default_value the default value (in case the option only accepts fixed values)
 * @param arg_type the type of this option
 * @param check_ambiguity @see cmdline_parser_params.check_ambiguity
 * @param override @see cmdline_parser_params.override
 * @param no_free whether to free a possible previous value
 * @param multiple_option whether this is a multiple option
 * @param long_opt the corresponding long option
 * @param short_opt the corresponding short option (or '-' if none)
 * @param additional_error possible further error specification
 */
static
int update_arg(void *field, char **orig_field,
               unsigned int *field_given, unsigned int *prev_given, 
               char *value, const char *possible_values[],
               const char *default_value,
               cmdline_parser_arg_type arg_type,
               int check_ambiguity, int override,
               int no_free, int multiple_option,
               const char *long_opt, char short_opt,
               const char *additional_error)
{
  char *stop_char = 0;
  const char *val = value;
  int found;
  FIX_UNUSED (field);

  stop_char = 0;
  found = 0;

  if (!multiple_option && prev_given && (*prev_given || (check_ambiguity && *field_given)))
    {
      if (short_opt != '-')
        fprintf (stderr, "%s: `--%s' (`-%c') option given more than once%s\n", 
               package_name, long_opt, short_opt,
               (additional_error ? additional_error : ""));
      else
        fprintf (stderr, "%s: `--%s' option given more than once%s\n", 
               package_name, long_opt,
               (additional_error ? additional_error : ""));
      return 1; /* failure */
    }

  FIX_UNUSED (default_value);
    
  if (field_given && *field_given && ! override)
    return 0;
  if (prev_given)
    (*prev_given)++;
  if (field_given)
    (*field_given)++;
  if (possible_values)
    val = possible_values[found];

  switch(arg_type) {
  case ARG_FLAG:
    *((int *)field) = !*((int *)field);
    break;
  default:
    break;
  };


  /* store the original value */
  switch(arg_type) {
  case ARG_NO:
  case ARG_FLAG:
    break;
  default:
    if (value && orig_field) {
      if (no_free) {
        *orig_field = value;
      } else {
        if (*orig_field)
          free (*orig_field); /* free previous string */
        *orig_field = gengetopt_strdup (value);
      }
    }
  };

  return 0; /* OK */
}


int
cmdline_parser_internal (
  int argc, char **argv, struct gengetopt_args_info *args_info,
                        struct cmdline_parser_params *params, const char *additional_error)
{
  int c;	/* Character of the parsed option.  */

  int error_occurred = 0;
  struct gengetopt_args_info local_args_info;
  
  int override;
  int initialize;
  int check_required;
  int check_ambiguity;
  
  package_name = argv[0];
  
  override = params->override;
  initialize = params->initialize;
  check_required = params->check_required;
  check_ambiguity = params->check_ambiguity;

  if (initialize)
    cmdline_parser_init (args_info);

  cmdline_parser_init (&local_args_info);

  optarg = 0;
  optind = 0;
  opterr = params->print_errors;
  optopt = '?';

  while (1)
    {
      int option_index = 0;

      static struct option long_options[] = {
        { "help",	0, NULL, 'h' },
        { "version",	0, NULL, 'V' },
        { "decode",	0, NULL, 'd' },
        { "lookup",	0, NULL, 'l' },
        { "register",	0, NULL, 'r' },
        { "tr46t",	0, NULL, 'T' },
        { "tr46nt",	0, NULL, 'N' },
        { "no-tr46",	0, NULL, 0 },
        { "usestd3asciirules",	0, NULL, 0 },
        { "debug",	0, NULL, 0 },
        { "quiet",	0, NULL, 0 },
        { 0,  0, 0, 0 }
      };

      c = getopt_long (argc, argv, "hVdlrTN", long_options, &option_index);

      if (c == -1) break;	/* Exit from `while (1)' loop.  */

      switch (c)
        {
        case 'h':	/* Print help and exit.  */
        
        
          if (update_arg( 0 , 
               0 , &(args_info->help_given),
              &(local_args_info.help_given), optarg, 0, 0, ARG_NO,
              check_ambiguity, override, 0, 0,
              "help", 'h',
              additional_error))
            goto failure;
          cmdline_parser_free (&local_args_info);
          return 0;
        
          break;
        case 'V':	/* Print version and exit.  */
        
        
          if (update_arg( 0 , 
               0 , &(args_info->version_given),
              &(local_args_info.version_given), optarg, 0, 0, ARG_NO,
              check_ambiguity, override, 0, 0,
              "version", 'V',
              additional_error))
            goto failure;
          cmdline_parser_free (&local_args_info);
          return 0;
        
          break;
        case 'd':	/* Decode (punycode) domain name.  */
        
        
          if (update_arg( 0 , 
               0 , &(args_info->decode_given),
              &(local_args_info.decode_given), optarg, 0, 0, ARG_NO,
              check_ambiguity, override, 0, 0,
              "decode", 'd',
              additional_error))
            goto failure;
        
          break;
        case 'l':	/* Lookup domain name (default).  */
        
        
          if (update_arg( 0 , 
               0 , &(args_info->lookup_given),
              &(local_args_info.lookup_given), optarg, 0, 0, ARG_NO,
              check_ambiguity, override, 0, 0,
              "lookup", 'l',
              additional_error))
            goto failure;
        
          break;
        case 'r':	/* Register label.  */
        
        
          if (update_arg( 0 , 
               0 , &(args_info->register_given),
              &(local_args_info.register_given), optarg, 0, 0, ARG_NO,
              check_ambiguity, override, 0, 0,
              "register", 'r',
              additional_error))
            goto failure;
        
          break;
        case 'T':	/* Enable TR46 transitional processing.  */
        
        
          if (update_arg((void *)&(args_info->tr46t_flag), 0, &(args_info->tr46t_given),
              &(local_args_info.tr46t_given), optarg, 0, 0, ARG_FLAG,
              check_ambiguity, override, 1, 0, "tr46t", 'T',
              additional_error))
            goto failure;
        
          break;
        case 'N':	/* Enable TR46 non-transitional processing.  */
        
        
          if (update_arg((void *)&(args_info->tr46nt_flag), 0, &(args_info->tr46nt_given),
              &(local_args_info.tr46nt_given), optarg, 0, 0, ARG_FLAG,
              check_ambiguity, override, 1, 0, "tr46nt", 'N',
              additional_error))
            goto failure;
        
          break;

        case 0:	/* Long option with no short option */
          /* Disable TR46 processing.  */
          if (strcmp (long_options[option_index].name, "no-tr46") == 0)
          {
          
          
            if (update_arg((void *)&(args_info->no_tr46_flag), 0, &(args_info->no_tr46_given),
                &(local_args_info.no_tr46_given), optarg, 0, 0, ARG_FLAG,
                check_ambiguity, override, 1, 0, "no-tr46", '-',
                additional_error))
              goto failure;
          
          }
          /* Enable STD3 ASCII rules.  */
          else if (strcmp (long_options[option_index].name, "usestd3asciirules") == 0)
          {
          
          
            if (update_arg((void *)&(args_info->usestd3asciirules_flag), 0, &(args_info->usestd3asciirules_given),
                &(local_args_info.usestd3asciirules_given), optarg, 0, 0, ARG_FLAG,
                check_ambiguity, override, 1, 0, "usestd3asciirules", '-',
                additional_error))
              goto failure;
          
          }
          /* Print debugging information.  */
          else if (strcmp (long_options[option_index].name, "debug") == 0)
          {
          
          
            if (update_arg((void *)&(args_info->debug_flag), 0, &(args_info->debug_given),
                &(local_args_info.debug_given), optarg, 0, 0, ARG_FLAG,
                check_ambiguity, override, 1, 0, "debug", '-',
                additional_error))
              goto failure;
          
          }
          /* Silent operation.  */
          else if (strcmp (long_options[option_index].name, "quiet") == 0)
          {
          
          
            if (update_arg((void *)&(args_info->quiet_flag), 0, &(args_info->quiet_given),
                &(local_args_info.quiet_given), optarg, 0, 0, ARG_FLAG,
                check_ambiguity, override, 1, 0, "quiet", '-',
                additional_error))
              goto failure;
          
          }
          
          break;
        case '?':	/* Invalid option.  */
          /* `getopt_long' already printed an error message.  */
          goto failure;

        default:	/* bug: option not considered.  */
          fprintf (stderr, "%s: option unknown: %c%s\n", CMDLINE_PARSER_PACKAGE, c, (additional_error ? additional_error : ""));
          abort ();
        } /* switch */
    } /* while */




  cmdline_parser_release (&local_args_info);

  if ( error_occurred )
    return (EXIT_FAILURE);

  if (optind < argc)
    {
      int i = 0 ;
      int found_prog_name = 0;
      /* whether program name, i.e., argv[0], is in the remaining args
         (this may happen with some implementations of getopt,
          but surely not with the one included by gengetopt) */

      i = optind;
      while (i < argc)
        if (argv[i++] == argv[0]) {
          found_prog_name = 1;
          break;
        }
      i = 0;

      args_info->inputs_num = argc - optind - found_prog_name;
      args_info->inputs =
        (char **)(malloc ((args_info->inputs_num)*sizeof(char *))) ;
      while (optind < argc)
        if (argv[optind++] != argv[0])
          args_info->inputs[ i++ ] = gengetopt_strdup (argv[optind-1]) ;
    }

  return 0;

failure:
  
  cmdline_parser_release (&local_args_info);
  return (EXIT_FAILURE);
}