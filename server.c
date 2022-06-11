#include "segel.h"
#include "request.h"
#include <pthread.h>
#include "queue.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// HW3: Parse the new arguments too
void getargs(int *port, int *number_of_workers, int *max_requests, int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *number_of_workers = atoi(argv[0]);
    *port = atoi(argv[1]);
    *max_requests = atoi(argv[2]);
}

typedef struct ServerData {
    int port;
    int busy_workers;
    int max_requests;
    int number_of_workers;
    int requests_in_progress;
    pthread_mutex_t lock_request_handle;
    pthread_cond_t queue_space_available;
    pthread_cond_t is_work_available;
    pthread_cond_t is_worker_available;
    Queue *workers_queue;
    Queue *requests;
} ServerData;

void init_server_data(ServerData *data, int argc, char *argv[]);

void thread_master_routine(void *data);

void thread_worker_routine(void *data);

int main(int argc, char *argv[]) {
    pthread_t master, *temp_worker;

    ServerData *server_data = (ServerData *) malloc(sizeof(ServerData));
    init_server_data(server_data, argc, argv);

    // Create master thread
    if (pthread_create(&master, NULL, (void *(*)(void *)) thread_master_routine, server_data) != 0) {
        //TODO something
    }

    for (int i = 0; i < server_data->number_of_workers; i++) {
        temp_worker = (pthread_t *) malloc(sizeof(pthread_t));
        if (pthread_create(temp_worker, NULL, (void *(*)(void *)) thread_worker_routine, server_data) != 0) {
            //TODO something
        }
        queue_push(server_data->workers_queue, temp_worker);
    }

    while(1);
}

/** Initialize data shared between master and workers **/
void init_server_data(ServerData *server_data, int argc, char *argv[]){
    server_data->busy_workers = 0;
    server_data->requests_in_progress = 0;
    pthread_mutex_init(&server_data->lock_request_handle, NULL);
    pthread_cond_init(&server_data->is_work_available, NULL);
    pthread_cond_init(&server_data->is_worker_available, NULL);
    pthread_cond_init(&server_data->queue_space_available, NULL);
    server_data->workers_queue = (Queue *) malloc(sizeof(Queue));
    queue_init(server_data->workers_queue);

    server_data->requests = (Queue *) malloc(sizeof(Queue));
    queue_init(server_data->requests);

    server_data->requests_in_progress = (Queue *) malloc(sizeof(Queue));
    queue_init(server_data->requests_in_progress);

    getargs(&server_data->port, &server_data->number_of_workers, &server_data->max_requests, argc, argv);
}

void thread_master_routine(void *data) {
    ServerData *server_data = (ServerData *) data;
    int conn_fd, client_len;
    struct sockaddr_in client_addr;
    int listenfd = Open_listenfd(server_data->port);
    int *current_fd;

    while (1) {
        // Actively listen on port for server requests
        client_len = sizeof(client_addr);
        conn_fd = Accept(listenfd, (SA *)&client_addr, (socklen_t *) &client_len);
        current_fd = (int *) malloc(sizeof(int));
        *current_fd = conn_fd;

        // If the requests in the system are currently at maximum capacity, stop receiving new requests
        pthread_mutex_lock(&server_data->lock_request_handle);
        while(queue_size(server_data->requests) + server_data->requests_in_progress == server_data->max_requests){
            pthread_cond_wait(&server_data->queue_space_available, &server_data->lock_request_handle);
        }
        pthread_mutex_unlock(&server_data->lock_request_handle);

        // push new request to queue, lock access to queue while it is done
        pthread_mutex_lock(&server_data->lock_request_handle);
        queue_push(server_data->requests, current_fd);
        while(server_data->busy_workers == server_data->number_of_workers){
            pthread_cond_wait(&server_data->is_worker_available, &server_data->lock_request_handle);
        }
        pthread_cond_signal(&server_data->is_work_available);
        pthread_mutex_unlock(&server_data->lock_request_handle);
    }
}

void thread_worker_routine(void *data) {
    ServerData *server_data = (ServerData *) data;
    int *conn_fd;

    while (1) {
        pthread_mutex_lock(&server_data->lock_request_handle);
        pthread_cond_signal(&server_data->is_worker_available);
        while(queue_size(server_data->requests) == 0){
            pthread_cond_wait(&server_data->is_work_available, &server_data->lock_request_handle);
        }

        conn_fd = queue_front(server_data->requests);
        queue_pop(server_data->requests, 0);
        server_data->busy_workers++;
        server_data->requests_in_progress++;
        pthread_mutex_unlock(&server_data->lock_request_handle);

        Close(requestHandle(*conn_fd));
        pthread_mutex_lock(&server_data->lock_request_handle);
        server_data->busy_workers--;
        server_data->requests_in_progress--;
        pthread_mutex_unlock(&server_data->lock_request_handle);
    }
}


    


 
