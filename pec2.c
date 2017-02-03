#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int count();

int
main(int argc, char *argv[]) {
  int nfiles = argc - 1;
  
  if ( nfiles == 0 ) {
    int ec = count(0);
    printf("%10d stdin\n", ec);
    exit(0);
  }

  char **files = &(argv[1]);

  int *fds = (int *) calloc(nfiles, sizeof(int));
  for ( int fn = 0 ; fn < nfiles ; fn++ ) {
    fds[fn] = open(files[fn], O_RDONLY);

    if ( fds[fn] < 0 ) {
      fprintf(stderr, "%s: can't open %s. ", argv[0], files[fn]);
      perror(NULL);
      exit(1);
    }
  }

  // We've successfully opened all the files, so start the children
  int *procs = (int *) calloc(nfiles, sizeof(int));
  for ( int fn = 0 ; fn < nfiles ; fn++ ) {
    if ( (procs[fn] = fork()) < 0 ) {
      fprintf(stderr, "Can't start child process: ");
      perror(NULL);
      exit(2);
    }

    if ( procs[fn] == 0 ) {
      int ec = count(fds[fn]);
      printf("%10d %s\n", ec, files[fn]);
      exit(0);
    }
  }

  exit(0);
}

// Count the 'e's in fd, and return a string documenting it
char *count(char *filename) {
  char buf;
  int ecount = 0;
  
  while ( read(fd, &buf, 1) > 0 ) {
    if ( buf == 'e' ) {
      ecount++;
    }
  }

  return ecount;
}
