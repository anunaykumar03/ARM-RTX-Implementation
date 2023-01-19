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
#define DEBUG_0
#ifdef DEBUG_0
#include "printf.h"
#endif  /* DEBUG_0 */
/*
 *==========================================================================
 *                           STRUCTS & TYPEDEFS
 *==========================================================================
 */
// probably combine these structs later
// header struct for allocated mem
typedef struct {
    unsigned int size;
    unsigned int pad1;
    unsigned int pad2;
    unsigned int pad3;
} header_t;

// node struct for free mem
typedef struct node_t node_t;
struct node_t {
    unsigned int size;
    node_t *next;
};

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
unsigned int list_size;


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
    return g_p_stacks[tid+1];
}

int k_mem_init(void) {
    unsigned int end_addr = (unsigned int) &Image$$ZI_DATA$$ZI$$Limit;
    // check region size
    if (RAM_END - end_addr < sizeof(node_t)){
        // free space too small to manage
        return RTX_ERR;
    }
    // create linked list 
    head = (node_t *)end_addr;
    head->size = RAM_END-end_addr;
    head->next = NULL;
    list_size = 1;

#ifdef DEBUG_0
    printf("k_mem_init: image ends at 0x%x\r\n", end_addr);
    printf("k_mem_init: RAM ends at 0x%x\r\n", RAM_END);
#endif /* DEBUG_0 */
    return RTX_OK;
}

void* k_mem_alloc(size_t size) {
#ifdef DEBUG_0
    printf("k_mem_alloc: requested memory size = %d\r\n", size);
#endif /* DEBUG_0 */

    // round size up to nearest multiple of 4
    if(size & 0x3){
        size = (size & ~0x3) + 4;
    }
    unsigned int find_size = size + sizeof(header_t);
    node_t *curr_node = head; 
    node_t *prev_node = NULL;
    for (int i = 0; i < list_size; ++i) {
        if (curr_node->size >= find_size) {
            unsigned int size_left = curr_node->size - find_size;
            if (size_left == 0){
            	printf("debug 0\n");
                // remove node
                if (i == 0) {
                    head = curr_node->next;
                }
                else {
                    prev_node->next = curr_node->next;
                }
                --list_size;
            }
            else {
                // update node
            	printf("debug 1\n");
                if (i == 0) {
                	printf("debug 2\n");
                	printf("updating free list\n");
                    head = (node_t *)((unsigned int)curr_node + find_size);
                    head->size = curr_node->size - find_size;
                    head->next = curr_node->next;
                    printf("head: %u, curr_node: %u, size: %u, find_size: %u\n", head, curr_node, size, find_size);
                }
                else {
                	printf("debug 3\n");
                    prev_node->next = (node_t *)((unsigned int)curr_node + find_size);
                    prev_node->next->size = curr_node->size - find_size;
                    prev_node->next->next = curr_node->next;
                }
            }
            ((header_t *)curr_node)->size = size;
            return (void *)((unsigned int)curr_node + sizeof(header_t));
        }
        prev_node = curr_node;
        curr_node = curr_node->next;
    }

    // no big enough free node found
    return NULL;
}

int k_mem_dealloc(void *ptr) {
    if (ptr == NULL){
        return RTX_OK;
    }

    node_t *node_ptr = (node_t *)((header_t *)ptr - 1);
    node_ptr->size = ((header_t *)node_ptr)->size + sizeof(header_t);

    node_t *curr_node = head;
    node_t *prev_node = NULL;
    // insert new free node into list
    for (int i = 0; i <= list_size; ++i){
        if (node_ptr < curr_node || i == list_size){
            ++list_size;
            node_ptr->next = curr_node;
            // coalesce with next node
            if (curr_node != NULL && (unsigned int)node_ptr + node_ptr->size == (unsigned int)curr_node){
                node_ptr->size += curr_node->size;
                node_ptr->next = curr_node->next;
                --list_size;
            }
            if (i == 0){
                // move head if inserted before first node
                head = node_ptr;
            }
            else {
                prev_node->next = node_ptr;
                // coalesce with previous node
                if ((unsigned int)prev_node + prev_node->size == (unsigned int)node_ptr){
                    prev_node->size += node_ptr->size;
                    prev_node->next = node_ptr->next;
                    --list_size;
                }
            }
            return RTX_OK;
        }
        prev_node = curr_node;
        curr_node = curr_node->next;
    }

#ifdef DEBUG_0
    printf("k_mem_dealloc: freeing 0x%x\r\n", (U32) ptr);
#endif /* DEBUG_0 */
    return RTX_ERR;
}

int k_mem_count_extfrag(size_t size) {
    node_t *curr_node = head;
    unsigned int count = 0;
    for (int i = 0; i < list_size; ++i){
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
