/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                     Copyright 2020-2021 Yiqing Huang
 *                          All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice and the following disclaimer.
 *
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************
 */

/**************************************************************************//**
 * @file        k_task.c
 * @brief       task management C file
 *              l2
 * @version     V1.2021.01
 * @authors     Yiqing Huang
 * @date        2021 JAN
 *
 * @attention   assumes NO HARDWARE INTERRUPTS
 * @details     The starter code shows one way of implementing context switching.
 *              The code only has minimal sanity check.
 *              There is no stack overflow check.
 *              The implementation assumes only two simple privileged task and
 *              NO HARDWARE INTERRUPTS.
 *              The purpose is to show how context switch could be done
 *              under stated assumptions.
 *              These assumptions are not true in the required RTX Project!!!
 *              Understand the assumptions and the limitations of the code before
 *              using the code piece in your own project!!!
 *
 *****************************************************************************/

//#include "VE_A9_MP.h"
#include "Serial.h"
#include "k_task.h"
#include "k_rtx.h"
#include "prio_heap.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/*
 *==========================================================================
 *                            GLOBAL VARIABLES
 *==========================================================================
 */

TCB             *gp_current_task = NULL;	// the current RUNNING task
TCB             g_tcbs[MAX_TASKS];			// an array of TCBs
RTX_TASK_INFO   g_null_task_info;			// The null task info
U32             g_num_active_tasks = 0;		// number of non-dormant tasks


// stores tasks IDs for assigning TID
unsigned int U_TID_Q[MAX_TASKS-1];
unsigned int U_TID_head;
unsigned int U_TID_tail;
RTX_TASK_INFO U_rtx_task_infos[MAX_TASKS-1];

// counters for FIFO
unsigned int counterL = 0;
unsigned int counterH = 0;

/*---------------------------------------------------------------------------
The memory map of the OS image may look like the following:

                       RAM_END+---------------------------+ High Address
                              |                           |
                              |                           |
                              |    Free memory space      |
                              |   (user space stacks      |
                              |         + heap            |
                              |                           |
                              |                           |
                              |                           |
 &Image$$ZI_DATA$$ZI$$Limit-->|---------------------------|-----+-----
                              |         ......            |     ^
                              |---------------------------|     |
                              |      U_STACK_SIZE         |     |
             g_p_stacks[15]-->|---------------------------|     |
                              |                           |     |
                              |  other kernel proc stacks |     |
                              |---------------------------|     |
                              |      U_STACK_SIZE         |  OS Image
              g_p_stacks[2]-->|---------------------------|     |
                              |      U_STACK_SIZE         |     |
              g_p_stacks[1]-->|---------------------------|     |
                              |      U_STACK_SIZE         |     |
              g_p_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |                           |  OS Image
                              |---------------------------|     |
                              |      K_STACK_SIZE         |     |                
             g_k_stacks[15]-->|---------------------------|     |
                              |                           |     |
                              |     other kernel stacks   |     |                              
                              |---------------------------|     |
                              |      K_STACK_SIZE         |  OS Image
              g_k_stacks[2]-->|---------------------------|     |
                              |      K_STACK_SIZE         |     |                      
              g_k_stacks[1]-->|---------------------------|     |
                              |      K_STACK_SIZE         |     |
              g_k_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |---------------------------|     |
                              |        TCBs               |  OS Image
                      g_tcbs->|---------------------------|     |
                              |        global vars        |     |
                              |---------------------------|     |
                              |                           |     |          
                              |                           |     |
                              |                           |     |
                              |                           |     V
                     RAM_START+---------------------------+ Low Address
    
---------------------------------------------------------------------------*/ 

/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */

//void switch_task(task_t tid, U8 exit_old, U8 remove_new_from_heap){ // args: tid of task to switch to
//        TCB *p_old_task = gp_current_task;
//        gp_current_task = &g_tcbs[tid];
//        gp_current_task->state = RUNNING; // change state of the to-be-switched-in  tcb
//        p_old_task->state = DORMANT;      // change state of the to-be-switched-out tcb
//
//	if(remove_new_from_heap){
//		sched_remove(tid);
//	}
//
//	if(!exit_old){
//        	p_old_task->state = READY;
//        	sched_insert(p_old_task);
//	}
//
//        k_tsk_switch(p_old_task); // switch stacks
//}

/**************************************************************************//**
 * @brief   scheduler, pick the TCB of the next to run task
 *
 * @return  TCB pointer of the next to run task
 * @post    gp_curret_task is updated
 *
 *****************************************************************************/

TCB *scheduler(void)
{
	// modify sched_remove() to pop instead (returns what it removes)
	if (sched_peak()->prio == NULL || ((gp_current_task->prio < sched_peak()->prio && gp_current_task->prio != 0) && gp_current_task->state != DORMANT)){
		return gp_current_task;
	}
	TCB * p_new_task = sched_peak();
	sched_remove(p_new_task->tid);
	if (gp_current_task->state != DORMANT){
		sched_insert(gp_current_task);
	}
	return p_new_task;
}



/**************************************************************************//**
 * @brief       initialize all boot-time tasks in the system,
 *
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       task_info   boot-time task information structure pointer
 * @param       num_tasks   boot-time number of tasks
 * @pre         memory has been properly initialized
 * @post        none
 *
 * @see         k_tsk_create_new
 *****************************************************************************/

int k_tsk_init(RTX_TASK_INFO *task_info, int num_tasks)
{
    extern U32 SVC_RESTORE;

    RTX_TASK_INFO *p_taskinfo = &g_null_task_info;
    g_num_active_tasks = 0;

    if (num_tasks > MAX_TASKS) {
    	return RTX_ERR;
    }

    // create the first task
    TCB *p_tcb = &g_tcbs[0];
    p_tcb->prio     = PRIO_NULL;
    p_tcb->priv     = 1;
    p_tcb->tid      = TID_NULL;
    p_tcb->state    = RUNNING;
    p_tcb->countL	= 0;
    p_tcb->countH 	= 0;
    g_num_active_tasks++;
    gp_current_task = p_tcb;

    // create the rest of the tasks
    p_taskinfo = task_info;
    for ( int i = 0; i < num_tasks; i++ ) {
        TCB *p_tcb = &g_tcbs[i+1];
        if (k_tsk_create_new(p_taskinfo, p_tcb, i+1) == RTX_OK) {
        	g_num_active_tasks++;
    		sched_insert(p_tcb);
        }
        p_taskinfo++;
    }

    // initialize TID number queue
    for (int i = num_tasks+1; i < MAX_TASKS; i++){
        U_TID_Q[i] = i;
        // initialize all invalid TCBs to dormant
        g_tcbs[i].state = DORMANT;
    }
    U_TID_head = num_tasks+1;
    U_TID_tail = 0;


    return RTX_OK;
}
/**************************************************************************//**
 * @brief       initialize a new task in the system,
 *              one dummy kernel stack frame, one dummy user stack frame
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       p_taskinfo  task information structure pointer
 * @param       p_tcb       the tcb the task is assigned to
 * @param       tid         the tid the task is assigned to
 *
 * @details     From bottom of the stack,
 *              we have user initial context (xPSR, PC, SP_USR, uR0-uR12)
 *              then we stack up the kernel initial context (kLR, kR0-kR12)
 *              The PC is the entry point of the user task
 *              The kLR is set to SVC_RESTORE
 *              30 registers in total
 *
 *****************************************************************************/
int k_tsk_create_new(RTX_TASK_INFO *p_taskinfo, TCB *p_tcb, task_t tid)
{
    extern U32 SVC_RESTORE;

    U32 *sp;

    if (p_taskinfo == NULL || p_tcb == NULL)
    {
        return RTX_ERR;
    }

    p_tcb->tid = tid;
    p_tcb->state = READY;
    p_tcb->prio = p_taskinfo->prio;
    p_tcb->priv = p_taskinfo->priv;
    p_tcb->countL = 0;
    p_tcb->countH = 0;

    /*---------------------------------------------------------------
     *  Step1: allocate kernel stack for the task
     *         stacks grows down, stack base is at the high address
     * -------------------------------------------------------------*/

    ///////sp = g_k_stacks[tid] + (K_STACK_SIZE >> 2) ;
    sp = k_alloc_k_stack(tid);
    p_taskinfo->k_stack_hi = (U32)sp;
    p_taskinfo->k_stack_size = K_STACK_SIZE;

    // 8B stack alignment adjustment
    if ((U32)sp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
        sp--;               // adjust it to 8B aligned
    }

    /*-------------------------------------------------------------------
     *  Step2: create task's user/sys mode initial context on the kernel stack.
     *         fabricate the stack so that the stack looks like that
     *         task executed and entered kernel from the SVC handler
     *         hence had the user/sys mode context saved on the kernel stack.
     *         This fabrication allows the task to return
     *         to SVC_Handler before its execution.
     *
     *         16 registers listed in push order
     *         <xPSR, PC, uSP, uR12, uR11, ...., uR0>
     * -------------------------------------------------------------*/

    // if kernel task runs under SVC mode, then no need to create user context stack frame for SVC handler entering
    // since we never enter from SVC handler in this case
    // uSP: initial user stack
    if ( p_taskinfo->priv == 0 ) { // unprivileged task
        // xPSR: Initial Processor State
        *(--sp) = INIT_CPSR_USER;
        // PC contains the entry point of the user/privileged task
        *(--sp) = (U32) (p_taskinfo->ptask);

        //********************************************************************//
        //*** allocate user stack from the user space, not implemented yet ***//
        //********************************************************************//
		void *p_stack = k_alloc_p_stack(tid);
		if (p_stack == NULL) return RTX_ERR;
        *(--sp) = (U32) p_stack;

        // uR12, uR11, ..., uR0
        for ( int j = 0; j < 13; j++ ) {
            *(--sp) = 0x0;
        }
    }


    /*---------------------------------------------------------------
     *  Step3: create task kernel initial context on kernel stack
     *
     *         14 registers listed in push order
     *         <kLR, kR0-kR12>
     * -------------------------------------------------------------*/
    if ( p_taskinfo->priv == 0 ) {
        // user thread LR: return to the SVC handler
        *(--sp) = (U32) (&SVC_RESTORE);
    } else {
        // kernel thread LR: return to the entry point of the task
        *(--sp) = (U32) (p_taskinfo->ptask);
    }

    // kernel stack R0 - R12, 13 registers
    for ( int j = 0; j < 13; j++) {
        *(--sp) = 0x0;
    }

    // kernel stack CPSR
    *(--sp) = (U32) INIT_CPSR_SVC;
    p_tcb->ksp = sp;

    return RTX_OK;
}

/**************************************************************************//**
 * @brief       switching kernel stacks of two TCBs
 * @param:      p_tcb_old, the old tcb that was in RUNNING
 * @return:     RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre:        gp_current_task is pointing to a valid TCB
 *              gp_current_task->state = RUNNING
 *              gp_crrent_task != p_tcb_old
 *              p_tcb_old == NULL or p_tcb_old->state updated
 * @note:       caller must ensure the pre-conditions are met before calling.
 *              the function does not check the pre-condition!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *****************************************************************************/
__asm void k_tsk_switch(TCB *p_tcb_old)
{
        PUSH    {R0-R12, LR}
        MRS 	R1, CPSR
        PUSH 	{R1}
        STR     SP, [R0, #TCB_KSP_OFFSET]   ; save SP to p_old_tcb->ksp
        LDR     R1, =__cpp(&gp_current_task);
        LDR     R2, [R1]
        LDR     SP, [R2, #TCB_KSP_OFFSET]   ; restore ksp of the gp_current_task
        POP		{R0}
        MSR		CPSR_cxsf, R0
        POP     {R0-R12, PC}
}


/**************************************************************************//**
 * @brief       run a new thread. The caller becomes READY and
 *              the scheduler picks the next ready to run task.
 * @return      RTX_ERR on error and zero on success
 * @pre         gp_current_task != NULL && gp_current_task == RUNNING
 * @post        gp_current_task gets updated to next to run task
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *****************************************************************************/
int k_tsk_run_new(void)
{
    TCB *p_tcb_old = NULL;

    if (gp_current_task == NULL) {
    	return RTX_ERR;
    }

    p_tcb_old = gp_current_task;
    gp_current_task = scheduler();

    if ( gp_current_task == NULL  ) {
        gp_current_task = p_tcb_old;        // revert back to the old task
        return RTX_ERR;
    }

    // at this point, gp_current_task != NULL and p_tcb_old != NULL
    if (gp_current_task != p_tcb_old) {
        gp_current_task->state = RUNNING;   // change state of the to-be-switched-in  tcb
        if (p_tcb_old->state != DORMANT){
            p_tcb_old->state = READY;           // change state of the to-be-switched-out tcb
        }
        k_tsk_switch(p_tcb_old);            // switch stacks
    }

    return RTX_OK;
}

/**************************************************************************//**
 * @brief       yield the cpu
 * @return:     RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre:        gp_current_task != NULL &&
 *              gp_current_task->state = RUNNING
 * @post        gp_current_task gets updated to next to run task
 * @note:       caller must ensure the pre-conditions before calling.
 *****************************************************************************/
int k_tsk_yield(void)
{
	if (sched_peak()->prio == NULL || (gp_current_task->prio < sched_peak()->prio && gp_current_task->prio != 0)){
		return RTX_OK;
	}

//    TCB *p_potential_new_task_to_run = scheduler();
//
//    if (p_potential_new_task_to_run == NULL || (gp_current_task->prio < p_potential_new_task_to_run->prio && gp_current_task->prio != 0)){
//        // keep running current task
//        return RTX_OK;
//    }
//
//    // otherwise switch to new task
//    switch_task(p_potential_new_task_to_run->tid, 0, 1);
//    //TCB *p_old_task = gp_current_task;
//    //gp_current_task = p_potential_new_task_to_run;
//    //gp_current_task->state = RUNNING;  // change state of the to-be-switched-in  tcb
//    //p_old_task->state = READY;         // change state of the to-be-switched-out tcb
//    //sched_pop();
//    //sched_insert(p_old_task);
//    //k_tsk_switch(p_old_task);          // switch stacks
//
//    return RTX_OK;

    return k_tsk_run_new();
}


/*
 *===========================================================================
 *                             TO BE IMPLEMETED IN LAB2
 *===========================================================================
 */

int k_tsk_create(task_t *task, void (*task_entry)(void), U8 prio, U16 stack_size)
{
    if (task == NULL || task_entry == NULL || g_num_active_tasks == MAX_TASKS-1 || stack_size < U_STACK_SIZE || prio == PRIO_NULL || prio == PRIO_RT) return RTX_ERR;
    // invalid state of RTX??????

//    // alloc stack for user task
//    // change to k_alloc_p_stack
//    unsigned int usptr = k_mem_alloc(stack_size);
//    if (usptr == NULL) return RTX_ERR;
    
    // assign task ID
    *task = U_TID_Q[U_TID_head];
    task_t tid = *task;
    U_TID_head = ++U_TID_head % (MAX_TASKS-1);
    ++g_num_active_tasks;

    // assign the task info // we don't need all of these
    RTX_TASK_INFO *task_info = &(U_rtx_task_infos[*task-1]);
    task_info->ptask = task_entry;
//    task_info->k_stack_hi = k_alloc_k_stack(*task);
//    task_info->k_stack_size = K_STACK_SIZE;
//    task_info->u_stack_hi = usptr + stack_size -1;
    task_info->u_stack_size = stack_size;
    task_info->tid = tid;
    task_info->prio = prio;
    task_info->state = READY;
    task_info->priv = 0;
    
    // update TCB
//    g_tcbs[tid].ksp = task_info->k_stack_hi;
//    g_tcbs[tid].tid = tid;
//    g_tcbs[tid].prio = prio;
//    g_tcbs[tid].state = READY;
//    g_tcbs[tid].priv = 0;

    // set up new task
    g_tcbs[tid].u_stack_size = stack_size;
    if (k_tsk_create_new(task_info, &g_tcbs[tid], tid) == RTX_ERR){
	    U_TID_Q[U_TID_tail] = tid;
	    U_TID_tail = ++U_TID_tail % (MAX_TASKS-1);
	    --g_num_active_tasks;
	    return RTX_ERR;
    }

    sched_insert(&g_tcbs[tid]);
    if (prio < gp_current_task->prio){
        // switch to new task
    	k_tsk_run_new();

//	    switch_task(tid, 0, 0);
        return RTX_OK;
    }

    // otherwise insert into scheduler ready heap
//    sched_insert(&g_tcbs[tid]);

#ifdef DEBUG_0
    printf("k_tsk_create: entering...\n\r");
    printf("task = 0x%x, task_entry = 0x%x, prio=%d, stack_size = %d\n\r", task, task_entry, prio, stack_size);
#endif /* DEBUG_0 */
    return RTX_OK;

}

void k_tsk_exit(void) 
{
//    if (g_tcbs[gp_current_task->tid].priv == 0 && k_dealloc_p_stack(gp_current_task->tid) == RTX_ERR){
//    	while (1){}
//    }
    // set tid number to free
    U_TID_Q[U_TID_tail] = gp_current_task->tid;
    U_TID_tail = ++U_TID_tail % (MAX_TASKS-1);
    --g_num_active_tasks;

    // then run the new task
//    switch_task(sched_peak()->tid, 1, 1);
    k_dealloc_p_stack(gp_current_task->tid);
    gp_current_task->state = DORMANT;
    k_tsk_run_new();

#ifdef DEBUG_0
    printf("k_tsk_exit: entering...\n\r");
#endif /* DEBUG_0 */
    return;
}

int k_tsk_set_prio(task_t task_id, U8 prio) 
{
	//TODO: check that task_id is valid
    if (prio == 255 || prio == 0 || task_id == 0 || task_id >= MAX_TASKS) return RTX_ERR;

    if (gp_current_task->priv == 0 && g_tcbs[task_id].priv == 1) return RTX_ERR;

    if (g_tcbs[task_id].state == DORMANT) return RTX_ERR;

    if (gp_current_task->tid == task_id){
        gp_current_task->prio = prio;
        if (prio >= sched_peak()->prio){
        	k_tsk_run_new();
//        	switch_task(sched_peak()->tid, 0, 1);
            // run new task and re-insert current task
        }
        return RTX_OK;
    }

    // else task_id != current task
    g_tcbs[task_id].prio = prio;
    if (prio < gp_current_task->prio){
//	    switch_task(task_id, 0, 1);
    	k_tsk_run_new();
        // preempt, switch to new task
    }
    else {
        sched_remove(task_id);
        sched_insert(&g_tcbs[task_id]);
    }

#ifdef DEBUG_0
    printf("k_tsk_set_prio: entering...\n\r");
    printf("task_id = %d, prio = %d.\n\r", task_id, prio);
#endif /* DEBUG_0 */
    return RTX_OK;    
}

int k_tsk_get_info(task_t task_id, RTX_TASK_INFO *buffer)
{
#ifdef DEBUG_0
    printf("k_tsk_get_info: entering...\n\r");
    printf("task_id = %d, buffer = 0x%x.\n\r", task_id, buffer);
#endif /* DEBUG_0 */    
    if (buffer == NULL || task_id > MAX_TASKS-1) {
        return RTX_ERR;
    }
    // check for invalid task ID
    for (int i = U_TID_head; i != U_TID_head; ++i ){
        if(i == MAX_TASKS-1){
            i = 0;
        }

        if (U_TID_Q[i] == task_id){
            return RTX_ERR;
        }
    }

    /* The code fills the buffer with some fake task information. 
       You should fill the buffer with correct information    */
    RTX_TASK_INFO *task_info = task_id == 0 ? &g_null_task_info : &U_rtx_task_infos[task_id-1];

    task_info->state = g_tcbs[task_id].state; //update the prio and state from the tcb
    task_info->prio = g_tcbs[task_id].prio;

    buffer->tid = task_id;
    buffer->prio = task_info->prio;
    buffer->state = task_info->state;
    buffer->priv = task_info->priv;
    buffer->ptask = task_info->ptask;
    buffer->k_stack_size = task_info->k_stack_size;
    buffer->u_stack_size = task_info->u_stack_size;
    buffer->k_stack_hi = task_info->k_stack_hi;
    buffer->u_stack_hi = task_info->u_stack_hi;

    return RTX_OK;     
}

task_t k_tsk_get_tid(void)
{
#ifdef DEBUG_0
    printf("k_tsk_get_tid: entering...\n\r");
#endif /* DEBUG_0 */ 
    return gp_current_task->tid;
}

int k_tsk_ls(task_t *buf, int count){
#ifdef DEBUG_0
    printf("k_tsk_ls: buf=0x%x, count=%d\r\n", buf, count);
#endif /* DEBUG_0 */
    return 0;
}

/*
 *===========================================================================
 *                             TO BE IMPLEMETED IN LAB4
 *===========================================================================
 */

int k_tsk_create_rt(task_t *tid, TASK_RT *task)
{
    return 0;
}

void k_tsk_done_rt(void) {
#ifdef DEBUG_0
    printf("k_tsk_done: Entering\r\n");
#endif /* DEBUG_0 */
    return;
}

void k_tsk_suspend(TIMEVAL *tv)
{
#ifdef DEBUG_0
    printf("k_tsk_suspend: Entering\r\n");
#endif /* DEBUG_0 */
    return;
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
