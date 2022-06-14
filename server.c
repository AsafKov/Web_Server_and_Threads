#include <pthread.h>
#include <sys/time.h>
#include "queue.h"
#include "WorkerThread.h"
#include "ServerRequest.h"

typedef struct ServerData {
    int port;
    int busy_workers;
    int max_requests;
    int number_of_workers;
    int requests_in_progress;
    enum OverloadPolicy overload_policy;
    pthread_mutex_t lock_request_handle;
    pthread_mutex_t lock_thread_init;
    pthread_cond_t queue_space_available;
    pthread_cond_t is_work_available;
    pthread_cond_t is_worker_available;
    Queue *workers_queue;
    Queue *requests;
} ServerData;

void getargs(ServerData *server_data, int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    server_data->port = atoi(argv[1]);
    server_data->number_of_workers = atoi(argv[2]);
    server_data->max_requests = atoi(argv[3]);

    if(strcmp(argv[4], "block") == 0){
        server_data->overload_policy = block;
    }
    if(strcmp(argv[4], "dt") == 0){
        server_data->overload_policy = drop_tail;
    }
    if(strcmp(argv[4], "random") == 0){
        server_data->overload_policy = drop_random;
    }
    if(strcmp(argv[4], "dh") == 0){
        server_data->overload_policy = drop_head;
    }
}

void init_server_data(ServerData *data, int argc, char *argv[]);

void thread_master_routine(void *data);

void thread_worker_routine(void *data);

int main(int argc, char *argv[]) {
    pthread_t master;
    WorkerThread *temp_worker;

    ServerData *server_data = (ServerData *) malloc(sizeof(ServerData));
    init_server_data(server_data, argc, argv);

    // Create master thread
    if (pthread_create(&master, NULL, (void *(*)(void *)) thread_master_routine, server_data) != 0) {
        //TODO something
    }

    for (int i = 0; i < server_data->number_of_workers; i++) {
        temp_worker = (WorkerThread *) malloc(sizeof (WorkerThread));
        temp_worker->given_id = i;
        temp_worker->requests_counter = 0;
        temp_worker->dynamic_requests_counter = 0;
        temp_worker->static_requests_counter = 0;
        temp_worker->thread = (pthread_t *) malloc(sizeof (pthread_t));
        queue_push(server_data->workers_queue, temp_worker);
        if (pthread_create(temp_worker->thread, NULL, (void *(*)(void *)) thread_worker_routine, server_data) != 0) {
            //TODO something
        }
    }

    while(1);
}

/** Initialize data shared between master and workers **/
void init_server_data(ServerData *server_data, int argc, char *argv[]){
    server_data->busy_workers = 0;
    server_data->requests_in_progress = 0;
    pthread_mutex_init(&server_data->lock_request_handle, NULL);
    pthread_mutex_init(&server_data->lock_thread_init, NULL);
    pthread_cond_init(&server_data->is_work_available, NULL);
    pthread_cond_init(&server_data->is_worker_available, NULL);
    pthread_cond_init(&server_data->queue_space_available, NULL);
    server_data->workers_queue = (Queue *) malloc(sizeof(Queue));
    queue_init(server_data->workers_queue);

    server_data->requests = (Queue *) malloc(sizeof(Queue));
    queue_init(server_data->requests);

    getargs(server_data, argc, argv);
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

        pthread_mutex_lock(&server_data->lock_request_handle);
        // If the requests in the system are currently at maximum capacity, stop receiving new requests
        if(queue_size(server_data->requests) + server_data->requests_in_progress >= server_data->max_requests){
            if(server_data->overload_policy == drop_tail){
                Close(conn_fd);
                pthread_mutex_unlock(&server_data->lock_request_handle);
                continue;
            }
            if(server_data->overload_policy == drop_random){
                queue_drop_random(server_data->requests);
            }

            if(server_data->overload_policy == drop_head){
                Close(((ServerRequest *)queue_front(server_data->requests))->fd);
                queue_pop(server_data->requests, 1);
            }
            if(server_data->overload_policy == block){
                while(queue_size(server_data->requests) + server_data->requests_in_progress == server_data->max_requests){
                    pthread_cond_wait(&server_data->queue_space_available, &server_data->lock_request_handle);
                }
            }
        }
        pthread_mutex_unlock(&server_data->lock_request_handle);

        current_fd = (int *) malloc(sizeof(int));
        *current_fd = conn_fd;

        ServerRequest *request = (ServerRequest *) malloc(sizeof (ServerRequest));
        if(gettimeofday(&request->arrival_interval, NULL) == -1){
            //TODO: handle error
        }
        request->fd = *current_fd;
        // push new request to queue, lock access to queue while it is done
        pthread_mutex_lock(&server_data->lock_request_handle);
        queue_push(server_data->requests, request);
        pthread_cond_signal(&server_data->is_work_available);
        pthread_mutex_unlock(&server_data->lock_request_handle);
    }
}

WorkerThread *find_thread_by_id(Queue *workers_queue, pthread_t curr_thread){
    QueueNode *curr = workers_queue->head;
    while(curr->link != NULL && (pthread_t) *((WorkerThread *) curr->data)->thread != curr_thread){
        curr = curr->link;
    }
    return (WorkerThread *) curr->data;
}

void thread_worker_routine(void *data) {
    ServerData *server_data = (ServerData *) data;
    ServerRequest *curr_request;

    while (1) {
        pthread_mutex_lock(&server_data->lock_request_handle);
        pthread_cond_signal(&server_data->is_worker_available);
        while(queue_size(server_data->requests) == 0){
            pthread_cond_wait(&server_data->is_work_available, &server_data->lock_request_handle);
        }
        curr_request = queue_front(server_data->requests);
        queue_pop(server_data->requests, 0);
        WorkerThread *handling_thread = find_thread_by_id(server_data->workers_queue, pthread_self());
        server_data->busy_workers++;
        server_data->requests_in_progress++;
        pthread_mutex_unlock(&server_data->lock_request_handle);

        struct timeval dispatch_time;
        if(gettimeofday(&dispatch_time, NULL) == -1){
            //TODO: handle error
        }
        timersub(&dispatch_time, &curr_request->arrival_interval, &curr_request->dispatch_interval);
        Close(requestHandle(curr_request, handling_thread));

        pthread_mutex_lock(&server_data->lock_request_handle);
        server_data->busy_workers--;
        server_data->requests_in_progress--;
        pthread_cond_signal(&server_data->queue_space_available);
        pthread_mutex_unlock(&server_data->lock_request_handle);
    }
}


    


 
