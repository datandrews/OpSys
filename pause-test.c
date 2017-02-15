#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/shm.h>

int
main() {
  int pid;


  // Get the shared memory for the mutex
  int shmmt = shmget(IPC_PRIVATE, sizeof(sem_t), 0700);
  sem_t *mutex = shmat(shmmt, NULL, 0);
  
  sem_init(mutex, 1, 1);

  if ( (pid = fork()) == 0 ) {

    while ( 1 ) {
      sem_wait(mutex);
      write(1, "Child...", 8);
      sleep(1);
      sem_post(mutex);
      write(1, "\tdone.\n", 7);
      sleep(1);
    }

    exit(EXIT_SUCCESS);
  }

  while ( 1 ) {
    sem_wait(mutex);
    write(1, "Parent...", 9);
    sleep(3);
    sem_post(mutex);
    write(1, "\tdone.\n", 7);
    sleep(3);
  }
  
  exit(EXIT_SUCCESS);
}
