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
#include "prio_heap.h"

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
	unsigned int start = timer_get_current_val(2);

	ownership_failed = 0;
	task_t utid1;
	static task_t utids[MAX_TASKS-4];
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
		printf("Failed to allocate memory\n");

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
	volatile int temp_counter;
	if(internal_error)
		eflag++;
	temp_counter = counter;
	if(temp_counter != MAX_TASKS-4)
		eflag++;


	printf("============================================\r\n");
	printf("=============Final test results=============\r\n");
	printf("============================================\r\n");
	printf("%d out of 8 tests passed!\r\n", 8-eflag);
	printf("%d out of 1 tests passed!\r\n", 1-ownership_failed);

	unsigned int end = timer_get_current_val(2);
	printf("This took %u us\r\n", start-end);

	k_tsk_exit();
}
#endif

#if TEST == 4
void ktask1(void){
	printf("%u ", k_tsk_get_tid());
	k_tsk_yield();
	printf("%u ", k_tsk_get_tid());
  k_tsk_exit();
}
#endif

#if TEST == 5
//Set priority to its current priority and make sure that it gets added to the back of the queue
void ktask1(void){ 
	// should print 1234512345
	RTX_TASK_INFO info;
	k_tsk_get_info(k_tsk_get_tid(), &info);
	printf("%u %u\n", info.tid, info.prio);
	k_tsk_set_prio(info.tid, info.prio);
	printf("%u %u\n", info.tid, info.prio);
	k_tsk_exit();
}
#endif

#if TEST == 6

task_t utid[3];

void utask3(void){
	printf("7 ");
	tsk_exit();
}

void utask2(void){
	printf("4 ");
	if(tsk_create(&utid[2],utask3, 125, 0x200) != RTX_OK)
	  printf("[utask1] FAILED to create user task.\r\n");
	printf("5 ");

	tsk_exit();
}

void utask1(void){
	printf("2 ");
	if(tsk_create(&utid[1],utask2, 75, 0x200) != RTX_OK)
	  printf("[utask1] FAILED to create user task.\r\n");
	printf("3 ");

	tsk_exit();
}
// Test priority change of k_task_create
void ktask1(void){
	//create user task that is higher priority, should immediately start running that task (utask1)
	//Inside the user task that just ran, create another user task with same priority, this should be ran after (utask2)
	//Then create a task with same priority as the original ktask1, this task should run after the ktask finishes (utask3)
	printf("1 ");
	if(k_tsk_create(&utid[0],utask1, 75, 0x200) != RTX_OK)
	  printf("[ktask1] FAILED to create user task.\r\n");

	printf("6 ");
	k_tsk_exit();
}
#endif

#if TEST == 7
task_t utid[2];

void utask1(void) {
	printf("4");
	if (tsk_set_prio(k_tsk_get_tid(), 253) != RTX_OK){
		printf("FAILED to set prio\n");
	}
	if (tsk_set_prio(1, 100) != RTX_ERR){
		printf("FAILED should not have set prio\n");
	}
	tsk_exit();
}

void utask2(void) {
	printf("7");
	if (tsk_set_prio(k_tsk_get_tid(), 50) != RTX_OK){
		printf("FAILED to set prio\n");
	}
	printf("8");
	if (tsk_set_prio(k_tsk_get_tid(), 250) != RTX_OK){
		printf("FAILED to set prio\n");
	}
	printf("10");
	if (tsk_set_prio(k_tsk_get_tid(), 252) != RTX_OK){
		printf("FAILED to set prio\n");
	}
	printf("12");
	tsk_exit();
}

void ktask1(void){ // 125
	RTX_TASK_INFO info;

	printf("1");
	if (k_tsk_set_prio(k_tsk_get_tid(), 100) != RTX_OK){
		printf("FAILED to set prio\n");
	}
	printf("2");
	if (k_tsk_set_prio(k_tsk_get_tid(), 0) != RTX_ERR){
		printf("FAILED should not have set prio\n");
	}
	if (k_tsk_set_prio(k_tsk_get_tid(), 255) != RTX_ERR){
		printf("FAILED should not have set prio\n");
	}
	k_tsk_get_info(k_tsk_get_tid(), &info);
	if (info.prio != 100){
		printf("FAILED prio incorrect\n");
	}
	if (k_tsk_create(&utid[0], utask1, 100, 0x200) != RTX_OK){ // should not preempt
		printf("FAILED to set prio\n");
	}
	printf("3");
	if (k_tsk_set_prio(k_tsk_get_tid(), 254) != RTX_OK){
		printf("FAILED to set prio\n");
	}
	printf("14");
	k_tsk_get_info(utid[0], &info);
	if (info.state != DORMANT){
		printf("FAILED state info incorrect\n");
	}
	if (k_tsk_set_prio(utid[0], 254) != RTX_ERR){ // dormant task
		printf("FAILED should not have set prio\n");
	}
	k_tsk_get_info(utid[1], &info);
	if (info.state != DORMANT){
		printf("FAILED state info incorrect\n");
	}
	if (k_tsk_set_prio(utid[1], 254) != RTX_ERR){ // dormant task
		printf("FAILED should not have set prio\n");
	}
	k_tsk_get_info(2, &info);
	if (info.state != DORMANT){
		printf("FAILED state info incorrect\n");
	}
	if (k_tsk_set_prio(2, 254) != RTX_ERR){ // dormant task
		printf("FAILED should not have set prio\n");
	}
	printf("15");
	k_tsk_exit();
}

void ktask2(void){ // 200
	printf("5");
	if (k_tsk_set_prio(k_tsk_get_tid(), 101) != RTX_OK){
		printf("FAILED to set prio\n");
	}
	printf("6");
	if (k_tsk_create(&utid[1], utask2, 100, 0x200) != RTX_OK){ // should preempt
		printf("FAILED to create\n");
	}
	printf("9");
	if (k_tsk_set_prio(k_tsk_get_tid(), 251) != RTX_OK){
		printf("FAILED to set prio\n");
	}
	printf("11");
	if (k_tsk_set_prio(utid[1], 2) != RTX_OK){
		printf("FAILED to set prio\n");
	}
	printf("13");
	k_tsk_exit();
}
#endif

#if TEST == 8
task_t utid[10];
void ktask1(void){
    printf("SUB TEST 1!\n");
    //case 1: yield when queue is empty (we are already running :)), 
    printf("1 ");
    k_tsk_yield();
    printf("2 ");
    
    //case 3: yield when there are multiple things in queue, one of them is same priority
    if(k_tsk_create(&utid[0], utask1, 100, 0x200) != RTX_OK){
        printf("ktask1 task creation failed!\n");
        k_tsk_exit();
        return;
    }

    printf("3 ");

    if(k_tsk_create(&utid[1], utask2, 115, 0x200) != RTX_OK){
        printf("ktask1 task creation failed!\n");
        k_tsk_exit();
        return;
    }

    printf("4 ");

    if(k_tsk_create(&utid[2], utask3, 115, 0x200) != RTX_OK){
        printf("ktask1 task creation failed!\n");
        k_tsk_exit();
        return;
    }

    printf("5 ");
    k_tsk_yield();
    printf("7 ");
    k_tsk_yield(); 
    printf("9 ");

    //case 4: yield when there are multiple things in queue, all of them are lower priority, should continue running
    k_tsk_yield();
    printf("10 ");
    k_tsk_exit();
}

void ktask2(void){
    printf("SUB TEST 2!\n");
    //case 2: multiple things in queue of same priority, a few back to back yields
    //spawn 4 tasks that each call yield 4 times
    for(int i = 0;i < 4;i++){
        if(k_tsk_create(&utid[i], utask4, 135, 0x200) != RTX_OK){
            printf("ktask2 task creation failed!\n");
            k_tsk_exit();
            return;
        }
    }

    printf("START ");
    k_tsk_exit();
}

void ktask3(void){
    printf("SUB TEST 3!\n");
    //case 5: yield when the only other thing in the queue is of same priority, yield back and forth
    printf("1 ");
    if(k_tsk_create(&utid[0], utask5, 150, 0x200) != RTX_OK){
        printf("ktask3 task creation failed!\n");
        k_tsk_exit();
        return;
    }
    k_tsk_yield();
    printf("3 ");
    k_tsk_yield();
    printf("5 ");
    k_tsk_yield();
    printf("7 ");
    printf("ALL TESTS DONE\n");
    k_tsk_exit();
}
#endif

#if TEST == 9
task_t utid[10];
void ktask1(void){ // check create yields
	printf("1");
	RTX_TASK_INFO info;
	k_tsk_get_info(k_tsk_get_tid(), &info);
	if (info.priv != 1){
		printf("FAILED incorrect privilege\n");
	}
	if (info.state != RUNNING){
		printf("FAILED incorrect state\n");
	}
	if (info.k_stack_size != 0x200){
		printf("FAILED incorrect k stack size\n");
	}
	if (info.u_stack_size != 0){
		printf("FAILED incorrect u stack size\n");
	}
	if (k_tsk_create(&utid[0], utask1, 200, 0x200) != RTX_OK){
		printf("FAILED to create task\n");
	}
	if (utid[0] == 0 || utid[0] == 255 || utid[0] > MAX_TASKS){
		printf("FAILED wrong tid\n");
	}
	printf("2");
	if (k_tsk_create(&utid[1], utask2, 125, 0x200) != RTX_OK){
		printf("FAILED to create task\n");
	}
	printf("3");
	if (k_tsk_create(&utid[2], utask3, 124, 0x200) != RTX_OK){
		printf("FAILED to create task\n");
	}
	printf("6");
    k_tsk_exit();
}

void ktask2(void){ // check invalid inputs
	if (k_tsk_create(&utid[0], utask10, 255, 0x200) != RTX_ERR){
		printf("FAILED");
	}
	if (k_tsk_create(&utid[0], utask10, 0, 0x200) != RTX_ERR){
		printf("FAILED");
	}
	if (k_tsk_create(&utid[0], utask10, 1, 0x100) != RTX_ERR){
		printf("FAILED");
	}
	if (k_tsk_create(NULL, utask10, 1, 0x200) != RTX_ERR){
		printf("FAILED");
	}
	if (k_tsk_create(&utid[0], NULL, 1, 0x200) != RTX_ERR){
		printf("FAILED");
	}
	if (k_tsk_create(&utid[0], utask10, 1, 0xFFFF) != RTX_ERR){
		printf("FAILED");
	}
	if (k_tsk_create(&utid[0], utask10, 1, 0xFFFF & ~0x7) != RTX_OK){
		printf("FAILED");
	}
	if (k_tsk_create(&utid[0], utask10, 1, 0x200) != RTX_OK){
		printf("FAILED");
	}
    k_tsk_exit();
}

void utask10(void){
	printf("\nTADA!\n");
	tsk_exit();
}
#endif

#if TEST == 10
#define NUM_TEST 20
task_t utids[MAX_TASKS-2]; 
task_t utid_map[MAX_TASKS];
task_t chosen_indices[NUM_TEST];
void ktask1(void){ 
    //first make sure only MAX_TASK number of tasks can be run
    for(int i = 0;i < MAX_TASKS-2;i++){
        if(k_tsk_create(&utids[i], utask1, 110, 0x200) != RTX_OK){
            printf("task creation failed on %d, ending test\n", i);
            k_tsk_exit();
            return;
        }
    }

    if(k_tsk_create(&utids[0], utask1, 110, 0x200) != RTX_ERR){
        printf("task creation succeeded when it should have failed, ending test\n");
        k_tsk_exit();
        return;
    }

    printf("1 out of 3 tests succeeded ==============\n");

    task_t kernel_tid = k_tsk_get_tid();
     //make sure that there are no TID duplicates
    for(int i = 0;i < MAX_TASKS;i++){
        utid_map[i] = 0;
    }
    utid_map[0] = 1;
    utid_map[kernel_tid] = 1;
    
    //if for loop runs without fail, then we did not perform double alloc
    //further, all TIDs should be alloced by pigeon hole :)
    for(int i = 0;i < MAX_TASKS-2;i++){
        if(utid_map[utids[i]] == 1){
            printf("FAIL, double TID alloced, ending test\n");
            k_tsk_exit();
            return;
        }
        utid_map[utids[i]] = 1;
    }
    printf("2 out of 3 tests succeeded ==============\n");


    //now allow 20 tests to run, record their TIDS, then create 20 more tests, these should be the TIDS that were deallocated
    //Do this 20 times, make sure double TID is never assigned!
    //Use "linear congruential generator" for random number generation.
    int gen_index = 1;
    int gen_mod = MAX_TASKS - 2; //large prime number
    int gen_a = 11; //multiplier for the random number generator
    int gen_c = 3; //adder

    printf("Running Test 3.... This may take a while\n");
    //50 iterations of the test
    for(int i = 0;i < 20;i++){
        //allow the 20 tests to run;
        for(int j = 0;j < 20;j++){
            gen_index = gen_a*(gen_index + gen_c) % gen_mod;
            if(utid_map[utids[gen_index]] == 1){
                utid_map[utids[gen_index]] = 0;
                chosen_indices[j] = gen_index;
                k_tsk_set_prio(utids[gen_index], 99); //allow it to run and finish
            } else {
                j--;
            }
        }

        //create 20 more tasks, add them to the available utids and ensure they were taken
        for(int j = 0;j < 20;j++){
            if(k_tsk_create(&utids[chosen_indices[j]], utask1, 110, 0x200) != RTX_OK){
                printf("task creation failed, ending test\n");
                k_tsk_exit();
                return;
            }

            if(utid_map[utids[chosen_indices[j]]] == 1){
                printf("assigned double tid, ending test \n");
                k_tsk_exit();
                return;
            } else {
                utid_map[utids[chosen_indices[j]]] = 1;
            }
        }
    }
    printf("3 out of 3 tests succeeded ==============\n");
    k_tsk_exit();
}
#endif

#if TEST == 11
task_t utid[10];\
void * ptr;

void utask2(void){
	printf("3 ");

	if (tsk_set_prio(utid[3], 3) != RTX_OK){
		printf("FAILED");
	}
//	printf("4 ");
//	tsk_yield();
	printf("4 ");
	tsk_exit();
}

void utask3(void){
	printf("5 ");
	if (mem_dealloc(ptr) != RTX_ERR){
		printf("FAILED mem2");
	}
	tsk_yield();
//	if (tsk_set_prio(tsk_get_tid(), 3) != RTX_OK){
//		printf("FAILED");
//	}
	printf("6 ");
	tsk_exit();
}

void utask4(void){
	printf("7 ");
	if (mem_dealloc(ptr) != RTX_ERR){
			printf("FAILED mem2");
		}
	if (tsk_set_prio(tsk_get_tid(), 2) != RTX_OK){
		printf("FAILED");
	}
	printf("8 ");
	tsk_exit();
}

void utask5(void){
	printf("9 ");
	ptr = mem_alloc(100);
	if (ptr == NULL) printf("FAILED mem1");
	if (tsk_set_prio(utid[2], 1) != RTX_OK){
		printf("FAILED");
	}
	tsk_yield();
	if (mem_dealloc(ptr) == RTX_ERR){
			printf("FAILED mem2");
		}
	printf("10 ");
	tsk_exit();
}

void ktask1(void){
	printf("1 ");
	if (k_tsk_create(&utid[0], utask2, 2, 0x200) != RTX_OK){
		printf("FAILED 1");
	}
	if (k_tsk_create(&utid[1], utask3, 3, 0x200) != RTX_OK){
		printf("FAILED 1");
	}
	if (k_tsk_create(&utid[2], utask4, 4, 0x200) != RTX_OK){
		printf("FAILED 1");
	}
	if (k_tsk_create(&utid[3], utask5, 5, 0x200) != RTX_OK){
		printf("FAILED 1");
	}
	printf("2 ");
	k_tsk_exit();
}
#endif

#if TEST == 12
U8 utid[10];
void ktask1(void){
    //Here, the heap should look like
    //      1
    //    0   2

    //make sure in this case, that yield works...
    //eg remove insert doesn't mess up

    //First set our current priority to 130 to yield to ktask2

    printf("SUB TEST1 START =============\n");
    printf("1 ");
    k_tsk_set_prio(1, 130);
    printf("3 ");
    k_tsk_set_prio(2, 115);

    //set own priority to lower priority
    printf("8 ");
    k_tsk_set_prio(1, 125);

    k_tsk_set_prio(1, 115); //these two should not do anything!
    k_tsk_yield();

    printf("\nSUB TEST1 FINISHED =============\n");
    k_tsk_exit();

}

void ktask2(void){
    //set prio of 1 to be higher, let it run first
    printf("2 ");
    k_tsk_set_prio(1, 120);
    printf("4 ");

    //set prio to 120 again, nothing should happen
    k_tsk_set_prio(1, 120);
    printf("5 ");

    //yield the task, again nothing should happen
    k_tsk_yield();

    //set the ktask 1 to have same prio
    //Nothing should happen
    printf("6 ");
    k_tsk_set_prio(1, 115);

    //yield, this should change to ktask1
    printf("7 ");
    k_tsk_yield();

    printf("9 ");
    k_tsk_exit();
}
#endif

#if TEST == 13
task_t utid[10];
void *ptr;
void utask1(void){
	if (mem_dealloc(g_tcbs[utid[0]].u_stack_lo) != RTX_ERR){
		printf("FAILED dealloc user");
	}
	printf("NAY\n");
	ptr = mem_alloc(0x8);
	if (ptr == NULL){
		printf("FAILED alloc user");
	}
	tsk_yield();
	if (mem_dealloc(ptr) != RTX_OK){
		printf("FAILED dealloc user");
	}
	printf("WOOT");
	tsk_exit();
}

void ktask1(void){
	if (k_tsk_create(&utid[0], utask1, 200, 0x200) != RTX_OK){
		printf("FAILED create");
	}

	if (k_mem_dealloc(g_tcbs[utid[0]].u_stack_lo) != RTX_ERR){
		printf("FAILED dealloc");
	}
	printf("YAY\n");
	if (tsk_set_prio(k_tsk_get_tid(), 200) != RTX_OK){
		printf("FAILED set prio");
	}

	if (k_mem_dealloc(ptr) != RTX_ERR){
		printf("why did u dealloc");
	}
	k_tsk_yield();
	printf("NOOT");
	k_tsk_exit();
}
#endif

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
