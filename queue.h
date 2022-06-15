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
    if(index == 0){
        Close(((ServerRequest *)(current->data))->fd);
        queue_pop(q, 1);
        return;
    }
    int current_index = 0;
    while(current_index != index){
        prev = current;
        current = current->link;
        current_index++;
    }

    prev->link = current->link;
    if(current == q->tail){
        q->tail = prev;
    }
    q->size--;
    current->link = NULL;
    Close(((ServerRequest *)(current->data))->fd);
    free(current->data);
    free(current);
}


void queue_drop_random(Queue *q) {
    srand(time(0));
    int drop_amount = ceil(q->size*0.3), rand_index;

    for (int i = 0; i < drop_amount; i++) {
        rand_index = rand() % q->size;
        queue_remove_index(q, rand_index);
    }
}