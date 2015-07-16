/*
 * queue.c
 *
 *  Created on: Jul 7, 2015
 *      Author: philhow
 */

#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

#include "queue.h"

//********************************
queue_t *queue_create()
{
    queue_t *q;

    q = (queue_t *)malloc(sizeof(queue_t));
    assert(q != NULL);

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);

    pthread_mutex_lock(&q->lock);

    q->head = NULL;
    q->tail = NULL;
    q->is_open = 1;

    pthread_mutex_unlock(&q->lock);

    return q;
}
//********************************
void queue_close(queue_t *q)
{
    free(q);
}
//********************************
void queue_insert(queue_t *q, void *data)
{
    assert(q != NULL);

    item_qt *item = (item_qt *)malloc(sizeof(item_qt));
    assert(item != NULL);

    pthread_mutex_lock(&q->lock);
    item->data = data;
    item->next = NULL;
    if (q->tail != NULL) q->tail->next = item;
    q->tail = item;
    if (q->head == NULL) q->head = item;
    pthread_mutex_unlock(&q->lock);
    pthread_cond_signal(&q->cond);
}
//********************************
void *queue_remove(queue_t *q)
{
    item_qt *item;
    char *data;

    assert(q != NULL);

    pthread_mutex_lock(&q->lock);
    while (q->head == NULL && q->is_open)
    {
        pthread_cond_wait(&q->cond, &q->lock);
    }

    if (q->head == NULL)
    {
        if (!q->is_open) pthread_cond_broadcast(&q->cond);
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    item = q->head;
    q->head = item->next;
    if (q->head == NULL) q->tail = NULL;

    if (!q->is_open) pthread_cond_broadcast(&q->cond);
    pthread_mutex_unlock(&q->lock);

    data = item->data;

    free(item);

    return data;
}
//**********************************
void queue_mark_closed(queue_t *q)
{
    pthread_mutex_lock(&q->lock);
    q->is_open = 0;
    pthread_mutex_unlock(&q->lock);
    pthread_cond_broadcast(&q->cond);
}
//**********************************
int queue_is_open(queue_t *q)
{
    int is_closed;

    if (q == NULL) return 0;

    pthread_mutex_lock(&q->lock);
    is_closed = (!q->is_open && q->head == NULL) ;
    pthread_mutex_unlock(&q->lock);

    return !is_closed;
}



