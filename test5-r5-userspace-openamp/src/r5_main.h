//
// R5 demo application
// IRQs from PL and 
// interproc communications with A53/linux
//
// this is the R5 side
//

#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <openamp/open_amp.h>
#include <metal/alloc.h>
#include "common.h"
#include "platform.h"
#include "rproc.h"
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
#include <openamp/version.h>
#include <metal/version.h>
#include "rsc_table.h"

//---------- openamp stuff  ------------------------
#define IPI_DEV_NAME         "poll_dev"
#define IPI_BUS_NAME         "generic"
#define XPAR_XIPIPSU_0_BASE_ADDRESS 0xff310000
#define XPAR_XIPIPSU_0_INT_ID 65

// Interrupt vectors
#define IPI_IRQ_VECT_ID     XPAR_XIPIPSU_0_INT_ID
#define POLL_BASE_ADDR      XPAR_XIPIPSU_0_BASE_ADDRESS
#define IPI_CHN_BITMASK     0x01000000


//---------- IRQ stuff -----------------------------
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
// #defines for IRQ counter
#define TIMER_IRQ_CNTR     0
#define GPIO_IRQ_CNTR      1
#define REGBANK_IRQ_CNTR   2
#define IPI_CNTR           3


// ##########  types  #######################
typedef struct
  {
  float param1;
  int param2;
  } LOOP_PARAM_MSG_TYPE;


// ##########  extern globals  ################


// ##########  protos  ########################

static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, 
                            uint32_t src, void *priv);
static void rpmsg_service_unbind(struct rpmsg_endpoint *ept);                            
void LocalAbortHandler(void *callbackRef);
void LocalUndefinedHandler(void *callbackRef);
void RegbankISR(void *CallbackRef);
void GpioISR(void *CallbackRef);
static void TimerISR(void *callbackRef, u8 timer_num);
void FiqHandler(void *cb) __attribute__((section(".tcmb_text")));
int SetupAXIGPIO(void);
int SetupAXItimer(void);
int SetupIRQs(void);
int CleanupIRQs(void);
int SetupSystem(void **platformp);
int CleanupSystem(void *platform);
void SetupExceptions(void);
static struct remoteproc *SetupRpmsg(int proc_index, int rsc_index);
int main(void);




#endif
