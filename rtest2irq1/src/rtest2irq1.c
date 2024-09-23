//
// R5 demo application
// IRQ from PS - test #1
//
// latest rev: sept 23 2024
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
#define INTC_TIMER_IRQ_ID	   121
#define INTC_AXIGPIO_IRQ_ID	 122
#define INTC_REGBANK_IRQ_ID	 123
#define REGBANK              (volatile unsigned int *)(XPAR_REGBANK_0_BASEADDR)


// ##########  globals  #######################

XScuGic interruptController;
static XScuGic_Config *gicConfig;


// ##########  protos  ########################

void RegbankIRS(void *CallbackRef);
int SetupIRQs(void);
int main(void);

// ##########  implementation  ################

void RegbankIRS(void *CallbackRef)
  {
  // Regbank Interrupt Servicing Routine
  printf("IRQ received from REGBANK\n");
  }

// -----------------------------------------------------------

int SetupIRQs(void)
  {
  int status;

  // setup IRQ system

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

  // now register the IRQs I am interested in

  // register ISR for REGBANK IRQ
  status=XScuGic_Connect(&interruptController, INTC_REGBANK_IRQ_ID,
                         (Xil_ExceptionHandler)RegbankIRS,
                         (void *)&interruptController);
  if(status!=XST_SUCCESS)
    return XST_FAILURE;

  XScuGic_Enable(&interruptController, INTC_REGBANK_IRQ_ID);

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
  
  status = SetupIRQs();
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
