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
#include "ae_priv_tasks.h"
#include "ae_usr_tasks.h"
#include "Serial.h"
#include "printf.h"
#include "k_mem.h"

task_t ktid1;
task_t ktid2;
task_t ktid3;

#if TEST == 1
task_t utid[3];
#define TOTAL_STEPS 11
char* pointer;
static char validators[TOTAL_STEPS];

void ktask1(void){
	pointer = validators;
	printf("Start");
	if(k_tsk_create(&utid[0],utask1, 125, 0x200) != RTX_OK)
	  printf("[ktask1] FAILED to create user task.\r\n");

	if(k_tsk_create(&utid[1],utask2, 150, 0x200) != RTX_OK)
	  printf("[ktask1] FAILED to create user task.\r\n");

	if(k_tsk_create(&utid[2],utask3, 200, 0x200) != RTX_OK)
	  printf("[ktask1] FAILED to create user task.\r\n");

  k_tsk_exit();
}

void ktask2(void){
	printf("1 ");
	*pointer = 1;
	pointer++;
	k_tsk_exit();
}

void ktask3(void){
	printf("4 ");
	*pointer = 4;
	pointer++;
	k_tsk_exit();
}

#endif


#if TEST == 2
void ktask1(void){
    char eflag = 0;
	printf("SPAWNING TASKS!\r\n");
	task_t utid;
	if(k_tsk_create(&utid, &utask1, 200, 0x200) != RTX_OK){
		printf("FAILED to create user task from within privileged task!\r\n");
		eflag++;
	}
	printf("%d out of 4 tests passed!\r\n", 1-eflag);
	task_t ntid;
	if(k_tsk_create(&ntid, &utask2, PRIO_NULL, 0x200) == RTX_OK){
		printf("FAILED! Tasks cannot be created with priority PRIO_NULL!\r\n");
		eflag++;
	}
	printf("%d out of 4 tests passed!\r\n", 2-eflag);
	//Add null for tid and pointer to function
	if(k_tsk_create(&ntid, NULL, 200, 0x200) == RTX_OK){
		printf("[T_05] FAILED! NULL passed as function entry point\r\n");
		eflag++;
	}
	printf("%d out of 4 tests passed!\r\n", 3-eflag);
	if(k_tsk_create(NULL, &utask2, 200, 0x200) == RTX_OK){
		printf("[T_05] FAILED! NULL passed as function entry point\r\n");
		eflag++;
	}
	printf("%d out of 4 tests passed!\r\n", 4-eflag);

	printf("============================================\r\n");
	printf("=============Final test results=============\r\n");
	printf("============================================\r\n");
	printf("%d out of 4 tests passed!\r\n", 4-eflag);

	k_tsk_exit();
}

#endif

#if TEST == 3
char eflag = 0;
extern volatile int counter;
void* my_memory;
char ownership_failed;
void ktask1(void){
	ownership_failed = 0;
	task_t utid1;
	task_t utids[MAX_TASKS-4];
	char internal_error = 0;
	for(int i =0; i < MAX_TASKS-4; i++){
		if(k_tsk_create(&utids[i], &utask1, 150, 0xFFF0) != RTX_OK){
			printf("FAILED to create user task!\r\n");
			internal_error++;
		}
	}
	if(internal_error)
		eflag++;
	if(k_tsk_create(&utid1, &utask2, 150, 0x9) == RTX_OK){
		printf("FAILED: created a user task with stack size that is not multiple of eight!\r\n");
		eflag++;
	}

	my_memory = k_mem_alloc(0xFF);
	if(my_memory == NULL)
		printf("Failed to allocate memory!\r\n");

	printf("Trying to exhaust memory, this may take a while... \r\n");

	int size = 0x3FFFFFFF;

	while(size > 0xFFF0){
		while(k_mem_alloc(size) != NULL);
		size = size >> 1;
	}



	//Resume execution after all created tasks complete
	if(k_tsk_create(&utid1, &utask2, 200, 0xFFFF) == RTX_OK){
		printf("FAILED: created task on system without memory!\r\n");
		eflag++;
	}
	internal_error = 0;
	for(int i =0; i < MAX_TASKS-4; i++){
		if(k_tsk_set_prio(utids[i], 100) != RTX_OK){
			printf("FAILED to set priority!\r\n");
			internal_error++;
		}
	}
	if(internal_error)
		eflag++;
	if(counter != MAX_TASKS-4)
		eflag++;
	counter = 0;
	internal_error = 0;
	for(int i =0; i < MAX_TASKS-4; i++){
		if(k_tsk_create(&utids[i], &utask1, 150, 0xFFF0) != RTX_OK){
			printf("FAILED to create user tasks: Stack deallocation might not be done properly once a task exits!\r\n");
			internal_error++;
		}
	}
	if(internal_error)
		eflag++;

	for(int i =0; i < MAX_TASKS-4; i++){
		if(k_tsk_set_prio(utids[i], 100) != RTX_OK){
			printf("FAILED to set priority!\r\n");
			internal_error++;
		}
	}
	if(internal_error)
		eflag++;
	if(counter != MAX_TASKS-4)
		eflag++;


	printf("============================================\r\n");
	printf("=============Final test results=============\r\n");
	printf("============================================\r\n");
	printf("%d out of 8 tests passed!\r\n", 8-eflag);
	printf("%d out of 1 tests passed!\r\n", 1-ownership_failed);

    k_tsk_exit();

}
#endif

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
