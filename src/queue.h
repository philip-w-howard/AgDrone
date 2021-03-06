/*
 * queue.h
 *
 *  Created on: Jul 7, 2015
 *      Author: philhow
 */

#pragma once

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Items that get queued
typedef struct item_qs
{
    void *data;                 // location for user's data
    struct item_qs *next;
} item_qt;

// Structure for actual queue
typedef struct
{
    item_qt *head;
    item_qt *tail;
    int is_open;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
} queue_t;

//********************************
queue_t *queue_create();
void queue_insert(queue_t *q, void *data);
void *queue_remove(queue_t *q);
void *queue_remove_timed(queue_t *q, int msecs);
void queue_mark_closed(queue_t *q);
int queue_is_open(queue_t *q);
void queue_close(queue_t *q);
void queue_signal_waiters(queue_t *q);

#ifdef __cplusplus
}
#endif

