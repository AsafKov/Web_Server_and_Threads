//
// Created by student on 6/11/22.
//

#ifndef THREADS_WORKERTHREAD_H
#define THREADS_WORKERTHREAD_H
#include <pthread.h>

enum OverloadPolicy {block, drop_tail, drop_random};

typedef struct WorkerThread {
    pthread_t *thread;
    int given_id;
    int requests_counter;
    int static_requests_counter;
    int dynamic_requests_counter;
} WorkerThread;

#endif //THREADS_WORKERTHREAD_H
