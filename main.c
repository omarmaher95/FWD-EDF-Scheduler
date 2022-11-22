/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
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
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"
#include "queue.h"
/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )
/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/
#define TASKS_NUMBER        5		

/* Tasks Periods */
#define BUTTON_1_MONITOR         50
#define BUTTON_2_MONITOR         50
#define PERIODIC_TRANSMITTER     100
#define UART_RECEIVER            20
#define LOAD_1_SIMULATION        10
#define LOAD_2_SIMULATION        100

/* Tasks Handlers */
TaskHandle_t Button1_Handler      = NULL;
TaskHandle_t Button2_Handler      = NULL;
TaskHandle_t Load1_Handler        = NULL;
TaskHandle_t Load2_Handler        = NULL;
TaskHandle_t Transmitter_Handler  = NULL;
TaskHandle_t Uart_Handler         = NULL;

/*Queue Handlers*/
QueueHandle_t xQueue = NULL;

/*GPIOS*/
#define APP_IDLE_HOOK      PIN0
#define BUTTON_1           PIN8
#define BUTTON_2           PIN9
#define BUTTON_1_ANALYZER  PIN3
#define BUTTON_2_ANALYZER  PIN4
#define TRANSMIT_ANALYZER  PIN5
#define UART_ANALYZER      PIN6
#define LOAD_1_ANALYZER    PIN7
#define LOAD_2_ANALYZER    PIN1
#define APP_Tick_HOOK      PIN2

#define APP_IDLE_HOOK_AN      (port0 &0x00010000)>>16
#define BUTTON_1_             (port0 &0x00800000)>>24
#define BUTTON_2_             (port0 &0x01000000)>>25
#define BUTTON_1_AN           (port0 &0x00080000)>>19
#define BUTTON_2_AN           (port0 &0x00100000)>>20
#define TRANSMIT_AN           (port0 &0x00200000)>>21
#define UART_AN               (port0 &0x00400000)>>22
#define LOAD_1_AN             (port0 &0x00800000)>>23
#define LOAD_2_AN             (port0 &0x00020000)>>17
#define APP_TICK_HOOK_AN      (port0 &0x00040000)>>18

typedef struct{
   char MessageID;
   char Data[20];
} QMessage;

void Button_1_Monitor(void * pvParameters){
    TickType_t xLastWakeTime = xTaskGetTickCount();
	
		QMessage risingEdge  = {'1', "\nRising Edge\n"};
		QMessage fallingEdge = {'1', "\nFalling Edge\n"};
		QMessage noChange    = {'1', "\nNo Change\n"};
		
		pinState_t oldButtonState = PIN_IS_LOW;
		pinState_t newButtonState;

    for(;;){
			GPIO_write(PORT_0, BUTTON_1_ANALYZER, PIN_IS_HIGH);
			newButtonState = GPIO_read(PORT_0, BUTTON_1);
			if(newButtonState == PIN_IS_HIGH && oldButtonState == PIN_IS_LOW){
				xQueueSend(xQueue,(void *) &risingEdge, 10);
			}else if(newButtonState == PIN_IS_LOW && oldButtonState == PIN_IS_HIGH){
				xQueueSend(xQueue,(void *) &fallingEdge, 10);
			}else{
				xQueueSend(xQueue,(void *) &noChange, 10);				
			}
			oldButtonState = newButtonState;
			GPIO_write(PORT_0, BUTTON_1_ANALYZER, PIN_IS_LOW);		
			vTaskDelayUntil( &xLastWakeTime, BUTTON_1_MONITOR);
    }//10us
}


void Button_2_Monitor(void * pvParameters){
    TickType_t xLastWakeTime = xTaskGetTickCount();
	
		QMessage risingEdge  = {'2', "\nRising Edge\n"};
		QMessage fallingEdge = {'2', "\nFalling Edge\n"};
		QMessage noChange    = {'2', "\nNo Change\n"};
		
		pinState_t oldButtonState = PIN_IS_LOW;
		pinState_t newButtonState;

    for(;;){
			GPIO_write(PORT_0, BUTTON_2_ANALYZER, PIN_IS_HIGH);
			newButtonState = GPIO_read(PORT_0, BUTTON_2);
			if(newButtonState == PIN_IS_HIGH && oldButtonState == PIN_IS_LOW){
				xQueueSend(xQueue,(void *) &risingEdge, 10);
			}else if(newButtonState == PIN_IS_LOW && oldButtonState == PIN_IS_HIGH){
				xQueueSend(xQueue,(void *) &fallingEdge, 10);
			}else{
				xQueueSend(xQueue,(void *) &noChange, 10);				
			}
			oldButtonState = newButtonState;
			GPIO_write(PORT_0, BUTTON_2_ANALYZER, PIN_IS_LOW);			
			vTaskDelayUntil(&xLastWakeTime, BUTTON_2_MONITOR);
    }
}//10us

void Periodic_Transmitter(void * pvParameters){				
	  TickType_t xLastWakeTime = xTaskGetTickCount();
		QMessage periodicTransmitter = {'3', "\nPeroidic Message\n"};

    for(;;){
			GPIO_write(PORT_0, TRANSMIT_ANALYZER, PIN_IS_HIGH);
			xQueueSend(xQueue,(void *) &periodicTransmitter, 10);
			GPIO_write(PORT_0, TRANSMIT_ANALYZER, PIN_IS_LOW);			
			vTaskDelayUntil(&xLastWakeTime, PERIODIC_TRANSMITTER);
    }		
}//6us

void Uart_Receiver(void * pvParameters){
		QMessage uartReceived;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for(;;){
			GPIO_write(PORT_0, UART_ANALYZER, PIN_IS_HIGH);
			xQueueReceive(xQueue, &uartReceived, portMAX_DELAY);
			switch(uartReceived.MessageID){
				case '1':
					vSerialPutString((signed char *)"\nButton 1\n", 20);
					break;
				case '2':
					vSerialPutString((signed char *)"\nButton 2\n", 20);
					break;
				case '3':
					vSerialPutString((signed char *)"\nPeriodic Message\n", 20);					
					break;
			}
			vSerialPutString((signed char *)uartReceived.Data, 20);
			GPIO_write(PORT_0, UART_ANALYZER, PIN_IS_LOW);			
			vTaskDelayUntil(&xLastWakeTime, UART_RECEIVER);
    }
}

void Load_1_Simulation(void * pvParameters){	
		int i =0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for(;;){
 			GPIO_write(PORT_0, LOAD_1_ANALYZER, PIN_IS_HIGH);
			for (i=0; i<40000; i++){
			}
			GPIO_write(PORT_0, LOAD_1_ANALYZER, PIN_IS_LOW);
			vTaskDelayUntil( &xLastWakeTime, LOAD_1_SIMULATION );
    }
}//5.3ms


void Load_2_Simulation(void * pvParameters){
		int i =0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for(;;){
			GPIO_write(PORT_0, LOAD_2_ANALYZER, PIN_IS_HIGH);
			for (i=0; i<100000; i++){				
			}
			GPIO_write(PORT_0, LOAD_2_ANALYZER, PIN_IS_LOW);
			vTaskDelayUntil(&xLastWakeTime, LOAD_2_SIMULATION);
    }
}


int main( void )
{	
	
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	
	xQueue = xQueueCreate(TASKS_NUMBER, sizeof(QMessage));
	
	xTaskPeriodicCreate( Button_1_Monitor, "BUTTON 1 MONITOR", configMINIMAL_STACK_SIZE, NULL, 1, NULL, BUTTON_1_MONITOR);
	xTaskPeriodicCreate( Button_2_Monitor, "BUTTON 2 MONITOR", configMINIMAL_STACK_SIZE, NULL, 1, NULL, BUTTON_2_MONITOR);
	xTaskPeriodicCreate( Load_1_Simulation, "LOAD 1 SIMULATION", configMINIMAL_STACK_SIZE, NULL, 1, NULL, LOAD_1_SIMULATION);
	xTaskPeriodicCreate( Load_2_Simulation, "LOAD 2 SIMULATION", configMINIMAL_STACK_SIZE, NULL, 1, NULL, LOAD_2_SIMULATION);
	xTaskPeriodicCreate( Periodic_Transmitter, "PERIODIC TRANSMITTER", configMINIMAL_STACK_SIZE, NULL, 1, NULL, PERIODIC_TRANSMITTER);
	xTaskPeriodicCreate( Uart_Receiver, "UART RECEIVER", configMINIMAL_STACK_SIZE, NULL, 1, NULL, UART_RECEIVER);

    /* Create Tasks here */


	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/

