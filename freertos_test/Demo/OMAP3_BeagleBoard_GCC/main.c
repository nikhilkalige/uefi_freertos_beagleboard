/*
    FreeRTOS V7.0.1 - Copyright (C) 2011 Real Time Engineers Ltd.

    ***************************************************************************
    *                                                                         *
    * If you are:                                                             *
    *                                                                         *
    *    + New to FreeRTOS,                                                   *
    *    + Wanting to learn FreeRTOS or multitasking in general quickly       *
    *    + Looking for basic training,                                        *
    *    + Wanting to improve your FreeRTOS skills and productivity           *
    *                                                                         *
    * then take a look at the FreeRTOS books - available as PDF or paperback  *
    *                                                                         *
    *        "Using the FreeRTOS Real Time Kernel - a Practical Guide"        *
    *                  http://www.FreeRTOS.org/Documentation                  *
    *                                                                         *
    * A pdf reference manual is also available.  Both are usually delivered   *
    * to your inbox within 20 minutes to two hours when purchased between 8am *
    * and 8pm GMT (although please allow up to 24 hours in case of            *
    * exceptional circumstances).  Thank you for your support!                *
    *                                                                         *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

/*
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 *
 * Main.c also creates a task called "Check".  This only executes every three
 * seconds but has the highest priority so is guaranteed to get processor time.
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is
 * incremented each time the task successfully completes its function.  Should
 * any error occur within such a task the count is permanently halted.  The
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 * To check the operation of the memory allocator the check task also
 * dynamically creates a task before delaying, and deletes it again when it
 * wakes.  If memory cannot be allocated for the new task the call to xTaskCreate
 * will fail and an error is signalled.  The dynamically created task itself
 * allocates and frees memory just to give the allocator a bit more exercise.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <bbconsole.h>
/*-----------------------------------------------------------*/

#include  <Uefi.h>

EFI_GUID rtos_var = {
    0xAA, 0xBB, 0xCC, {
        0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C
    }
};

/* Priorities for the demo application tasks. */
#define mainLED_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )
#define mainCOM_TEST_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define mainQUEUE_POLL_PRIORITY		( tskIDLE_PRIORITY + 0 )
#define mainCHECK_TASK_PRIORITY		( tskIDLE_PRIORITY + 4 )
#define mainSEM_TEST_PRIORITY		( tskIDLE_PRIORITY + 0 )
#define mainBLOCK_Q_PRIORITY		( tskIDLE_PRIORITY + 2 )

/* The rate at which the on board LED will toggle when there is/is not an
error. */
#define mainNO_ERROR_FLASH_PERIOD	( ( portTickType ) 3000 / portTICK_RATE_MS  )
#define mainERROR_FLASH_PERIOD		( ( portTickType ) 500 / portTICK_RATE_MS  )
#define mainON_BOARD_LED_BIT		PIN21
/* Constants used by the vMemCheckTask() task. */
#define mainCOUNT_INITIAL_VALUE		( ( unsigned long ) 0 )
#define mainNO_TASK					( 0 )

/* The size of the memory blocks allocated by the vMemCheckTask() task. */
#define mainMEM_CHECK_SIZE_1		( ( size_t ) 51 )
#define mainMEM_CHECK_SIZE_2		( ( size_t ) 52 )
#define mainMEM_CHECK_SIZE_3		( ( size_t ) 151 )

/* This is for Serial Task */
#define mainCOM_TEST_BAUD_RATE		( ( unsigned long ) 115200 )
#define mainCOM_TEST_LED			( 1 )
/*-------------------------------------------------------------------------------------------*/



  /*-------------------------------------------------------------------------------------------*/

/*
 * The Beagleboard has 2 LEDS available on the GPIO module
 * I will use LED0 to express errors on tasks
 */

//void prvToggleOnBoardLED( void );

/*
 * Configure the processor of use with the BeagleBoard
 * Currently I assume that U-boot has done all the work for us
 */
static void prvSetupHardware( void );

/*
 * Checks that all the demo application tasks are still executing without error
 * - as described at the top of the file.
 */
//static long prvCheckOtherTasksAreStillRunning( unsigned long ulMemCheckTaskCount );

/*
 * The task that executes at the highest priority and calls
 * prvCheckOtherTasksAreStillRunning(). See the description at the top
 * of the file
 */
//static void vErrorChecks ( void *pvParameters );

/*
 * Dynamically created and deleteted during each cycle of the vErrorChecks()
 * task. This is done to check the operation of the memory allocator.
 * See the top of vErrorChecks for more details
 */
//static void vMemCheckTask( void *pvParameters );

void HelloWorld1(void * pvParameters);
void HelloWorld2(void * pvParameters);
void FlashLED1(void * pvParameters);
void FlashLED2(void * pvParameters);

/*-----------------------------------------------------------*/


EFI_RUNTIME_SERVICES  *uefi_services;
// char var_name[] = "FREERTOS";
uint16_t var_name[] = {'F', 'R', 'E', 'E', 'R', 'T', 'O', 'S', 0};

/*
 * Starts all the other tasks, then starts the scheduler.
 */
int main(void *x)
{
    EFI_STATUS Status;
    uint16_t temp;
    uint32_t len;
    uefi_services = (EFI_RUNTIME_SERVICES*)x;
    xTaskHandle hdl = 0;
    int c = 'p';
    len = sizeof(uint16_t);

    temp = 80;
    Status = uefi_services->GetVariable((CHAR16*)var_name, &rtos_var, NULL, (UINTN*)&len, (UINTN*)&temp);
    printf("\nGetvariable value:%d\n", temp);


    /* Setup the hardware for use with the Beableboard. */
    prvSetupHardware();
    memdump_32(0x4020ffc0, 64);


    if (EFI_ERROR (Status)) {
        temp = 32;
    }

	printf("\nIn Main, please press a key\n");
	c = getc();
	printf("Your input %c\n", c);

	xTaskCreate(HelloWorld1, ( signed char * ) "HelloWorld1",
				configMINIMAL_STACK_SIZE, (void *) NULL, 1, &hdl);

	if(hdl == 0)
	{
		printf("Hello world 1 creation failed");
	}

	xTaskCreate(HelloWorld2, ( signed char * ) "HelloWorld2",
				configMINIMAL_STACK_SIZE, (void *) NULL, 1, &hdl);

	if(hdl == 0)
	{
		printf("Hello world 2 creation failed");
	}

	xTaskCreate(FlashLED1, ( signed char * ) "FlashLED1",
				configMINIMAL_STACK_SIZE, (void *) NULL, 1, &hdl);

	if(hdl == 0)
	{
		printf("Flash LED 1 creation failed");
	}

	xTaskCreate(FlashLED2, ( signed char * ) "FlashLED2",
				configMINIMAL_STACK_SIZE, (void *) NULL, 1, &hdl);

	if(hdl == 0)
	{
		printf("Flash LED 2 creation failed");
	}

	/* Now all the tasks have been stared - start the scheduler.
	 * NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	 * Te processor MUST be in supervisor mode when vTaskStartScheduler is called.
	 * The demo applications included in the the FreeRTOS.og download swith to supervisor
	 * mode prior to main being called. If you are not using one of these demo application
	 * projects then ensure Supervisor mode is used here */
	/* Should never reach here! */

	printf("Start FreeRTOS ...\n");

	vTaskStartScheduler();
	return 0;
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{

	/* Initialize GPIOs */
	/* GPIO5: 31,30,29,28,22,21,15,14,13,12
	 * GPIO6: 23,10,08,02,01 */
	(*(REG32(GPIO5_BASE+GPIO_OE))) = ~(PIN31|PIN30|PIN29|PIN28|PIN22|PIN21|PIN15|PIN14|PIN13|PIN12);
	(*(REG32(GPIO6_BASE+GPIO_OE))) = ~(PIN23|PIN10|PIN8|PIN2|PIN1);

	/* Switch off the leds */
	(*(REG32(GPIO5_BASE+GPIO_CLEARDATAOUT))) = PIN22|PIN21;
}


/*---------------------------------------------------------*/
