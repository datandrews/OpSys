#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int
main(int argc, char *argv[]) {

  char *mp;

  if ( argc != 2 ) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int fd = open(argv[1], O_RDONLY);
  if ( fd < 0 ) {
    perror(argv[1]);
    exit(EXIT_FAILURE);
  }

  struct stat fs;
  if ( fstat(fd, &fs) < 0 ) {
    perror(argv[1]);
    exit(EXIT_FAILURE);
  }

  if ( ! S_ISREG(fs.st_mode) ) {
    fprintf(stderr, "%s is not a regular file.\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  mp = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if ( mp == MAP_FAILED ) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  int ecount = 0;
  char *cur = mp;
  char *end = mp + fs.st_size;

  printf("Reading from %p to %p...", (void *) cur, (void *) end);
  while ( cur != end )
    ecount += *cur++ == 'e';

  printf("Number of e's: %d\n", ecount);
}
