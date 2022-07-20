#ifndef __SYS_ARCH_H__
#define __SYS_ARCH_H__

#include <pthread.h>
#include <semaphore.h>

typedef struct {
    pthread_mutex_t mutex;
    int canuse;
} sys_mutex_t;
typedef struct {
    sem_t sem;
    int canuse;
 } sys_sem_t;
typedef struct {
    sem_t writesem;
    sem_t readsem;
    pthread_mutex_t mutex;
    void **data;
    int size;
    int begin;
    int end;
    int canuse;
} sys_mbox_t;
typedef pthread_t sys_thread_t;

#endif