/**
 * @file:   k_msg.c
 * @brief:  kernel message passing routines
 * @author: Yiqing Huang
 * @date:   2020/10/09
 */

#include "k_msg.h"
#include "k_mem.h"
#include "prio_heap.h"
#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

extern void kcd_task(void);

int k_mbx_create(size_t size) {
	if (size < MIN_MBX_SIZE) return RTX_ERR;

	if (gp_current_task->mailbox_lo != NULL) return RTX_ERR;

	void* ptr = k_mem_alloc_internals(size, (task_t) 0);
	if (ptr == NULL) return RTX_ERR;

	gp_current_task->mailbox_lo = ptr;
	gp_current_task->mailbox_capacity = size;
	gp_current_task->mailbox_size = 0;
	gp_current_task->mail_head = 0;
	gp_current_task->mail_tail = 0;

#ifdef DEBUG_0
    printf("k_mbx_create: size = %d\r\n", size);
#endif /* DEBUG_0 */
    return 0;
}

int k_send_msg(task_t receiver_tid, const void *buf) {

	if (receiver_tid == 0 || receiver_tid >= MAX_TASKS) return RTX_ERR;

	TCB * receiver_tcb = k_tsk_get_tcb(receiver_tid);
	
	if( receiver_tcb->mailbox_lo == NULL
		|| receiver_tcb->mailbox_size == receiver_tcb->mailbox_capacity || receiver_tcb->state == DORMANT) return RTX_ERR;

	if (buf == NULL || ((RTX_MSG_HDR *)buf)->length < MIN_MSG_SIZE || ((RTX_MSG_HDR *)buf)->length + sizeof(mailbox_metadata_t) > receiver_tcb->mailbox_capacity - receiver_tcb->mailbox_size) return RTX_ERR;

	U8* recv_box = receiver_tcb->mailbox_lo;
	U32 *head = &receiver_tcb->mail_head;

	// set metadata
	mailbox_metadata_t* metadata = (mailbox_metadata_t *)(recv_box + *head);
	metadata->sender_tid = gp_current_task->tid;
	if (UART_IRQ_flag){
		// message is from UART
		metadata->sender_tid = TID_UART_IRQ;

	}

	U32 msg_len = ((RTX_MSG_HDR *)buf)->length;
	U32 mb_cap = receiver_tcb->mailbox_capacity;
	*head = (*head + sizeof(mailbox_metadata_t)) % mb_cap;

    U8* rec_buf = (U8 *)buf;
	// copy message (includes msg header)
	for (int i = 0; i < msg_len; i++) {
		recv_box[*head] = rec_buf[i];
		*head = (*head + 1) % mb_cap;
	}

	receiver_tcb->mailbox_size += sizeof(mailbox_metadata_t) + msg_len;

	// unblock receiver & switch if needed
	if(receiver_tcb->state == BLK_MSG){
		receiver_tcb->state = READY;
		sched_insert(receiver_tcb);
		k_tsk_yield();
	}

	return RTX_OK;

#ifdef DEBUG_0
    printf("k_send_msg: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */
}

int k_recv_msg(task_t *sender_tid, void *buf, size_t len) {
    if (gp_current_task->mailbox_lo == NULL) return RTX_ERR;
    if (gp_current_task->mailbox_size == 0) {
        // block current task
        gp_current_task->state = BLK_MSG;
        sched_remove(gp_current_task->tid);
        k_tsk_run_new();
        return RTX_OK;
    }
    
	U32 *tail = &(gp_current_task->mail_tail);
    U8* start = (U8 *)(gp_current_task->mailbox_lo) + *tail;
    mailbox_metadata_t* metadata = (mailbox_metadata_t *)start;
    U8* msg_start = start + sizeof(mailbox_metadata_t);
	U32 mb_cap = gp_current_task->mailbox_capacity;
    
    U32 msg_len = ((RTX_MSG_HDR *)msg_start)->length;
    if (buf == NULL || msg_len > len) { // message does not fit in buf
        // discard top message
	    gp_current_task->mailbox_size -= sizeof(mailbox_metadata_t) + msg_len;
        *tail = (*tail + sizeof(mailbox_metadata_t) + msg_len) % mb_cap;
        return RTX_ERR;
    }

    U8* fill_buf = buf;
    for (int i = 0; i < msg_len; i++){
        fill_buf[i] = msg_start[*tail];
        *tail = (*tail + 1) % mb_cap;
    }
	gp_current_task->mailbox_size -= sizeof(mailbox_metadata_t) + msg_len;

    if (sender_tid != NULL){
        *sender_tid = metadata->sender_tid;
    }

    return RTX_OK;

#ifdef DEBUG_0
    printf("k_recv_msg: sender_tid  = 0x%x, buf=0x%x, len=%d\r\n", sender_tid, buf, len);
#endif /* DEBUG_0 */
}

int k_recv_msg_nb(task_t *sender_tid, void *buf, size_t len) {
#ifdef DEBUG_0
    printf("k_recv_msg_nb: sender_tid  = 0x%x, buf=0x%x, len=%d\r\n", sender_tid, buf, len);
#endif /* DEBUG_0 */
    return 0;
}

int k_mbx_ls(task_t *buf, int count) {
#ifdef DEBUG_0
    printf("k_mbx_ls: buf=0x%x, count=%d\r\n", buf, count);
#endif /* DEBUG_0 */
    return 0;
}
