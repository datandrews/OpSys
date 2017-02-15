#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/shm.h>

int
main(int argc, char *argv[]) {
  
#ifdef _SC_NPROCESSORS_ONLN
  int nproc = sysconf(_SC_NPROCESSORS_ONLN);
#else
  int nproc = 1;
#endif

  if ( argc != 2 ) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int fd = open(argv[1], O_RDONLY);
  if ( fd < 0 ) {
    // Can't open file
    perror(argv[1]);
    exit(EXIT_FAILURE);
  }

  struct stat fs;
  if ( fstat(fd, &fs) < 0 ) {
    // Can't stat file
    perror(argv[1]);
    exit(EXIT_FAILURE);
  }

  if ( ! S_ISREG(fs.st_mode) ) {
    // Not regular file
    fprintf(stderr, "%s is not a regular file.\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  // Map the file and close it
  char *mp = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if ( mp == MAP_FAILED ) {
    // Can't map the file
    perror("mmap");
    exit(EXIT_FAILURE);
  }
  close(fd);

  sem_t mutex;
  if ( sem_init(&mutex, 1, 1) < 0 ) {
    // Can't make a semaphore
    perror("Semaphore initialzation");
    exit(EXIT_FAILURE);
  }
  
  // Create memory to be shared...
  int eshm = shmget(IPC_PRIVATE, sizeof(int), 0700 | IPC_CREAT);
  if ( eshm < 0 ) {
    // Can't make shared memory
    perror("Shared memory allocation");
    exit(EXIT_FAILURE);
  }

  // Attached the shared memory segment
  int *etotal = (int *) shmat(eshm, NULL, 0);
  if ( etotal == (void *) -1 ) {
    perror("Shared memory attachment");
    exit(EXIT_FAILURE);
  }

  // Initialize the count...
  *etotal = 0;
      
  // Divy up the work between the processes...
  int bytesperproc = fs.st_size / nproc + 1;

  char *cur;
  int p;
  for ( p = 0, cur = mp ; p < nproc ; p++, cur += bytesperproc ) {

    int pid = fork();
    if ( pid < 0 ) {
      perror("fork");
      exit(EXIT_FAILURE);
    }

    if ( pid == 0 ) {
      // The child
      int ecount = 0;
      char *end;

      if ( p < nproc - 1 ) {
	end = cur + bytesperproc;
      }
      else {
	end = mp + fs.st_size;
      }

      //      printf("Started child %d. Working from %p to %p...\n", p, (void *) cur, (void *) end);

      while ( cur != end )
	ecount += *cur++ == 'e';

      //      printf("Child %d read %d e's\n", p, ecount);

      // The total count is a shared resource, so coordinate among the children...
      sem_wait(&mutex);
      *etotal += ecount;
      sem_post(&mutex);
      
      exit(EXIT_SUCCESS);
    }
  }

  // Parent waits for all children, and receives their messages
  int retval;

  while ( wait(&retval) > 0 ) {
    if ( retval != EXIT_SUCCESS ) {
      fprintf(stderr, "A child ended with exit value = %d. Report count set to 0.\n", retval);
      continue;
    }
  }

  printf("Number of e's: %d\n", *etotal);
}
