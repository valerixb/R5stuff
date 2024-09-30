//
// R5 demo application
// IRQs from PL
// test #3 : 
//    - IRQ from register bank
//    - IRQ from AXI GPIO
//    - IRQ from AXI timer
//
// latest rev: sept 30 2024
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "xil_io.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xil_types.h"
#include "xscugic.h"
#include "xil_util.h"
#include "xil_exception.h"
#include "xplatform_info.h"
#include "platform.h"
#include "xgpio.h"
#include "xtmrctr.h"


// ##########  local defs  ###################

#define INTC_DEVICE_ID        XPAR_SCUGIC_SINGLE_DEVICE_ID
// AXI timer has IRQ ID 121, which is XPS_FPGA0_INT_ID in xparameters_ps.h
#define INTC_TIMER_IRQ_ID     XPS_FPGA0_INT_ID
// AXI GPIO has IRQ ID 122, which is XPS_FPGA1_INT_ID in xparameters_ps.h
#define INTC_AXIGPIO_IRQ_ID   XPS_FPGA1_INT_ID
// PL regbank has IRQ ID 123, which is XPS_FPGA2_INT_ID in xparameters_ps.h
#define INTC_REGBANK_IRQ_ID   XPS_FPGA2_INT_ID
// PL regbank has base address 0x8002_0000,  which is XPAR_REGBANK_0_BASEADDR in xparameters.h
#define REGBANK               (volatile unsigned int *)(XPAR_REGBANK_0_BASEADDR)
// AXI GPIO ID is 0, even if it's not defined in xparameters.h
#define GPIO_DEVICE_ID        0
// buttons are on GPIO channel 1, LEDs on channel 2; enable IRQ for channel 1
#define GPIO_BUTTON_IRQ_MASK  XGPIO_IR_CH1_MASK
// AXI timer ID is 0, even if it's not defined in xparameters.h
#define TIMER_DEVICE_ID        0
// AXI timer has 2 timers; we only use the first one, timer#0
#define TIMER_NUMBER           0
// frequency of the timer interrupt:
#define TIMER_FREQ_HZ          1


// ##########  globals  #######################

XScuGic interruptController;
static XScuGic_Config *gicConfig;
unsigned long int irq_cntr[3];
XGpio theGpio;
static XGpio_Config *gpioConfig;
XTmrCtr theTimer;
static XTmrCtr_Config *timerConfig;


// ##########  protos  ########################

void LocalAbortHandler(void *callbackRef);
void LocalUndefinedHandler(void *callbackRef);
void RegbankISR(void *CallbackRef);
void GpioISR(void *CallbackRef);
static void TimerISR(void *callbackRef, u8 timer_num);
int SetupAXIGPIO(void);
int SetupAXItimer(void);
int SetupIRQs(void);
int CleanupIRQs(void);
int SetupSystem(void);
int CleanupSystem(void);
void SetupExceptions(void);
int main(void);

// ##########  implementation  ################

static void AssertPrint(const char8 *FilenamePtr, s32 LineNumber)
  {
  xil_printf("ASSERT: File Name: %s ", FilenamePtr);
  xil_printf("Line Number: %d\r\n", LineNumber);
  }


// -----------------------------------------------------------

void LocalAbortHandler(void *callbackRef)
  {
  // Data Abort exception handler
  printf("DATA ABORT exception\n");
  }


// -----------------------------------------------------------

void LocalUndefinedHandler(void *callbackRef)
  {
  // Undefined Instruction exception handler
  printf("UNDEFINED INSTRUCTION exception\n");
  }


// -----------------------------------------------------------

void RegbankISR(void *callbackRef)
  {
  // Regbank Interrupt Servicing Routine
  irq_cntr[2]++;
  // don't use printf in ISR! Xilinx standalone stdio is NOT thread safe
  //printf("IRQ received from REGBANK (%lu)\n", irq_cntr[2]);
  }


// -----------------------------------------------------------

void GpioISR(void *callbackRef)
  {
  XGpio *gpioPtr = (XGpio *)callbackRef;
  
  // AXI GPIO Interrupt Servicing Routine
  irq_cntr[1]++;
  // don't use printf in ISR! Xilinx standalone stdio is NOT thread safe
  //printf("IRQ received from AXI GPIO (%lu)\n", irq_cntr[1]);

  XGpio_InterruptClear(gpioPtr, GPIO_BUTTON_IRQ_MASK);
  }


// -----------------------------------------------------------

static void TimerISR(void *callbackRef, u8 timer_num)
  {
  XTmrCtr *timerPtr = (XTmrCtr *)callbackRef;
  
  // AXI timer Interrupt Servicing Routine
  irq_cntr[0]++;
  // don't use printf in ISR! Xilinx standalone stdio is NOT thread safe
  //printf("IRQ received from AXI timer (%lu)\n", ++irq_cntr[0]);
  }


// -----------------------------------------------------------

int SetupIRQs(void)
  {
  int status;
  u8  irqPriority, irqSensitivity;

  // setup IRQ system --------------------------

  gicConfig=XScuGic_LookupConfig(INTC_DEVICE_ID);
  if(NULL==gicConfig) 
    return XST_FAILURE;

  status=XScuGic_CfgInitialize(&interruptController, gicConfig, gicConfig->CpuBaseAddress);
  if (status!=XST_SUCCESS)
    return XST_FAILURE;

  status=XScuGic_SelfTest(&interruptController);
  if(status!=XST_SUCCESS)
    return XST_FAILURE;

  Xil_ExceptionInit();

  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                               (Xil_ExceptionHandler) XScuGic_InterruptHandler,
                               &interruptController);
  Xil_ExceptionEnable();

  // now register the IRQs I am interested in ----------------------------

  // Setup PL REGBANK IRQ ---------------------------------
  // register ISR
  status=XScuGic_Connect(&interruptController, INTC_REGBANK_IRQ_ID,
                         (Xil_ExceptionHandler)RegbankISR,
                         (void *)&interruptController);
  if(status!=XST_SUCCESS)
    return XST_FAILURE;
  // set priority and endge sensitivity
  XScuGic_GetPriorityTriggerType(&interruptController, INTC_REGBANK_IRQ_ID, &irqPriority, &irqSensitivity);
  //printf("REGBANK old priority=0x%02X; old sensitivity=0x%02X\n", irqPriority, irqSensitivity);
  //irqPriority = 0;           // in step of 8; 0=highest; 248=lowest
  irqSensitivity=0x03;         // rising edge
  XScuGic_SetPriorityTriggerType(&interruptController, INTC_REGBANK_IRQ_ID, irqPriority, irqSensitivity);
  // now enable the IRQ
  XScuGic_Enable(&interruptController, INTC_REGBANK_IRQ_ID);


  // Setup AXI GPIO IRQs ---------------------------------
  // register ISR
  status=XScuGic_Connect(&interruptController, INTC_AXIGPIO_IRQ_ID,
                         (Xil_ExceptionHandler)GpioISR,
                         (void *)&theGpio);
  if(status!=XST_SUCCESS)
    return XST_FAILURE;
  // set priority and endge sensitivity
  XScuGic_GetPriorityTriggerType(&interruptController, INTC_AXIGPIO_IRQ_ID, &irqPriority, &irqSensitivity);
  //printf("AXI GPIO old priority=0x%02X; old sensitivity=0x%02X\n", irqPriority, irqSensitivity);
  //irqPriority = 0;           // in step of 8; 0=highest; 248=lowest
  irqSensitivity=0x03;         // rising edge
  XScuGic_SetPriorityTriggerType(&interruptController, INTC_AXIGPIO_IRQ_ID, irqPriority, irqSensitivity);
  // enable the IRQ
  XScuGic_Enable(&interruptController, INTC_AXIGPIO_IRQ_ID);
  XGpio_InterruptEnable(&theGpio, GPIO_BUTTON_IRQ_MASK);
  XGpio_InterruptGlobalEnable(&theGpio);


  // Setup AXI timer IRQs ---------------------------------
  // register ISR into standalone AXI timer driver
  XTmrCtr_SetHandler(&theTimer, (XTmrCtr_Handler)TimerISR, (void *)&theTimer);
  // now register AXI timer driver own ISR into the GIC
  status=XScuGic_Connect(&interruptController, INTC_TIMER_IRQ_ID,
                         (Xil_ExceptionHandler)XTmrCtr_InterruptHandler,
                         (void *)&theTimer);
  if(status!=XST_SUCCESS)
    return XST_FAILURE;

  // set priority and endge sensitivity
  XScuGic_GetPriorityTriggerType(&interruptController, INTC_TIMER_IRQ_ID, &irqPriority, &irqSensitivity);
  //printf("AXI timer old priority=0x%02X; old sensitivity=0x%02X\n", irqPriority, irqSensitivity);
  //irqPriority = 0;           // in step of 8; 0=highest; 248=lowest
  irqSensitivity=0x03;         // rising edge
  XScuGic_SetPriorityTriggerType(&interruptController, INTC_TIMER_IRQ_ID, irqPriority, irqSensitivity);
  // enable the IRQ
  XScuGic_Enable(&interruptController, INTC_TIMER_IRQ_ID);
  // start timer
  XTmrCtr_Start(&theTimer, TIMER_NUMBER);


  return XST_SUCCESS;
  }


// -----------------------------------------------------------

int CleanupIRQs(void)
  {
  // Cleanup PL REGBANK IRQ ---------------------------------
  XScuGic_Disable(&interruptController, INTC_REGBANK_IRQ_ID);
  XScuGic_Disconnect(&interruptController, INTC_REGBANK_IRQ_ID);

  // Cleanup AXI GPIO IRQs ---------------------------------
  XGpio_InterruptGlobalDisable(&theGpio);
  XGpio_InterruptDisable(&theGpio, GPIO_BUTTON_IRQ_MASK);
  XScuGic_Disable(&interruptController, INTC_AXIGPIO_IRQ_ID);
  XScuGic_Disconnect(&interruptController, INTC_AXIGPIO_IRQ_ID);

  // Cleanup AXI timer IRQs ---------------------------------
  XTmrCtr_Stop(&theTimer, TIMER_NUMBER);
  XScuGic_Disable(&interruptController, INTC_TIMER_IRQ_ID);
  XScuGic_Disconnect(&interruptController, INTC_TIMER_IRQ_ID);

  return XST_SUCCESS;
  }


// -----------------------------------------------------------

int SetupAXIGPIO(void)
  {
  int status;

  gpioConfig=XGpio_LookupConfig(GPIO_DEVICE_ID);
  if(NULL==gpioConfig) 
    return XST_FAILURE;

  status=XGpio_CfgInitialize(&theGpio, gpioConfig, gpioConfig->BaseAddress);
  if (status!=XST_SUCCESS)
    return XST_FAILURE;

  status=XGpio_SelfTest(&theGpio);
  if(status!=XST_SUCCESS)
    return XST_FAILURE;

  return XST_SUCCESS;
  }


// -----------------------------------------------------------

int SetupAXItimer(void)
  {
  int status;

  // XTmrCtr_Initialize calls XTmrCtr_LookupConfig, XTmrCtr_CfgInitialize and XTmrCtr_InitHw
  // I prefer to call them explicitly to get a copy of the configuration structure, 
  // which contains the value of the AXI clock, needed to calculate the timer period.
  timerConfig=XTmrCtr_LookupConfig(TIMER_DEVICE_ID);
  if(NULL==timerConfig) 
    return XST_FAILURE;
  
  XTmrCtr_CfgInitialize(&theTimer, timerConfig, timerConfig->BaseAddress);

  status=XTmrCtr_InitHw(&theTimer);
  if(status!=XST_SUCCESS)
    return XST_FAILURE;

  // I don't use XTmrCtr_Initialize; see previous remark
  //status=XTmrCtr_Initialize(&theTimer,TIMER_DEVICE_ID);
  //if(status!=XST_SUCCESS)
  //  return XST_FAILURE;

  status=XTmrCtr_SelfTest(&theTimer, TIMER_NUMBER);
  if(status!=XST_SUCCESS)
    return XST_FAILURE;

  // set timer in generate mode, auto reload, count down (for easier setup of timer period)
  XTmrCtr_SetOptions(&theTimer, TIMER_NUMBER, 
          XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION);

  // set timer period
  XTmrCtr_SetResetValue(&theTimer, TIMER_NUMBER,
          (u32)(timerConfig->SysClockFreqHz/TIMER_FREQ_HZ));

  return XST_SUCCESS;
  }


// -----------------------------------------------------------

void SetupExceptions(void)
  {
  // ASSERT callback for debug, in case of an exception
  Xil_AssertSetCallback(AssertPrint);
  // register some exception call backs
  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_DATA_ABORT_INT, LocalAbortHandler, NULL);
  //Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_UNDEFINED_INT, LocalUndefinedHandler, NULL);
  }


// -----------------------------------------------------------

int SetupSystem(void)
  {
  int status;

  SetupExceptions();

  status = SetupAXIGPIO();
  if(status!=XST_SUCCESS)
    return XST_FAILURE;

  status = SetupAXItimer();
  if(status!=XST_SUCCESS)
    return XST_FAILURE;

  status = SetupIRQs();
  if(status!=XST_SUCCESS)
    return XST_FAILURE;

  return XST_SUCCESS;
  }

// -----------------------------------------------------------

int CleanupSystem(void)
  {
  int status;

  status = CleanupIRQs();
  if(status!=XST_SUCCESS)
    return XST_FAILURE;

  return XST_SUCCESS;
  }

// -----------------------------------------------------------

int main()
  {
  unsigned int thereg, theval;
  int          status;

  init_platform();

  printf("\nR5 test application #4 - IRQ test #3 (regbank + gpio + timer)\n");

  // remove buffering from stdin and stdout
  setvbuf (stdout, NULL, _IONBF, 0);
  setvbuf (stdin, NULL, _IONBF, 0);
  
  // init number of IRQ served
  irq_cntr[0]=0;
  irq_cntr[1]=0;
  irq_cntr[2]=0;

  status = SetupSystem();
  if(status!=XST_SUCCESS)
    {
    printf("ERROR Setting up IRQ System - aborting\n");
    return status;
    }


  while(1)
    {
    printf("\nTimer   IRQs: %lu\n",irq_cntr[0]);
    printf(  "GPIO    IRQs: %lu\n",irq_cntr[1]);
    printf(  "RegBank IRQs: %lu\n",irq_cntr[2]);
    printf("\nInput register number to read (0..15)\n");
    status=scanf("%u",&thereg);
    if(status>0)
      {
      if(thereg>15) break;
      theval=*(REGBANK+thereg);
      printf("Reg #%02u is 0x%08X\n", thereg, theval);
      }
    }

  printf("\nExiting\n");

  status = CleanupSystem();
  if(status!=XST_SUCCESS)
    {
    printf("ERROR Cleaning up IRQ System\n");
    // continue anyway, as we are shutting down
    //return status;
    }

  cleanup_platform();
  return 0;
  }
