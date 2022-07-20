#include <arch/sys_arch.h>
#include <lwip/err.h>
#include <lwip/sys.h>

void sys_init(void) {
}

void sys_msleep(u32_t ms) {
    msleep(ms);
}

err_t sys_mutex_new(sys_mutex_t *mutex) {
    int res = pthread_mutex_init(&mutex->mutex, NULL);
    if (res == 0) {
        mutex->canuse = 1;
    } else {
        mutex->canuse = 0;
    }
    return res;
}

void sys_mutex_lock(sys_mutex_t *mutex) {
    pthread_mutex_lock(&mutex->mutex);
}

void sys_mutex_unlock(sys_mutex_t *mutex) {
    pthread_mutex_unlock(&mutex->mutex);
}

void sys_mutex_free(sys_mutex_t *mutex) {
    pthread_mutex_destroy(&mutex->mutex);
    mutex->canuse = 0;
}

int sys_mutex_valid(sys_mutex_t *mutex) {
    return mutex->canuse;
}

void sys_mutex_set_invalid(sys_mutex_t *mutex) {
    mutex->canuse =0;
}

err_t sys_sem_new(sys_sem_t *sem, u8_t count) {
    int res = sem_init(&sem->sem, 0, count);
    if (res == 0) {
        sem->canuse = 1;
    } else {
        sem->canuse = 0;
    }
    return res;
}

void sys_sem_signal(sys_sem_t *sem) {
    sem_post(&sem->sem);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout) {
    u32_t start = sys_now();
    if (timeout > 0) {
        struct timespec timeval;
        timeval.tv_sec = timeout / 1000;
        timeval.tv_nsec = 1000000 * (timeout % 1000);
        if (sem_timedwait(&sem->sem, &timeval) == -1) {
            return SYS_ARCH_TIMEOUT;
        }
    } else {
        sem_wait(&sem->sem);
    }
    u32_t end = sys_now();
    return end-start;
}

void sys_sem_free(sys_sem_t *sem) {
    sem->canuse = 0;
    sem_destroy(&sem->sem);
}

int sys_sem_valid(sys_sem_t *sem) {
    return sem->canuse;
}

void sys_sem_set_invalid(sys_sem_t *sem) {
    sem->canuse = 0;
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size) {
    void **data = (void**)malloc(size*sizeof(void*));
    if (data == NULL) {
        mbox->canuse = 0;
        return ERR_MEM;
    }
    mbox->data = data;
    if (sem_init(&mbox->readsem, 0, size) != 0) {
        free(data);
        mbox->canuse = 0;
        return ERR_MEM;
    }
    if (sem_init(&mbox->writesem, 0, 0) != 0) {
        sem_destroy(&mbox->readsem);
        free(data);
        mbox->canuse = 0;
        return ERR_MEM;
    }
    if (pthread_mutex_init(&mbox->mutex, NULL) != 0) {
        sem_destroy(&mbox->writesem);
        sem_destroy(&mbox->readsem);
        free(data);
        mbox->canuse = 0;
        return ERR_MEM;
    }
    mbox->size = size;
    mbox->begin = 0;
    mbox->end = 0;
    mbox->canuse = 1;
    return ERR_OK;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg) {
    sem_wait(&mbox->readsem);
    pthread_mutex_lock(&mbox->mutex);
    mbox->data[mbox->begin] = msg;
    mbox->begin++;
    if (mbox->begin == mbox->size) {
        mbox->begin = 0;
    }
    pthread_mutex_unlock(&mbox->mutex);
    sem_post(&mbox->writesem);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg) {
    if (sem_trywait(&mbox->readsem) < 0) {
        return ERR_BUF;
    }
    pthread_mutex_lock(&mbox->mutex);
    mbox->data[mbox->begin] = msg;
    mbox->begin++;
    if (mbox->begin == mbox->size) {
        mbox->begin = 0;
    }
    pthread_mutex_unlock(&mbox->mutex);
    sem_post(&mbox->writesem);
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg) {
    sys_mbox_trypost(&mbox, msg);
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout) {
    if (timeout > 0) {
        struct timespec timeval;
        timeval.tv_sec = timeout / 1000;
        timeval.tv_nsec = 1000000 * (timeout % 1000);
        if (sem_timedwait(&mbox->writesem, &timeval) == -1) {
            return SYS_ARCH_TIMEOUT;
        }
    } else {
        sem_wait(&mbox->writesem);
    }
    pthread_mutex_lock(&mbox->mutex);
    printf("in %s, at %d\n", __FILE__, __LINE__);
    *msg = mbox->data[mbox->end];
    mbox->end++;
    if (mbox->end == mbox->size) {
        mbox->end = 0;
    }
    pthread_mutex_unlock(&mbox->mutex);
    sem_post(&mbox->readsem);
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg) {
    printf("in %s, at %d\n", __FILE__, __LINE__);
    if (sem_trywait(&mbox->writesem) < 0) {
        return ERR_BUF;
    }
    pthread_mutex_lock(&mbox->mutex);
    printf("in %s, at %d\n", __FILE__, __LINE__);
    *msg = mbox->data[mbox->end];
    mbox->end++;
    if (mbox->end == mbox->size) {
        mbox->end = 0;
    }
    pthread_mutex_unlock(&mbox->mutex);
    sem_post(&mbox->readsem);
}

void sys_mbox_free(sys_mbox_t *mbox) {
    free(mbox->data);
    sem_destroy(&mbox->readsem);
    sem_destroy(&mbox->writesem);
    pthread_mutex_destroy(&mbox->mutex);
    mbox->canuse = 0;
}

int sys_mbox_valid(sys_mbox_t *mbox) {
    return mbox->canuse;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox) {
    mbox->canuse = 0;
}


sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio) {
    printf("in %s, at %d\n", __FILE__, __LINE__);
    pthread_t th;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&th, &attr, thread, arg) != 0) {
        printf("in %s, at %d\n", __FILE__, __LINE__);
    }
    pthread_attr_destroy(&attr);
    return th;
}
