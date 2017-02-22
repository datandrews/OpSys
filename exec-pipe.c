#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[], char *envp[]) {
  
  int ififo[2];
  if ( pipe(ififo) < 0 ) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  int ofifo[2];
  if ( pipe(ofifo) < 0 ) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  int pid = fork();
  if ( pid < 0 ) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if ( pid == 0 ) {
    if ( dup2(ififo[0], 0) < 0 ) {
      perror("dup2");
      exit(EXIT_FAILURE);
    }

    if ( dup2(ofifo[1], 1) < 0 ) {
      perror("dup2");
      exit(EXIT_FAILURE);
    }

    close(ififo[1]);
    close(ofifo[0]);

    char *args[] = {"bc", NULL};

    execve("/usr/bin/bc", args, envp);
    perror("exec");
    exit(EXIT_FAILURE);
  }

  char wstr[100];
  char rstr[100];
  for ( int i = 0 ; i < 100 ; i++ ) {
    
    sprintf(bcstr, "6 * %d\n", i);
    int slen = strlen(bcstr);
    if ( write(ififo[1], bcstr, slen) < slen ) {
      perror("write");
    }
  }
  
  close(ififo[1]);
  
  wait(NULL);

  printf("The child is dead.\n");
}
