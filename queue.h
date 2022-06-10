typedef struct _QueueNode {
    void *data;
    struct _QueueNode *link;
} QueueNode;

typedef struct _Queue {
    int size;
    QueueNode *head;
    QueueNode *tail;
} Queue;

/*****
** initialize an empty Queue
** must be called first after a new Queue is declared
*/ void queue_init(Queue *q)
	{
		q->size = 0;
		q->head = q->tail = NULL;
	}

/*****
** push a new element to the end of the Queue
** it's up to the client code to allocate and maintain memory of "element"
*/ void queue_push(Queue *q, void *element)
	{
		if (!q->head) {
			q->head = (QueueNode*)malloc(sizeof(QueueNode));
			q->head->data = element;
			q->tail = q->head;
		} else {
			q->tail->link = (QueueNode*)malloc(sizeof(QueueNode));
			q->tail = q->tail->link;
			q->tail->data = element;
		}

		q->tail->link = NULL;
		q->size++;
	}

/*****
** return the first element in the Queue, or NULL when the Queue is empty
*/ void *queue_front(Queue *q)
	{
		return q->size ? q->head->data : NULL;
	}

/*****
** remove the first element (pointer) from the Queue
** set "release" to non-zero if memory deallocation is desired
*/ void queue_pop(Queue *q, int release)
	{
		if (q->size) {
			QueueNode *temp = q->head;
			if (--(q->size)) {
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

/*****
** remove all elements (pointers) from the Queue
** set "release" to non-zero if memory deallocation is desired
*/ void queue_clear(Queue *q, int release)
	{
		while (q->size) {
			QueueNode *temp = q->head;
			q->head = q->head->link;
			if (release) {
				free(temp->data);
			}
			free(temp);
			q->size--;
		}

		q->head = q->tail = NULL;
	}


/*****
** return current number of elements in the Queue, or 0 when Queue is empty
*/ int queue_size(Queue *q)
	{
		return q->size;
	}
