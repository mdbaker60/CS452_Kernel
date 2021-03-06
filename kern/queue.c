#include <task.h>
#include <queue.h>
#include <values.h>

#include <bwio.h>

void initQueue(struct PriorityQueue *queue) {
  queue->highPriority = -1;
  queue->availablePriorities = 0;
  queue->size = 0;
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
    task->last = (queue->tail)[priority];
    (queue->tail)[priority] = task;
  }

  ++queue->size;
}


struct Task *dequeue(struct PriorityQueue *queue) {
  if(queue->highPriority == LogTable256[0]) return NULL;

  struct Task *retVal = (queue->head)[queue->highPriority];
  if(retVal->next != NULL) {
    (queue->head)[queue->highPriority] = retVal->next;
  }else{
    (queue->head)[queue->highPriority] = (queue->tail)[queue->highPriority] = NULL;

    queue->availablePriorities &= ~(1 << queue->highPriority);
    queue->highPriority = LogTable256[queue->availablePriorities];
  }

  retVal->next = NULL;
  retVal->last = NULL;
  --queue->size;
  return retVal;
}

int queueEmpty(struct PriorityQueue *queue) {
  return queue->highPriority == -1;
}

void removeFromQueue(struct PriorityQueue *queue, struct Task *task) {
  if(task->last == NULL && task->next == NULL) {
    (queue->head)[task->priority] = (queue->tail)[task->priority] = NULL;
    queue->availablePriorities &= ~(1 << task->priority);
    queue->highPriority = LogTable256[queue->availablePriorities];
  }else if(task->next == NULL) {
    (task->last)->next = NULL;
    (queue->tail)[task->priority] = task->last;
  }else if(task->last == NULL) {
    (task->next)->last = NULL;
    (queue->head)[task->priority] = task->next;
  }else{
    (task->last)->next = task->next;
    (task->next)->last = task->last;
  }
  task->next = NULL;
  task->last = NULL;
}
