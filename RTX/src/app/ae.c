/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                 Copyright 2020-2021 ECE 350 Teaching Team
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

#include "ae.h"

/**************************************************************************//**
 * @brief   	ae_init
 * @return		RTX_OK on success and RTX_ERR on failure
 * @param[out]  sys_info system initialization struct AE writes to
 * @param[out]	task_info boot-time tasks struct array AE writes to
 *
 *****************************************************************************/

int ae_init(RTX_SYS_INFO *sys_info, RTX_TASK_INFO *task_info, int num_tasks) {
    if (ae_set_sys_info(sys_info) != RTX_OK) {
        return RTX_ERR;
    }

    ae_set_task_info(task_info, num_tasks);
    return RTX_OK;
}

/**************************************************************************//**
 * @brief       fill the sys_info struct with system configuration info.
 * @return		RTX_OK on success and RTX_ERR on failure
 * @param[out]  sys_info system initialization struct AE writes to
 *
 *****************************************************************************/
int ae_set_sys_info(RTX_SYS_INFO *sys_info) {
    if (sys_info == NULL) {
        return RTX_ERR;
    }

    // Scheduling sys info set up, only do DEFAULT in lab2
    sys_info->sched = DEFAULT;

    return RTX_OK;
}

/**************************************************************************//**
 * @brief       fill the tasks array with information
 * @param[out]  tasks 		An array of RTX_TASK_INFO elements to write to
 * @param[in]	num_tasks	The length of tasks array
 * @return		None
 *****************************************************************************/

void ae_set_task_info(RTX_TASK_INFO *tasks, int num_tasks) {

    if (tasks == NULL) {
    	printf("[ERROR] RTX_TASK_INFO undefined\n\r");
        return;
    }

#if TEST == 1
    printf("RUNNING\r\n");

	tasks[0].prio = 100;
	tasks[0].priv = 1;
	tasks[0].ptask = &ktask1;
	tasks[0].k_stack_size = (0x200);

  	tasks[1].prio = 125;
	tasks[1].priv = 1;
	tasks[1].ptask = &ktask2;
	tasks[1].k_stack_size = (0x200);

  	tasks[2].prio = 150;
	tasks[2].priv = 1;
	tasks[2].ptask = &ktask3;
	tasks[2].k_stack_size = (0x200);

#endif


#if TEST == 2
    printf("RUNNING\r\n");
  	tasks[0].prio = 100;
	tasks[0].priv = 1;
	tasks[0].ptask = &ktask1;
	tasks[0].k_stack_size = 0x200;

#endif

#if TEST == 3
    printf("RUNNING\r\n");

  	tasks[0].prio = 125;
	tasks[0].priv = 1;
	tasks[0].ptask = &ktask1;
	tasks[0].k_stack_size = 0x200;

#endif
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
