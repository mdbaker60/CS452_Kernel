#ifndef __QUEUE_H__
#define __QUEUE_H__

#define NUMPRIO 8

#include <task.h>

static const int LogTable256[256] = 
{
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    -1, 0, 1, 1, 2 ,2 ,2 ,2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

struct PriorityQueue {
  int highPriority;
  int availablePriorities;
  struct Task *head[NUMPRIO];
  struct Task *tail[NUMPRIO];
};

void initQueue(struct PriorityQueue *queue);
void enqueue(struct PriorityQueue *queue, struct Task *task, int priority);
struct Task *dequeue(struct PriorityQueue *queue);
int queueEmpty(struct PriorityQueue *queue);
void removeFromQueue(struct PriorityQueue *queue, struct Task *task);

#endif
