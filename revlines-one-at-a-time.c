#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <stdio.h>

#define BUFSZ 100

char *progname;

int
main(int argc, char *argv[]) {
  int fd;

  char bufin, bufout[BUFSZ];
  int nchar, bufoutpos;

  progname = argv[0];
  
  if ( argc != 2 ) {
    fprintf(stderr, "Usage: %s <file>\n", progname);
    exit(1);
  }

  if ( (fd = open(argv[1], O_RDONLY)) < 0 ) {
    perror(progname);
    exit(1);
  }

  bufoutpos = 0;
  
  while ( (nchar = read(fd, &bufin, 1)) > 0 ) {

    if ( bufin == '\n' || bufin == '\0' ) {

      // Found the end of the word, so reverse it...
      for ( int i = bufoutpos-1 ; i >= 0 ; i-- )
	write(1, &(bufout[i]), 1);

      write(1, "\n", 1);

      // Reset the output buffer...
      bufoutpos = 0;
    }

    else {
      bufout[bufoutpos++] = bufin;

      if ( bufoutpos > BUFSZ-1 ) {
	fprintf(stderr, "Error: buffer to small for one of the words.\n");
	exit(3);
      }
    }
  }
    
  if ( nchar < 0 ) {
    perror("Error: ");
    exit(2);
  }
  
  // Don't forget the last word!
  bufout[bufoutpos] = '\n';
  write(1, bufout, bufoutpos);

  exit(0);
}
