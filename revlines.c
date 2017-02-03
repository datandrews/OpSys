#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <stdio.h>

#define BUFSZ 10000

char *progname;

int
main(int argc, char *argv[]) {
  int fd;

  char bufin[BUFSZ];
  int bufend;
  
  char bufout[BUFSZ];
  
  int nchar;

  int done;

  progname = argv[0];
  
  if ( argc != 2 ) {
    fprintf(stderr, "Usage: %s <file>\n", progname);
    exit(1);
  }

  if ( (fd = open(argv[1], O_RDONLY)) < 0 ) {
    perror(progname);
    exit(2);
  }

  bufend = 0;
  done = 0;
  while ( ! done ) {
    if ( (nchar = read(fd, &(bufin[bufend]), BUFSZ - bufend)) < 0 ) {
      perror(progname);
      exit(3);
    }

    if ( nchar < BUFSZ - bufend ) {
      bufin[bufend + nchar + 1] = '\n'; // Just in case there was no terminating newline in file
      done = 1;
    }

    int lastnl = -1;
    for ( int scan = 0 ; scan < BUFSZ ; scan++ ) {
      
      if ( bufin[scan] == '\n' ) {
	
	// Found end of the line, so reverse and print
	for ( int i = scan - 1 ; i > lastnl ; i-- )
	  bufout[scan - i - 1] = bufin[i];
	bufout[scan - lastnl - 1] = '\n';
	write(1, bufout, scan-lastnl);

	lastnl = scan;
      }
      
    }

    // Check to see that we found a newline somewhere in the buffer!
    if ( lastnl == -1 ) {
      fprintf(stderr, "%s: buffer insufficiently big for one of the lines.\n", progname);
      exit(4);
    }

    // Move the remaining buffer data down to the end
    for ( int i = lastnl + 1 ; i < BUFSZ ; i++ )
      bufin[i - lastnl - 1] = bufin[i];
    bufend = BUFSZ - lastnl - 1;

  }
}
