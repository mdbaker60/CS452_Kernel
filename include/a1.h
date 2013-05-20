#ifndef __A1_H__
#define __A1_H__

#include <values.h>
#include <task.h>

int *getSP();
struct Task *Create_sys(int priority, void (*code)());
void syscall_enter();
void getNextRequest(struct Task *task, struct Request *request);

//tempory queue structure
//will not use in final version
struct Queue {
  int length;
  int *data[128];
  int head, tail;
};

void init(struct Queue *queue) {
  queue->length = 0;
  queue->head = queue->tail = 0;
}

void enqueue(int *newData, struct Queue *queue) {
  if(queue->length < 128) {
    (queue->head)++;
    queue->head %= 128;
    queue->data[queue->head] = newData;
    (queue->length)++;
  }
}

int *dequeue(struct Queue *queue) {
  if(queue->length > 0) {
    (queue->length)--;
    (queue->tail)++;
    queue->tail %= 128;
    return queue->data[queue->tail];
  }else{
    return NULL;
  }
}

int empty(struct Queue *queue) {
  if(queue->length == 0) {
    return true;
  }else{
    return false;
  }
}

#endif
