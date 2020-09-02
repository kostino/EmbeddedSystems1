/*
 *	File  : prod-cons.c
 *
 *	Title  : Producer/Consumer with function pointers.
 *
 *	Description  : A solution to the producer consumer problem using
 *		pthreads, where the producers add functions to be executed and the
 *    consumers execute said functions from a queue.
 *
 *	Author  : Kostas Triaridis
 *
 *	Date  : March 2020
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "Timer.h"

#define QUEUESIZE 10 //size of function pointer queue
#define LOOP 2000     //how many items each producer thread makes

void *producer (void *args);
void *consumer (void *args);


typedef struct {
  void * (*work)(void *);
  void * arg;
} workFunction;

typedef struct {
  workFunction* buf[QUEUESIZE];
  long head, tail;
  //finished gives us a way to notify consumers when production
  //is done like suggested in the forum: https://elearning.auth.gr/mod/forum/discuss.php?d=76697
  int full, empty, finished;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction *in);
void queueDel (queue *q, workFunction **out);
void fun1(int arg);
void fun2(int arg);
void fun3(int arg);
void fun4(int arg);
workFunction *workFunctionInit(void * (*funcPtr)(void *), void * funcArg);

//Global variables for timestamps and functions
void (*functions[4])(int) = {&fun1,&fun2,&fun3,&fun4};
int resIndex=0;
double* results;

//we need to hold completed consumers (those broken out of while(1)) to see when to stop broadcasting not empty.
int completedConsumers=0;

int main ()
{
  //set producer count for different benchmarks every time
  int p = 1;
  //we run for different number of consumers a lot of times to gather a lot of data
  for(int q = 1;q<=32;q*=2){
    for(int i=0;i<10;i++){
      // Create results array
      results = calloc(p*LOOP, sizeof(double));

      // Create a queue
      queue *fifo;
      fifo = queueInit ();
      if (fifo ==  NULL) {
        fprintf (stderr, "main: Queue Init failed.\n");
        exit (1);
      }

      // Spawn threads
      pthread_t *pro = calloc(p, sizeof(pthread_t));
      pthread_t *con = calloc(q, sizeof(pthread_t));
      for(int i = 0; i < p; i++){
        pthread_create (pro + i, NULL, producer, fifo);
      }
      for(int j = 0; j < q; j++){
        pthread_create (con + j, NULL, consumer, fifo);
      }

      // Join Producers
      for(int i = 0; i < p; i++){
        pthread_join (pro[i], NULL);
      }
      //after producers join we have to notify consumers
      fifo->finished=1;

      //join Consumers
      while (completedConsumers < q){
        pthread_cond_broadcast(fifo->notEmpty);
      }
      for(int i = 0; i < q; i++){
        pthread_join (con[i], NULL);
      }


      //write results to specific file
      FILE *fp;

      char filename[50];
      sprintf(filename, "Prod%d-Cons%d.csv", p, q);

      fp = fopen(filename, "a");

      for (int i = 0; i < p*LOOP; i++) {
        fprintf(fp, "%f\n ", results[i]);
      }
      fprintf(fp, "\n");
      fclose(fp);
      printf("%s file created\n", filename);
      //reset global vars for next run
      resIndex = 0;
      completedConsumers = 0;
      free(results);
      queueDelete(fifo);

    }
  }
  return 0;
}

void *producer (void *q)
{
  queue *fifo;
  int i;

  fifo = (queue *)q;

  for (i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    //select random function with random args to add to queue
    workFunction* w= workFunctionInit((void* (*)(void *))functions[rand()%4],(rand()%10) );
    queueAdd (fifo, w);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
  }
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  fifo = (queue *)q;
  struct timeval endTime;
  workFunction* w;

  while(1) {
    pthread_mutex_lock (fifo->mut);
    while ((fifo->empty)&&(fifo->finished==0)) {
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    //only break from the while when all producers are finished as suggested in the forum
    if ((fifo->empty)&&(fifo->finished==1)){
      completedConsumers++;
      pthread_mutex_unlock(fifo->mut);
      break;
    }
    queueDel (fifo, &w);
    //get end time and calculate total time
    //useful code from: https://stackoverflow.com/a/12722972
    gettimeofday(&endTime,NULL);
    void** structArgs = w->arg;
    struct timeval *startTime = (struct timeval*)structArgs[0];
    double time = (endTime.tv_sec - startTime->tv_sec) * 1000000.0;
    time += (endTime.tv_usec - startTime->tv_usec);
    //save result
    results[resIndex]=time;
    resIndex++;

    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);

    //execute function
    (w->work)(structArgs[1]);
    //always remember the free()  ()
    free(startTime);
    free(w);
  }
  return (NULL);
}

/*
  typedef struct {
  void * (*work)(void *);
  void * arg;
  } workFunction;
*/

workFunction *workFunctionInit(void * (*funcPtr)(void *), void * funcArg) {
  workFunction *w;
  w = (workFunction*)malloc(sizeof(workFunction));
  if (w == NULL) return (NULL);
  w->work = funcPtr;
  //get the time the struct was initialized using gettimeofday
  struct timeval *start = malloc(sizeof(struct timeval));
  gettimeofday(start, NULL);
  // Put start time in struct arguments for ease of use by consumer, as suggested in forum
  void** structArgs = malloc(2*sizeof(void*));
  structArgs[0] = start;
  structArgs[1] = funcArg;
  w->arg = structArgs;

  return w;
}

/*
  typedef struct {
  workFunction* buf[QUEUESIZE];
  long head, tail;
  int full, empty, allProdFin;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  } queue;
*/

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->finished = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);

  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q, workFunction* in)
{
  q->buf[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel (queue *q, workFunction **out)
{
  *out = q->buf[q->head];

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}

void fun1(int arg) {
  for(int i=0;i<arg;i++){
    printf("Hello, friend: %d\n", i);
    usleep(30);
  }
}

void fun2(int arg) {
  double sum=0;
  srand(time(NULL));
  for(int i=0;i<arg;i++){
    sum+=sin(rand()%10);
  }
  printf("%f\n",sum);
  usleep(30);
}

void fun3(int arg) {
  int sum=0;
  srand(time(NULL));
  for(int i=0;i<arg;i++){
    sum+=arg*(rand()%10);
  }
  printf("%d\n",sum);
  usleep(30);
}

void fun4(int arg) {
  double sum = sin((double)arg)+cos((double)arg)+tan((double)arg);
  printf("%f\n",sum);
  usleep(30);
}
