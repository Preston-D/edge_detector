// Preston Duffield
// In this solution, if a thread cannot acquire both locks, it releases the one it has and tries again.
#include <pthread.h>
#include <err.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t mutex_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_b = PTHREAD_MUTEX_INITIALIZER;

void lock_both_mutexes()
{
  while (1)
  {
    if (pthread_mutex_trylock(&mutex_a) == 0)
    {
      if (pthread_mutex_trylock(&mutex_b) == 0)
      {
        return; // both locks acquired, break out of the loop
      }
      pthread_mutex_unlock(&mutex_a); // if can't get mutex_b, release mutex_a and try again
    }
    // sleep for a bit to prevent starving other threads
    struct timespec sleepTime;
    sleepTime.tv_sec = 0;
    sleepTime.tv_nsec = 1000000; // 1 ms
    nanosleep(&sleepTime, NULL);
  }
}

void *thread1(void *arg)
{
  while (1)
  {
    lock_both_mutexes();
    printf("[%lu]thread 1 is running! \n", time(NULL));
    pthread_mutex_unlock(&mutex_b);
    pthread_mutex_unlock(&mutex_a);
  }
  return NULL;
}

void *thread2(void *arg)
{
  while (1)
  {
    lock_both_mutexes();
    printf("[%lu]thread 2 is running! \n", time(NULL));
    pthread_mutex_unlock(&mutex_b);
    pthread_mutex_unlock(&mutex_a);
  }
  return NULL;
}
// Rest of the code remains same
int main()
{
  pthread_t tid1, tid2;
  int status;

  status = pthread_create(&tid1, NULL, thread1, NULL);
  if (status != 0)
    errx(status, "thread 1");

  status = pthread_create(&tid2, NULL, thread2, NULL);
  if (status != 0)
    errx(status, "thread 2");

  status = pthread_join(tid1, NULL);
  if (status != 0)
    errx(status, "join thread1");

  status = pthread_join(tid2, NULL);
  if (status != 0)
    errx(status, "join thread2");
}
