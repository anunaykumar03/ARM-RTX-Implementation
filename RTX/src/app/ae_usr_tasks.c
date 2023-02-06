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
#include "ae_usr_tasks.h"
#include "rtx.h"
#include "Serial.h"
#include "printf.h"

task_t utid1;
task_t utid2;
task_t utid3;

#if TEST == 1
extern volatile char* pointer;
void utask1(void){
	printf("2 ");
	*pointer = 2;
	pointer++;
	tsk_yield();

	printf("3 ");

	*pointer = 3;
	pointer++;
	if(tsk_set_prio(tsk_get_tid(),150) != RTX_OK) {
	  printf("FAILED! Task could not set its own priority\r\n");
	}
	printf("6 ");

	*pointer = 6;
	pointer++;
	if(tsk_set_prio(tsk_get_tid(),200) != RTX_OK) {
	  printf("FAILED! Task could not set its own priority\r\n");
	}
	printf("9 ");

	*pointer = 9;
	pointer++;
	tsk_exit();
}


void utask2(void){
	printf("5 ");
	*pointer = 5;
	pointer++;
	if(tsk_set_prio(tsk_get_tid(),200) != RTX_OK) {
	  printf("FAILED! Task could not set its own priority\r\n");
	}

	printf("8 ");
	*pointer = 8;
	pointer++;

	tsk_yield();

	printf("11 \n\r");
	*pointer = 11;
	pointer++;
	int eflag = 0;
	for(int i=11; i >0; i--){
		if ((int)*(--pointer) != i)
			eflag = 1;
	}
	printf("============================================\r\n");
	printf("=============Final test results=============\r\n");
	printf("============================================\r\n");
	printf("%d out of 1 test passed!\r\n", 1-(eflag));

	tsk_exit();
}

void utask3(void){
	printf("7 ");
	*pointer = 7;
	pointer++;
	tsk_yield();

	printf("10 ");
	*pointer = 10;
	pointer++;
	tsk_exit();
}

#endif

#if TEST == 2
void utask1(void){
    tsk_exit();
}

void utask2(void){
    tsk_exit();
}
#endif

#if TEST == 3
extern char eflag;
extern volatile void* my_memory;
int counter = 0;
extern volatile char ownership_failed;
void utask1(void){
	if(mem_dealloc(my_memory) == RTX_OK)
		ownership_failed = 1;
	counter++;
	tsk_exit();
}


void utask2(void){
	tsk_exit();
}
#endif

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
