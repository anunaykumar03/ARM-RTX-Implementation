/**
 * @file:   k_msg.c
 * @brief:  kernel message passing routines
 * @author: Yiqing Huang
 * @date:   2020/10/09
 */

#include "k_msg.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

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
	if (receiver_tid == 0 || receiver_tid >= MAX_TASKS || g_tcbs[receiver_tid].mailbox_lo == NULL
		|| g_tcbs[receiver_tid].mailbox_size == g_tcbs[receiver_tid].mailbox_capacity || g_tcbs[receiver_tid].state == DORMANT) return RTX_ERR;

	if (buf == NULL || ((RTX_MSG_HDR *)buf)->length + sizeof(mail_metadata) > g_tcbs[receiver_tid].mailbox_capacity - g_tcbs[receiver_tid].mailbox_size) return RTX_ERR;

	U8* recv_box = g_tcbs[receiver_tid].mailbox_lo;
	U32 *head = &g_tcbs[receiver_tid].mail_head;
	U32 *tail = &g_tcbs[receiver_tid].mail_tail;

	// set metadata
	mailbox_metadata_t* metadata = recv_box + head;
	metadata->sender_tid = gp_current_task->tid;

	U32 msg_len = ((RTX_MSG_HDR *)buf)->length;
	U32 mb_cap = g_tcbs[receiver_tid].mailbox_capacity;
	*head = (*head + sizeof(mailbox_metadata_t)) % mb_cap;

	// copy message (includes msg header)
	for (int i = 0; i < msg_len; i++) {
		recv_box[*head] = buf[i];
		*head = (*head + 1) % mb_cap;
	}

	g_tcbs[receiver_tid].mailbox_size += sizeof(mailbox_metadata_t) + msg_len;

	// unblock receiver & switch if needed
	if(g_tcbs[receiver_tid].state == BLK_MSG){
		g_tcbs[receiver_tid].state = READY;
		sched_insert(receiver_tid);
		k_tsk_yield();
	}

	return RTX_OK;

#ifdef DEBUG_0
    printf("k_send_msg: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */
}

int k_recv_msg(task_t *sender_tid, void *buf, size_t len) {

#ifdef DEBUG_0
    printf("k_recv_msg: sender_tid  = 0x%x, buf=0x%x, len=%d\r\n", sender_tid, buf, len);
#endif /* DEBUG_0 */
    return 0;
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
