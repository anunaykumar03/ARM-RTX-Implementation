#ifndef PRIO_HEAP_H
#define PRIO_HEAP_H

#include "common.h"
#include "k_inc.h"

//void sched_pop();  // return and remove highest priority task in ready queue
TCB* sched_peak(void); // return highest priority task in ready queue
void sched_insert(TCB *tcb);      // insert new task into scheduler heap
void sched_remove(task_t tid);                // removes task that calls exit
void sched_change_prio(task_t tid, U8 prio);     // change task priority

#endif
