#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

// Do we talk a lot?  This is changed by getopt
static int verbose_flag = 0;

// The variables that getopt uses, as defined in getopt.h
// extern char *optarg; --- a pointer to the argument of the option, if it has one
// extern int optind; --- the index into argv that getopt last worked on
// extern int opterr; --- set this to 1 to turn off automatic error messages
// extern int optopt; --- Used for errors: the "bad" option

// Our options to parse
static struct option long_options[] =
  {
    // A flag and its inverse
    {"verbose", no_argument,       &verbose_flag, 1},
    {"brief",   no_argument,       &verbose_flag, 0},

    // Some simple flags that tell us to do something
    {"add",     no_argument,       0, 'a'},
    {"append",  no_argument,       0, 'b'},

    // Some flags that need arguments
    {"delete",  required_argument, 0, 'd'},
    {"create",  required_argument, 0, 'c'},
    {"file",    required_argument, 0, 'f'},

    // A flag that could have arguments
    {"gravy",   optional_argument, 0, 'g'},
    
    // Must end with a zero'd out entry
    {NULL, 0, NULL, 0}
  };

int
main(int argc, char *argv[])
{
  int c;

  // We parse all the arguments by calling getopt_long repeatedly
  while (1)
    {
      // getopt_long stores the option index here.
      int option_index = 0;

      c = getopt_long(argc, argv, "abc:d:f:g", long_options, &option_index);

      // No more options to parse, so quit
      if (c == -1)
        break;

      switch (c) {
      case 0:
	// Don't do anything on the options that automatically adjust a flag
	if (long_options[option_index].flag != 0)
	  break;

	// But otherwise print out some information
	printf ("option %s", long_options[option_index].name);
	if (optarg)
	  printf(" with arg %s", optarg);
	
	printf("\n");
	break;

      case 'a':
	puts("option -a\n");
	break;
	
      case 'b':
	puts("option -b\n");
	break;
	
      case 'c':
	printf("option -c with value `%s'\n", optarg);
	break;
	
      case 'd':
	printf("option -d with value `%s'\n", optarg);
	break;
	
      case 'f':
	printf("option -f with value `%s'\n", optarg);
	break;
	
      case 'g':
	printf("option -g");
	if (optarg)
	  printf(" with arg %s", optarg);
	putchar('\n');

      case '?':
	// getopt_long already printed an error message, but we could provide a message here if we wanted...
	break;

      default:
	// Something went badly wrong
	abort();
      }
    }


  // After parsing, we report if the verbose flag ever got set..
  if (verbose_flag)
    puts("verbose flag is set");
  else
    puts("verbose flag is not set");

  // Print out the remaining arguments
  if (optind < argc) {
    printf ("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    putchar('\n');
  }

  exit(0);
}
