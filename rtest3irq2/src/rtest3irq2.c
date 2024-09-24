//
// R5 demo application
// IRQs from PL
// test #2 : 
//    - IRQ from register bank
//    - IRQ from AXI GPIO
//
// latest rev: sept 24 2024
//

#include <stdio.h>
#include <stdlib.h>
#include "xil_io.h"
#include "xil_exception.h"
#include "xparameters.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xil_types.h"
#include "xscugic.h"
#include "xil_util.h"
#include "xplatform_info.h"
#include "platform.h"

// ##########  local defs  ###################

#define INTC_DEVICE_ID       XPAR_SCUGIC_SINGLE_DEVICE_ID
// AXI timer has IRQ ID 121, which is XPS_FPGA0_INT_ID in xparameters_ps.h
#define INTC_TIMER_IRQ_ID	   XPS_FPGA0_INT_ID
// AXI GPIO has IRQ ID 122, which is XPS_FPGA1_INT_ID in xparameters_ps.h
#define INTC_AXIGPIO_IRQ_ID	 XPS_FPGA1_INT_ID
// PL regbank has IRQ ID 123, which is XPS_FPGA2_INT_ID in xparameters_ps.h
#define INTC_REGBANK_IRQ_ID	 XPS_FPGA2_INT_ID
// PL regbank has base address 0x8002_0000,  which is XPAR_REGBANK_0_BASEADDR in xparameters.h
#define REGBANK              (volatile unsigned int *)(XPAR_REGBANK_0_BASEADDR)


// ##########  globals  #######################

XScuGic interruptController;
static XScuGic_Config *gicConfig;
unsigned long int irq_cntr[3];


// ##########  protos  ########################

void RegbankIRS(void *CallbackRef);
int SetupAXIGPIO(void);
int SetupIRQs(void);
int SetupSystem(void);
int main(void);

// ##########  implementation  ################

static void AssertPrint(const char8 *FilenamePtr, s32 LineNumber)
  {
  xil_printf("ASSERT: File Name: %s ", FilenamePtr);
  xil_printf("Line Number: %d\r\n", LineNumber);
  }


// -----------------------------------------------------------

void RegbankIRS(void *CallbackRef)
  {
  // Regbank Interrupt Servicing Routine
  printf("IRQ received from REGBANK (%lu)\n", ++irq_cntr[2]);
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

  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                               (Xil_ExceptionHandler) XScuGic_InterruptHandler,
                               &interruptController);
  Xil_ExceptionEnable();

  // now register the IRQs I am interested in ----------------------------

  // register ISR for REGBANK IRQ ---------------------------------
  status=XScuGic_Connect(&interruptController, INTC_REGBANK_IRQ_ID,
                         (Xil_ExceptionHandler)RegbankIRS,
                         (void *)&interruptController);
  if(status!=XST_SUCCESS)
    return XST_FAILURE;
  // set priority and endge sensitivity
  XScuGic_GetPriorityTriggerType(&interruptController, INTC_REGBANK_IRQ_ID, &irqPriority, &irqSensitivity);
  //irqPriority = 0;           // in step of 8; 0=highest; 248=lowest
  irqSensitivity=0x03;         // rising edge
  XScuGic_SetPriorityTriggerType(&interruptController, INTC_REGBANK_IRQ_ID, irqPriority, irqSensitivity);

  // now enable the IRQ
  XScuGic_Enable(&interruptController, INTC_REGBANK_IRQ_ID);

  return XST_SUCCESS;
  }


// -----------------------------------------------------------

int SetupAXIGPIO(void)
  {
  int status;

  }


// -----------------------------------------------------------

int SetupSystem(void)
  {
  int status;

  status = SetupAXIGPIO();
  if(status!=XST_SUCCESS)
    return XST_FAILURE;

  status = SetupIRQs();
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

  printf("R5 test application #2 - IRQ test #1\n");

  setvbuf (stdout, NULL, _IONBF, 0);
  setvbuf (stdin, NULL, _IONBF, 0);
  
  // ASSERT callback for debug, in case of an exception
  Xil_AssertSetCallback(AssertPrint);
  
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
    printf("\nInput register number to read (0..15)\n");
    scanf("%u",&thereg);
    if(thereg>15) break;
    theval=*(REGBANK+thereg);
    printf("Reg #%02u is 0x%08X\n", thereg, theval);
    }


  cleanup_platform();
  return 0;
  }
