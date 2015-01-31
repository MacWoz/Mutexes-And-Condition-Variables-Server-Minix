#include "inc.h"
#include "queue.h"

queue* createQueue(int mutex) {
    queue* Q = malloc(sizeof(queue));
    Q->mutex_id = mutex;
    Q->head = malloc(sizeof(Node));
    Q->tail = malloc(sizeof(Node));
    Q->head->next = Q->tail;
    Q->tail->next = NULL;
    return Q;
}

void enqueue(endpoint_t proc_nr, queue* Q) {
    Node* last = Q->head;
    while (last->next != Q->tail)
        last = last->next;

    Node* newElement = malloc(sizeof(Node));
    newElement->next = Q->tail;
    last->next = newElement;
    newElement->proc_nr = proc_nr;
}

int pop(queue* Q) {
    Node* first = Q->head->next;
    int value = first->proc_nr;
    Q->head->next = first->next;
    free(first);
    return value;
}

int isEmpty(queue* Q) {
    return Q->head->next == Q->tail;
}

void destroyQueue(queue* Q) {
    Node* temp = Q->head;
    while (temp != NULL) {
        Node* element = temp->next;
        free(temp);
        temp = element;
    }
    free(Q);
}
