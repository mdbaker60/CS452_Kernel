#include <task.h>
#include <queue.h>
#include <values.h>

#include <bwio.h>

void initQueue(struct PriorityQueue *queue) {
  queue->highPriority = -1;
  queue->availablePriorities = 0;
  int i;
  for(i=0; i<NUMPRIO; i++) {
    (queue->head)[i] = (queue->tail)[i] = NULL;
  }
}

void enqueue(struct PriorityQueue *queue, struct Task *task, int priority) {
  if(priority > queue->highPriority) {
    queue->highPriority = priority;
  }

  queue->availablePriorities |= 1 << priority;
  if((queue->tail)[priority] == NULL) {
    (queue->tail)[priority] = (queue->head)[priority] = task;
  }else{
    (queue->tail)[priority]->next = task;
    (queue->tail)[priority] = task;
  }
}


struct Task *dequeue(struct PriorityQueue *queue) {
  if(queue->highPriority == LogTable256[0]) return NULL;
  //bwprintf(COM2, "queue not returned NULL, getting task of priority %d\r", queue->highPriority);

  struct Task *retVal = (queue->head)[queue->highPriority];
  if(retVal->next != NULL) {
    (queue->head)[queue->highPriority] = retVal->next;
  }else{
    (queue->head)[queue->highPriority] = (queue->tail)[queue->highPriority] = NULL;

    queue->availablePriorities &= ~(1 << queue->highPriority);
    queue->highPriority = LogTable256[queue->availablePriorities];
  }
  //bwprintf(COM2, "dequeued task %d of priority %d\r", retVal->ID, retVal->priority);

  retVal->next = NULL;
  return retVal;
}

int queueEmpty(struct PriorityQueue *queue) {
  return queue->highPriority == -1;
}
