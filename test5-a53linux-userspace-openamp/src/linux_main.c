//
// R5 demo application
// IRQs from PL and 
// interproc communications with A53/linux
//
// this is the linux/A53 side
//
// features:
//    - IRQ from register bank
//    - IRQ from AXI GPIO
//    - IRQ from AXI timer
//    - readout register in register bank
//    - get parameters from linux
//
// latest rev: nov 5 2024
//

#include "linux_main.h"

// ##########  globals  #######################

void *platform;
// openamp
static struct rpmsg_endpoint lept;
static LOOP_PARAM_MSG_TYPE gLoopParameters;
static LOOP_PARAM_MSG_TYPE *gMsgPtr;
struct rpmsg_device *rpdev;
static struct remoteproc rproc_inst;
static int ept_deleted = 0;

struct remoteproc_priv rproc_priv = 
  {
  .shm_name = SHM_DEV_NAME,
  .shm_bus_name = DEV_BUS_NAME,
  .ipi_name = IPI_DEV_NAME,
  .ipi_bus_name = DEV_BUS_NAME,
  .ipi_chn_mask = IPI_CHN_BITMASK,
  };



// ------------ RPMSG endpoint callbacks ----------

// I don't expect any messages from R5
static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
  {
  (void)ept;    // avoid warning on unused parameter
  (void)data;   // avoid warning on unused parameter
  (void)len;    // avoid warning on unused parameter
  (void)src;    // avoid warning on unused parameter
  (void)priv;   // avoid warning on unused parameter

  return RPMSG_SUCCESS;
  }

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
  {
  (void)ept;
  rpmsg_destroy_ept(&lept);
  LPRINTF("rpmsg service is destroyed\n");
  ept_deleted = 1;
  }

static void rpmsg_name_service_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
  {
  LPRINTF("new endpoint notification is received.\n");
  if (strcmp(name, RPMSG_SERVICE_NAME))
    LPERROR("Unexpected name service %s.\n", name);
  else
    (void)rpmsg_create_ept(&lept, rdev, 
                           RPMSG_SERVICE_NAME, RPMSG_ADDR_ANY, dest, 
                           rpmsg_endpoint_cb, rpmsg_service_unbind);
  }


// -----------------------------------------------------------

static struct remoteproc *SetupRpmsg(int proc_index, int rsc_index)
  {
  int status;
  void *rsc_table;
  int rsc_size;
  metal_phys_addr_t pa;

  (void)proc_index;    // avoid warning on unused parameter
  (void)rsc_index;     // avoid warning on unused parameter
  rsc_size = RSC_MEM_SIZE;
  // init remoteproc instance
  if(!remoteproc_init(&rproc_inst, &rproc_ops, &rproc_priv))
    return NULL;
  LPRINTF("Remoteproc successfully initialized\n");
  // mmap resource table
  pa = RSC_MEM_PA;
  LPRINTF("Calling mmap resource table.\n");
  rsc_table = remoteproc_mmap(&rproc_inst, &pa, NULL, rsc_size, 0, NULL);
  if(!rsc_table)
    {
    LPRINTF("ERROR: Failed to mmap resource table.\n");
    return NULL;
    }
  LPRINTF("Resource table successfully mmaped\n");
  // parse resource table to remoteproc
  status = remoteproc_set_rsc_table(&rproc_inst, rsc_table, rsc_size);
  if(status!=0) 
    {
    LPRINTF("Failed to initialize remoteproc\n");
    remoteproc_remove(&rproc_inst);
    return NULL;
    }
  LPRINTF("Successfully set resource table to remoteproc\n");

  return &rproc_inst;
  }


// -----------------------------------------------------------

int SetupSystem(void **platformp)
  {
  int status, max_size;
  struct remoteproc *rproc;
  struct metal_init_params metal_param = METAL_INIT_DEFAULTS;

  metal_param.log_level = METAL_LOG_INFO;

  if(!platformp)
    {
    LPRINTF("NULL platform pointer -");
    return -EINVAL;
    }

  metal_init(&metal_param);
  
  rproc = SetupRpmsg(0,0);
  if(!rproc)
    return -1;
  
  *platformp = rproc;

  rpdev = create_rpmsg_vdev(rproc, 0, VIRTIO_DEV_DRIVER, NULL, rpmsg_name_service_bind_cb);
  if(!rpdev)
    {
    LPERROR("Failed to create rpmsg virtio device.\n");
    return -1;
    }
  
  LPRINTF("Allocating msg buffer\n");
  max_size = rpmsg_virtio_get_buffer_size(rpdev);
  if(max_size < sizeof(LOOP_PARAM_MSG_TYPE))
    {
    LPERROR("too small buffer size.\r\n");
    return -1;
    }

  gMsgPtr = (LOOP_PARAM_MSG_TYPE *)metal_allocate_memory(sizeof(LOOP_PARAM_MSG_TYPE));

  if(!gMsgPtr)
    {
    LPERROR("msg buffer allocation failed.\n");
    return -1;
    }

  LPRINTF("Try to create rpmsg endpoint.\n");
  status = rpmsg_create_ept(&lept, rpdev, RPMSG_SERVICE_NAME,
                            RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
                            rpmsg_endpoint_cb,
                            rpmsg_service_unbind);
  if(status)
    {
    LPERROR("Failed to create endpoint.\n");
    metal_free_memory(gMsgPtr);
    return -1;
    }
  LPRINTF("Successfully created rpmsg endpoint.\n");

  LPRINTF("Waiting for remote answer...\n");
	while(!is_rpmsg_ept_ready(&lept))
		platform_poll(platform);

	LPRINTF("RPMSG endpoint is binded with remote.\r\n");
  return 0;
  }


// -----------------------------------------------------------

int CleanupSystem(void *platform)
  {
  struct remoteproc *rproc = platform;

  rpmsg_destroy_ept(&lept);
  metal_free_memory(gMsgPtr);
  release_rpmsg_vdev(rpdev, platform);
  if(rproc)
    remoteproc_remove(rproc);
  metal_finish();
  return 0;
  }


// -----------------------------------------------------------

int main(int argc, char *argv[])
  {
  int status, p2, numbytes, msglen;
  float p1;

  // remove buffering from stdin and stdout
  setvbuf (stdout, NULL, _IONBF, 0);
  setvbuf (stdin, NULL, _IONBF, 0);

  LPRINTF("\nR5 test application #5 : shared PL resources + IRQs + IPC (openamp)\n\n");
#ifdef ARMR5
  LPRINTF("This is R5/baremetal side\n\n");
#else
  LPRINTF("This is A53/linux side\n\n");
#endif

  status = SetupSystem(&platform);
  if(status!=0)
    {
    LPRINTF("ERROR Setting up System - aborting\n");
    return status;
    }

  // main loop
  msglen=sizeof(LOOP_PARAM_MSG_TYPE);
  while(1)
    {
    LPRINTF("Enter parameter#1 (float)     : ");
    status=scanf("%f", &p1);
    if(status<1)
      break;
    LPRINTF("Enter parameter#2 (signed int): ");
    status=scanf("%d", &p2);
    if(status<1)
      break;
    // send new parameters to R5
    gMsgPtr->param1=p1;
    gMsgPtr->param2=p2;
    numbytes= rpmsg_send(&lept, gMsgPtr, msglen);
    if(numbytes<msglen)
      LPRINTF("ERROR sending RPMSG\n");
    else
      LPRINTF("Successfully sent RPMSG\n");
    }
  
  // cleanup and exit
  LPRINTF("\nExiting\n");

  status = CleanupSystem(platform);
  if(status!=0)
    {
    LPRINTF("ERROR Cleaning up System\n");
    // continue anyway, as we are shutting down
    //return status;
    }

  return 0;
  }
