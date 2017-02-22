#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int total;

int
main() {
  printf("Parent: %d\n", &total);

  for ( int i = 0 ; i < 10 ; i++ ) {
    int pid;
    if ( (pid = fork()) == 0 ) {
      printf("Child: %d\n", &total);
      total = total + 1;
      printf("Child: %d\n", &total);
      exit(0);
    }
    else {
      wait(NULL);
    }
  }

  printf("Parent: %d\n", &total);
  printf("Total : %d\n", total);
}
