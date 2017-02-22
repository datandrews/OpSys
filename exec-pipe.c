#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[], char *envp[]) {
  
  int fifo[2];
  if ( pipe(fifo) < 0 ) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  int pid = fork();
  if ( pid < 0 ) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if ( pid == 0 ) {
    if ( dup2(fifo[0], 0) < 0 ) {
      perror("dup2");
      exit(EXIT_FAILURE);
    }

    close(fifo[1]);

    char *args[] = {"wc", NULL};

    execve("/usr/bin/bc", args, envp);
    perror("exec");
    exit(EXIT_FAILURE);
  }

  for ( int i = 0 ; i < 100 ; i++ ) {
    if ( write(fifo[1], "6 * 9\n", 6) < 6 ) {
      perror("write");
    }
  }
  
  close(fifo[1]);
  
  wait(NULL);

  printf("The child is dead.\n");
}
