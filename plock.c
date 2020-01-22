/**************************************
  Rebecca Edson
  CPSC 3220 Project 2 - Priority Lock
  June 3, 2019
**************************************/

#include "plock.h"


plock_t *plock_create() {

  plock_t *lock = malloc(sizeof(plock_t)); // Allocate memory for lock

  // Initialize struct variables
  lock->value = FREE;
  pthread_mutex_init(&lock->mlock, NULL);
  lock->head = NULL;

  return lock;

}


void plock_destroy(plock_t *lock) {

  node_t *temp;

  // Destroy nodes on waiting list
  while(lock->head != NULL) {
    temp = lock->head->next; // Temp set to head's next node
    pthread_cond_destroy(&lock->head->waitCV); // Destroy CV
    free(lock->head); // Destroy head node
    lock->head = temp; // Head set to next node
  }

  // Destroy lock
  pthread_mutex_destroy(&lock->mlock);
  free(lock);

}


void plock_enter(plock_t *lock, int priority) {

    pthread_mutex_lock(&lock->mlock); // Enter critical section

    // Create node instance of thread
    node_t *thread = malloc(sizeof(node_t));
    thread->priority = priority;
    pthread_cond_init(&thread->waitCV, NULL);
    thread->next = NULL;


    /* If no thread waiting OR thread priority is not less than head's,
       thread becomes head                                              */
    if(lock->head == NULL || thread->priority >= lock->head->priority) {
      thread->next = lock->head;
      lock->head = thread;
    }
    else {
      /* Iterate through waiting list until end OR thread has higher
         priority than temp's next node                                     */
      node_t *temp = lock->head;
      while(temp->next != NULL && thread->priority < temp->next->priority)
        temp = temp->next;

      thread->next = temp->next; // Thread's next node set to temp's
      temp->next = thread; // Replace temp's next node with thread
    }

    /* Wait on thread's condition variable, until thread is highest
       priority node AND lock is free                               */
    while(lock->head != thread || lock->value != FREE)
      pthread_cond_wait(&thread->waitCV, &lock->mlock);

    /* Head becomes next node, destroy thread node and its
       condition variable, and set lock to busy                 */
    lock->head = lock->head->next;
    pthread_cond_destroy(&thread->waitCV);
    free(thread);
    lock->value = BUSY;

    pthread_mutex_unlock(&lock->mlock); // Exit critical section

}


void plock_exit(plock_t *lock) {

  pthread_mutex_lock(&lock->mlock); // Enter critical section

  lock->value = FREE; // Set lock to free

  // If waiting thread, signal it
  if(lock->head != NULL)
    pthread_cond_signal(&lock->head->waitCV);

  pthread_mutex_unlock(&lock->mlock); // Exit critical section

}

