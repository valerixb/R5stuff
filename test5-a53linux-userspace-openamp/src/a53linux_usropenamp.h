//
// R5 demo application
// IRQs from PL and 
// interproc communications with A53/linux
//
// this is the linux/A53 side
//

#ifndef A53LINUX_USROPENAMP_H_
#define A53LINUX_USROPENAMP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openamp/open_amp.h>
#include <metal/alloc.h>
#include "platform.h"
#include "zynqmp_linux_r5_proc.h"

// ##########  local defs  ###################

#define LPRINTF(format, ...) printf(format, ##__VA_ARGS__)
#define LPERROR(format, ...) LPRINTF("ERROR: " format, ##__VA_ARGS__)

//---------- openamp stuff -------------------------
#define RPMSG_SERVICE_NAME         "rpmsg-uopenamp-loop-params"

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
static void rpmsg_name_service_bind_cb(struct rpmsg_device *rdev,
    const char *name, uint32_t dest);
int SetupSystem(void **platformp);
int CleanupSystem(void *platform);
static struct remoteproc *SetupRpmsg(int proc_index, int rsc_index);
int main(int argc, char *argv[]);


#endif
