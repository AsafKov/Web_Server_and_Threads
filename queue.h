#include "segel.h"
#include "request.h"

typedef struct QueueNode {
    void *data;
    struct QueueNode *link;
} QueueNode;

typedef struct Queue {
    int size;
    QueueNode *head;
    QueueNode *tail;
} Queue;

void queue_init(Queue *q) {
    q->size = 0;
    q->head = q->tail = NULL;
}

void queue_push(Queue *q, void *element) {
    if (!q->head) {
        q->head = (QueueNode *) malloc(sizeof(QueueNode));
        q->head->data = element;
        q->tail = q->head;
    } else {
        q->tail->link = (QueueNode *) malloc(sizeof(QueueNode));
        q->tail = q->tail->link;
        q->tail->data = element;
    }

    q->tail->link = NULL;
    q->size++;
}

void *queue_front(Queue *q) {
    return q->size ? q->head->data : NULL;
}

void queue_pop(Queue *q, int release) {
    if (q->size) {
        QueueNode *temp = q->head;
        if (--(q->size) > 0) {
            q->head = q->head->link;
        } else {
            q->head = q->tail = NULL;
        }
        // release memory accordingly
        if (release) {
            free(temp->data);
        }
        free(temp);
    }
}

int queue_size(Queue *q) {
    return q->size;
}

void queue_remove_index(Queue *q, int index) {
    QueueNode *prev = NULL, *current = q->head;
    ServerRequest *request;
    if(index == 0){
        request = (ServerRequest *) queue_front(q);
        Close(request->fd);
        queue_pop(q, 1);
        return;
    }
    int current_index = 0;
    while(current_index != index){
        prev = current;
        current = current->link;
        current_index++;
    }

    if(current->link != NULL){
        prev->link = current->link;
    }
    if(current == q->tail){
        q->tail = prev;
    }
    q->size--;
    current->link = NULL;
    request = (ServerRequest *) current->data;
    Close(request->fd);
    free(current->data);
    free(current);
}


void queue_drop_random(Queue *q) {
    ServerRequest *request;
    if(q->size == 0) return;
    if(q->size == 1){
        request = (ServerRequest *) queue_front(q);
        Close(request->fd);
        queue_pop(q, 1);
        return;
    }
    srand(time(0));
    double temp = (double) q->size * 0.3;
    int drop_amount = ceil(temp), rand_index;

    for (int i = 0; i < drop_amount; i++) {
        rand_index = abs(rand() % q->size);
        queue_remove_index(q, rand_index);
    }
}