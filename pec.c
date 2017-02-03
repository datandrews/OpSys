#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

typedef unsigned long count_t;

typedef struct procinfo {
  int pid;
  int fdout;
  int fileno;
  struct procinfo *next;
} procinfo;


count_t count(int fd);
count_t readCount(int status, int fd);

char *progname;  // Global variable for usage function
void usage();

int
main(int argc, char *argv[]) {
  progname = argv[0];

  // The default number of processors
#ifdef _SC_NPROCESSORS_ONLN
  int nproc = sysconf(_SC_NPROCESSORS_ONLN);
#else
  int nproc = 1;
#endif

  // Parse the arguments
  // Keep track of where the file list starts...
  char nfiles = argc - 1;
  char **files = &(argv[2]);

  // Check for -P flag
  if ( argc > 1 && strcmp(argv[1], "-P") == 0 ) {

    // Could have nothing (or a empty arg) after -P
    if ( argc < 3 || argv[2][1] == '\0' ) {
      usage();
    }

    // Convert the argument and check for errors
    char **endptr;
    int reqproc = strtol(argv[2], endptr, 0);

    if ( **endptr != '\0' || reqproc < 1 ) { // Found a bad number
      fprintf(stderr, "'%s' is not a valid number of processes.\n", argv[2]);
      usage();
    }

    nproc = reqproc;

    // The rest of the arguments are files
    nfiles = argc - 3;
    files = &(argv[4]);
  }

  // Special case for no files --- read standard input (though we're ignoring
  // the number of processes requested here).
  if ( nfiles == 0 ) {
    count_t ec = count(0);
    printf("stdin: %lu\n", ec);
    exit(0);
  }

  procinfo *plist = NULL;
  int nchild = 0, fn = 0;
  int ecounts[nfiles];
  

  // We don't quit until we've worked through the files and there aren't any more children to wait for...
  while ( fn < nfiles || nchild > 0 ) {

    // First harvest any children that may have finished...
    int cpid, status;
    while ( ( cpid = waitpid(-1, &status, WNOHANG) ) > 0 ) {

      if ( plist == NULL ) {
	// Hmm... We got a child that isn't on the list?
	fprintf(stderr, "%s: Child died, but no children on the list?\n", progname);
	continue; // Maybe we should die on this error?
      }
      
      // Special case for the first on the list...
      if ( plist->pid == cpid ) {
	ecounts[plist->fileno] = readCount(status, plist->fdout);
	plist = plist->next;
	nchild = nchild - 1;
	continue;
      }

      // Find the process in the list
      procinfo *last = plist, *this = plist->next;
      while ( this != NULL ) {
	if ( this->pid == cpid ) {
	  ecounts[this->fileno] = readCount(status, plist->fdout);
	  last->next = this->next;
	  nchild = nchild - 1;
	  
	  free((void *) this);
	  this = last;  // To show we found somebody, this != NULL
	  break;
	}

	last = this;
	this = this->next;
      }

      // We reached the end of the list and didn't find our process???
      if ( this == NULL ) {
	fprintf(stderr, "%s: Child exited, but we didn't have a record of it?\n", progname);
	continue; // Maybe we should die on this error?
      }
    }

    // Now try to start a child process (if we can)
    if ( nchild < nproc && fn < nfiles) {

      // Open the file, if we can
      int fd;
      if ( (fd = open(files[fn], O_RDONLY)) < 0 ) {
	// Error, so check for those that we can recover from
	if ( errno == EMFILE || errno == ENFILE ) {
	  sleep(1);
	  continue;
	}

	// Not a recoverable error, so make a little message, and skip this file
	fprintf(stderr, "%s: Can't open %s: %s", progname, files[fn], strerror(errno));
	fn = fn + 1;
	continue;
      }
      
      // Open a pipe, if we can
      int fifo[2];
      if ( pipe(fifo) < 0 ) {
	// Error, so close the file and wait a little while to try again...
	close(fd);
	sleep(1);
	continue;
      }

      // Start a child, if we can
      int pid;
      if ( (pid = fork()) < 0 ) {
	// Error, so close the file & pipe and wait a little while to try again...
	close(fd);
	close(fifo[0]);
	close(fifo[1]);
	sleep(1);
	continue;
      }

      if ( pid == 0 ) {
	// The child process

	// Close the read end of the pipe
	close(fifo[0]);

	// Count up the e's
	count_t es = count(fd);

	// Write to the pipe, if we can
	if ( write(fifo[1], &es, sizeof(count_t)) != sizeof(count_t) ) {
	  exit(EXIT_FAILURE);
	}

	// Clean up and exit
	close(fifo[1]);
	exit(EXIT_SUCCESS);
      }

      // Parent process

      // Close the write end of the pipe
      close(fifo[1]);

      // Create a new record
      procinfo *new = (procinfo *) malloc(sizeof(procinfo));
      new->pid = pid;
      new->fdout = fifo[0];
      new->fileno = fn;
      new->next = plist;
      plist = new;

      // Move on to the next file...
      fn = fn + 1;
    }
  }

  for ( fn = 0 ; fn < nfiles ; fn++ ) {
    printf("%10d %s\n", ecounts[fn], files[fn]);
  }

  exit(EXIT_SUCCESS);
}

// Count the 'e's in fd
count_t count(int fd) {
  char buf;
  int ecount = 0;
  
  while ( read(fd, &buf, 1) > 0 ) {
    if ( buf == 'e' ) {
      ecount++;
    }
  }

  return ecount;
}


// Get the count from the stream and close it...
count_t readCount(int status, int fd) {
  count_t retval = 0;
  
  if ( status != EXIT_SUCCESS ) {
    retval = 0;
  }
  else if ( read(fd, &retval, sizeof(count_t)) != sizeof(count_t) ) {
    retval = 0;
  }

  close(fd);

  return retval;
}


void usage() {
  fprintf(stderr, "Usage: %s [-P <nproc>] [<file> ...]\n", progname);
  exit(EXIT_FAILURE);
}
