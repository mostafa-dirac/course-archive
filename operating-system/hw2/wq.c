#include <stdlib.h>
#include "wq.h"
#include "utlist.h"

pthread_mutex_t mutual_exclusion = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t is_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t is_not_full = PTHREAD_COND_INITIALIZER;

/* Initializes a work queue WQ. */
void wq_init(wq_t *wq, int num_thread) {

  /* TODO: Make me thread-safe! */
  pthread_mutex_lock(&mutual_exclusion);
  wq->size = 0;
  wq->head = NULL;
  wq->max_queue_size = num_thread;
  pthread_mutex_unlock(&mutual_exclusion);
}

/* Remove an item from the WQ. This function should block until there
 * is at least one item on the queue. */
int wq_pop(wq_t *wq) {

  /* TODO: Make me blocking and thread-safe! */
  pthread_mutex_lock(&mutual_exclusion);
  while(wq->size == 0){
    pthread_cond_wait(&is_not_empty, &mutual_exclusion);
  }
  wq_item_t *wq_item = wq->head;
  int client_socket_fd = wq->head->client_socket_fd;

  wq->size--;
  DL_DELETE(wq->head, wq->head);
  free(wq_item);
  pthread_cond_signal(&is_not_full);
  pthread_mutex_unlock(&mutual_exclusion);
  return client_socket_fd;
}

/* Add ITEM to WQ. */
void wq_push(wq_t *wq, int client_socket_fd) {

  /* TODO: Make me thread-safe! */
  pthread_mutex_lock(&mutual_exclusion);
  while(wq->size == wq->max_queue_size){
    pthread_cond_wait(&is_not_full, &mutual_exclusion);
  }

  wq_item_t *wq_item = calloc(1, sizeof(wq_item_t));
  wq_item->client_socket_fd = client_socket_fd;
  DL_APPEND(wq->head, wq_item);
  wq->size++;
  pthread_cond_signal(&is_not_empty);
  pthread_mutex_unlock(&mutual_exclusion);
}
