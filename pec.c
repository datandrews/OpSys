#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define BUFSIZE (16 * 1024)

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
  int nfiles = argc - 1;
  char **files = &(argv[1]);

  // Check for -P flag
  if ( argc > 1 && strcmp(argv[1], "-P") == 0 ) {

    // Could have nothing (or a empty arg) after -P
    if ( argc < 3 || argv[2][0] == '\0' ) {
      usage();
    }

    // Convert the argument and check for errors
    char *endptr;
    int reqproc = strtol(argv[2], &endptr, 0);

    if ( *endptr != '\0' || reqproc < 1 ) { // Found a bad number
      fprintf(stderr, "'%s' is not a valid number of processes.\n", argv[2]);
      usage();
    }

    nproc = reqproc;

    // The rest of the arguments are files
    nfiles = argc - 3;
    files = &(argv[3]);
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
  count_t ecounts[nfiles];

  for ( int i = 0 ; i < nfiles ; i++ ) {
    ecounts[i] = 0;
  }
  
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
	
	void *hold = plist;
	plist = plist->next;
	free(hold);
	
	nchild = nchild - 1;
	continue;
      }

      // Find the process in the list
      procinfo *last = plist, *this = plist->next;
      while ( this != NULL ) {
	if ( this->pid == cpid ) {
	  ecounts[this->fileno] = readCount(status, this->fdout);
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

      // Try to open the file if we can...
      int fd;
      if ( (fd = open(files[fn], O_RDONLY)) < 0 ) {
	// Error, so check for those that we can recover from
	if ( errno == EMFILE || errno == ENFILE ) {
	  sleep(1);
	  continue;
	}

	// Not a recoverable error, so make a little message, and skip this file
	fprintf(stderr, "%s: %s: %s\n", progname, files[fn], strerror(errno));
	ecounts[fn] = 0;
	files[fn][0] = '\0';  // Used to skip the output later...
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
	close(fd);

	// Write to the pipe, if we can
	if ( write(fifo[1], &es, sizeof(count_t)) != sizeof(count_t) ) {
	  close(fifo[1]);
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

      // Add to the list
      new->next = plist;
      plist = new;

      // Increase the child count
      nchild = nchild + 1;

      // Move on to the next file...
      fn = fn + 1;
    }
  }

  count_t total = 0;
  for ( fn = 0 ; fn < nfiles ; fn++ ) {
    if ( files[fn][0] != '\0' ) {
      printf("%10lu %s\n", ecounts[fn], files[fn]);
      total += ecounts[fn];
    }
  }

  if ( nfiles > 1 ) {
    printf("%10lu total\n", total);
  }

  exit(EXIT_SUCCESS);
}

// Count the 'e's in fd
count_t count(int fd) {
  char buf[BUFSIZE + 1];
  int ecount = 0;

  // Let the kernel know how we'll be using the data...
  posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_FADV_NOREUSE);

  int bytes_read;

  while ( (bytes_read = read(fd, buf, BUFSIZE)) > 0 ) {
    
    char *p = buf;
    char *end = p + bytes_read;
    
    while ( p != end ) {
      ecount += *p++ == 'e';
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
