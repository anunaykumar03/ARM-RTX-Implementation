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
 * @file        k_mem.c
 * @brief       Kernel Memory Management API C Code
 *
 * @version     V1.2021.01.lab2
 * @authors     Yiqing Huang
 * @date        2021 JAN
 *
 * @note        skeleton code
 *
 *****************************************************************************/

/** 
 * @brief:  k_mem.c kernel API implementations, this is only a skeleton.
 * @author: Yiqing Huang
 */

#include "k_mem.h"
#include "Serial.h"
#include "k_task.h"
//#define DEBUG_0
#ifdef DEBUG_0
#include "printf.h"
#endif  /* DEBUG_0 */

#define SET_BIT(var, bit) (var |= (1U << bit))
#define CLEAR_BIT(var, bit) (var &= ~(1U << bit))

#define FREE_SET_FL_POSITION (31)
#define FREE_IS_FL_SET(a) (a >> FREE_SET_FL_POSITION)
#define FREE_SET_FL(a) (SET_BIT(a, FREE_SET_FL_POSITION))
#define FREE_CLEAR_FL(a) (CLEAR_BIT(a, FREE_SET_FL_POSITION))
#define NO_FL_SIZE(a) (a & ~(1U << FREE_SET_FL_POSITION))
#define RAM_END1 RAM_END+1

/*
 *==========================================================================
 *                           STRUCTS & TYPEDEFS
 *==========================================================================
 */
typedef struct node_t node_t;
struct node_t {
    unsigned int size;
    node_t *next;
};

typedef struct {
	unsigned int size;
    unsigned int owner_tid;
} header_t;

/*
 *==========================================================================
 *                            GLOBAL VARIABLES
 *==========================================================================
 */
// kernel stack size, referred by startup_a9.s
const U32 g_k_stack_size = K_STACK_SIZE;
// task proc space stack size in bytes, referred by system_a9.cs
const U32 g_p_stack_size = U_STACK_SIZE;

// task kernel stacks
U32 g_k_stacks[MAX_TASKS][K_STACK_SIZE >> 2] __attribute__((aligned(8)));

//process stack for tasks in SYS mode
U32 g_p_stacks[MAX_TASKS][U_STACK_SIZE >> 2] __attribute__((aligned(8)));

// pointer to start of list
node_t *head;
node_t *free_head;

/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */

U32* k_alloc_k_stack(task_t tid)
{
    return g_k_stacks[tid+1];
}

U32* k_alloc_p_stack(task_t tid)
{
	U32 size = U_rtx_task_infos[tid-1].u_stack_size;
	U8 *ptr = k_mem_alloc(size);
	return (U32 *)((U32)(ptr + size - 1) & ~(0x3)); // return hi addr
//    return g_p_stacks[tid+1];
}

int k_mem_init(void) {
    unsigned int end_addr = (unsigned int) &Image$$ZI_DATA$$ZI$$Limit;
    // check region size
    if (RAM_END1 - end_addr < sizeof(node_t)){ // fits header but wont be able to alloc anything, is this ok?
        // free space too small to manage
        return RTX_ERR;
    }
    // create linked list 
    head = (node_t *)end_addr;
    head->size = RAM_END1-end_addr;
    head->next = NULL;
    free_head = head;

#ifdef DEBUG_0
    printf("k_mem_init: image ends at 0x%x\r\n", end_addr);
    printf("k_mem_init: RAM ends at 0x%x\r\n", RAM_END);
#endif /* DEBUG_0 */
    return RTX_OK;
}

void* k_mem_alloc_internals(size_t size, task_t owner){
#ifdef DEBUG_0
    printf("k_mem_alloc: requested memory size = %d\r\n", size);
#endif /* DEBUG_0 */
    //printf("size of node is %d\r\n", sizeof(node_t));
    // round size up to nearest multiple of 4
    // if(size & 0x3){
    //     size = (size & ~0x3) + 4;
    // }
    size = (size + 3) & ~3;
    // size = size + 4 - (size & 0x3)
    unsigned int find_size = size + sizeof(header_t);
    node_t *curr_node = free_head;
    node_t *prev_node = NULL;
    while (curr_node != NULL) {
        if (curr_node->size >= find_size) {
            unsigned int size_left = curr_node->size - find_size;
        	node_t *temp = curr_node->next;
            if(size_left >= sizeof(header_t)){
            	// split node
            	temp = (node_t *)((unsigned int)curr_node + find_size);
            	temp->size = size_left;
                temp->next = curr_node->next;
                curr_node->size = find_size; // note that for both free and alloced nodes, the size is total size including sizeof(node_t)
            }
            if (prev_node == NULL){
            	free_head = temp;
            }
            else {
            	prev_node->next = temp;
            }
            ((header_t *)curr_node)->owner_tid = owner;
            return (void *)((unsigned int)curr_node + sizeof(header_t));
        }
        prev_node = curr_node;
        curr_node = curr_node->next;
    }

    // no big enough free node found
    return NULL;
}

void* k_mem_alloc(size_t size) {
	return k_mem_alloc_internals(size, gp_current_task->tid);
}

int k_mem_dealloc_internals(void *ptr, task_t owner){
    if ((ptr == NULL) || ((unsigned int)ptr < (unsigned int)head + sizeof(header_t)) || ((unsigned int)ptr & 0x3) || ((unsigned int)ptr > RAM_END) ){
        return RTX_ERR;
    }


    node_t *node_ptr = (node_t *)((header_t *)ptr - 1);
    node_t *start_node = head;
    node_t *end_node = free_head;

    // check ownership
    if (owner != ((header_t *)node_ptr)->owner_tid) {
    	return RTX_ERR;
    }

    while (end_node != NULL && end_node < node_ptr){
    	start_node = end_node;
    	end_node = end_node->next;
    }

    node_t *temp = start_node;
	while ((unsigned int)temp < RAM_END1 && temp != end_node){
		if (temp == node_ptr){
			// found the node to free
			temp->next = end_node;
			if(start_node == head && free_head != head){
				free_head = temp;
			} else {
				start_node->next = temp;
			}

			if (temp->next != NULL && (unsigned int)temp + temp->size == (unsigned int)(temp->next)){
				temp->size += temp->next->size;
				temp->next = temp->next->next;
			}
			if ((start_node != head || start_node == free_head) && (unsigned int)start_node + start_node->size == (unsigned int)temp){
				start_node->size += temp->size;
				start_node->next = temp->next;
			}
			return RTX_OK;
		}
		temp = (node_t*)((unsigned int)temp + temp->size);
	}

#ifdef DEBUG_0
    printf("k_mem_dealloc: freeing 0x%x\r\n", (U32) ptr);
#endif /* DEBUG_0 */
    return RTX_ERR;
}

int k_mem_dealloc(void *ptr) {
	return k_mem_dealloc_internals(ptr, gp_current_task->tid);
}

int k_mem_count_extfrag(size_t size) {
    node_t *curr_node = free_head;
    unsigned int count = 0;
    while (curr_node != NULL) {
        if (curr_node->size < size){
            ++count;
        }
        curr_node = curr_node->next;
    }
    return count;

#ifdef DEBUG_0
    printf("k_mem_extfrag: size = %d\r\n", size);
#endif /* DEBUG_0 */
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
