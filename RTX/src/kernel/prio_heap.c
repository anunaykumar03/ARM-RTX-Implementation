#include "common.h"
#include "k_inc.h"
#include "prio_heap.h"

unsigned int sched_heap[MAX_TASKS];
unsigned int heap_size = 0;
unsigned int countL = 0;
unsigned int countH = 0;

//inline void sched_pop(){
//    if (heap_size == 0){
//        return;
//    }
//    sched_remove(sched_heap[0]);
//}

inline TCB* sched_peak(void){
    if (heap_size == 0){
        return NULL;
    }
    return &g_tcbs[sched_heap[0]];
}

//returns 1 if a has higher prio than b
//returns 0 if a is has lower prio than b
int has_higher_prio(unsigned int index_a, unsigned int index_b){
    if(g_tcbs[sched_heap[index_a]].prio == 0) return 0;
    if(g_tcbs[sched_heap[index_b]].prio == 0) return 1;
    // optimize these comparisons
    if (g_tcbs[sched_heap[index_a]].prio < g_tcbs[sched_heap[index_b]].prio){
        return 1;
    }
    else if (g_tcbs[sched_heap[index_a]].prio > g_tcbs[sched_heap[index_b]].prio){
        return 0;
    }

    if (g_tcbs[sched_heap[index_a]].countH < g_tcbs[sched_heap[index_b]].countH) {
        return 1;
    }
    else if (g_tcbs[sched_heap[index_a]].countH > g_tcbs[sched_heap[index_b]].countH) {
        return 0;
    }

    if (g_tcbs[sched_heap[index_a]].countL < g_tcbs[sched_heap[index_b]].countL) {
        return 1;
    }
    else {
        return 0;
    }
}

void up_heap(unsigned int index){
    unsigned int temp = 0;
    while(index > 0){
        //current prio is higher than parent
        if(has_higher_prio(index, index >> 1)){
            temp = sched_heap[index];
            sched_heap[index] = sched_heap[index >> 1];
            sched_heap[index >> 1] = temp;

            g_tcbs[sched_heap[index]].heap_idx = index;
            g_tcbs[sched_heap[index >> 1]].heap_idx = index >> 1;
        } else {
            break;
        }

        index = index >> 1;
    }
}

void down_heap(unsigned int index){
    int smaller_child;
    unsigned int temp = 0;
    while((index << 1) < heap_size){
        smaller_child = index << 1;
        //if right child is has higher prio than left child
        if(((index << 1) + 1 < heap_size) && has_higher_prio(index << 1 + 1, index << 1)){
            smaller_child++;
        }

        //smallest child has higher prio than current, swap with it
        if(has_higher_prio(smaller_child, index)){
            temp = sched_heap[index];
            sched_heap[index] = sched_heap[smaller_child];
            sched_heap[smaller_child] = temp;

            g_tcbs[sched_heap[index]].heap_idx = index;
            g_tcbs[sched_heap[smaller_child]].heap_idx = smaller_child;
        } else{
            break;
        }

        index = smaller_child;
    }
}

void sched_insert(TCB *tcb){
    sched_heap[heap_size] = tcb->tid;
    tcb->heap_idx = heap_size;
    tcb->countL = countL++;
    tcb->countH = countH;
    if (countL == 0){
        ++countH;
    }
    up_heap(heap_size);
    ++heap_size;
}

void sched_remove(task_t tid){
    unsigned int old_idx = g_tcbs[tid].heap_idx;
    sched_heap[old_idx] = sched_heap[--heap_size];
    g_tcbs[sched_heap[old_idx]].heap_idx = old_idx;
    down_heap(old_idx);
}
