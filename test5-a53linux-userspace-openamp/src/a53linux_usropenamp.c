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

#include "a53linux_usropenamp.h"

// change_me
struct _payload {
	unsigned long num;
	unsigned long size;
	unsigned char data[];
};

#define PAYLOAD_MIN_SIZE	1
// /change_me

// ##########  globals  #######################

void *platform;
// openamp
static struct rpmsg_endpoint lept;
static LOOP_PARAM_MSG_TYPE gLoopParameters;
static LOOP_PARAM_MSG_TYPE *gMsgPtr;
struct rpmsg_device *rpdev;
static struct remoteproc rproc_inst;

struct remoteproc_priv rproc_priv = 
  {
  .shm_name = SHM_DEV_NAME,
  .shm_bus_name = DEV_BUS_NAME,
  .ipi_name = IPI_DEV_NAME,
  .ipi_bus_name = DEV_BUS_NAME,
  .ipi_chn_mask = IPI_CHN_BITMASK,
  };

// change_me
static struct _payload *i_payload;
static int rnum = 0;
static int err_cnt = 0;
static int ept_deleted = 0;
// /change_me


/*-----------------------------------------------------------------------------*
 *  RPMSG endpoint callbacks
 *-----------------------------------------------------------------------------*/
static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
			     uint32_t src, void *priv)
{
	int i;
	struct _payload *r_payload = (struct _payload *)data;

	(void)ept;
	(void)src;
	(void)priv;
	LPRINTF(" received payload number %lu of size %lu \r\n",
		r_payload->num, (unsigned long)len);

	if (r_payload->size == 0) {
		LPERROR(" Invalid size of package is received.\r\n");
		err_cnt++;
		return RPMSG_SUCCESS;
	}
	/* Validate data buffer integrity. */
	for (i = 0; i < (int)r_payload->size; i++) {
		if (r_payload->data[i] != 0xA5) {
			LPRINTF("Data corruption at index %d\r\n", i);
			err_cnt++;
			break;
		}
	}
	rnum = r_payload->num + 1;
	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	(void)ept;
	rpmsg_destroy_ept(&lept);
	LPRINTF("echo test: service is destroyed\r\n");
	ept_deleted = 1;
}

static void rpmsg_name_service_bind_cb(struct rpmsg_device *rdev,
				       const char *name, uint32_t dest)
{
	LPRINTF("new endpoint notification is received.\r\n");
	if (strcmp(name, RPMSG_SERVICE_NAME))
		LPERROR("Unexpected name service %s.\r\n", name);
	else
		(void)rpmsg_create_ept(&lept, rdev, RPMSG_SERVICE_NAME,
				       RPMSG_ADDR_ANY, dest,
				       rpmsg_endpoint_cb,
				       rpmsg_service_unbind);

}

/*-----------------------------------------------------------------------------*
 *  Application
 *-----------------------------------------------------------------------------*/
int app (struct rpmsg_device *rdev, void *priv)
{
	int ret;
	int i;
	int size, max_size, num_payloads;
	int expect_rnum = 0;

	LPRINTF(" 1 - Send data to remote core, retrieve the echo");
	LPRINTF(" and validate its integrity ..\r\n");

	
	for (i = 0, size = PAYLOAD_MIN_SIZE; i < num_payloads; i++, size++) {
		i_payload->num = i;
		i_payload->size = size;

		/* Mark the data buffer. */
		memset(&(i_payload->data[0]), 0xA5, size);

		LPRINTF("sending payload number %lu of size %lu\r\n",
			i_payload->num,
			(unsigned long)(2 * sizeof(unsigned long)) + size);

		ret = rpmsg_send(&lept, i_payload,
				 (2 * sizeof(unsigned long)) + size);

		if (ret < 0) {
			LPERROR("Failed to send data...\r\n");
			break;
		}
		LPRINTF("echo test: sent : %lu\r\n",
			(unsigned long)(2 * sizeof(unsigned long)) + size);

		expect_rnum++;
		do {
			platform_poll(priv);
		} while ((rnum < expect_rnum) && !err_cnt && !ept_deleted);

	}

	LPRINTF("**********************************\r\n");
	LPRINTF(" Test Results: Error count = %d \r\n", err_cnt);
	LPRINTF("**********************************\r\n");
	/* Destroy the RPMsg endpoint */
	rpmsg_destroy_ept(&lept);
	LPRINTF("Quitting application .. Echo test end\r\n");

	metal_free_memory(i_payload);
	return 0;
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
  if(!remoteproc_init(&rproc_inst, &zynqmp_linux_r5_proc_ops, &rproc_priv))
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

static void system_metal_logger(enum metal_log_level level, const char *format, ...)
  {
  (void)level;
  (void)format;
  }


// -----------------------------------------------------------

int SetupSystem(void **platformp)
  {
  int status, max_size;
  struct remoteproc *rproc;
  struct metal_init_params metal_param = 
    {
    .log_handler = system_metal_logger,
    .log_level = METAL_LOG_INFO,
    };

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
  }


// -----------------------------------------------------------

int main(int argc, char *argv[])
  {
  int status, p2;
  float p1;

  // remove buffering from stdin and stdout
  setvbuf (stdout, NULL, _IONBF, 0);
  setvbuf (stdin, NULL, _IONBF, 0);

  status = SetupSystem(&platform);
  if(status!=0)
    {
    printf("ERROR Setting up System - aborting\n");
    return status;
    }

  // main loop
  while(1)
    {
    printf("Enter parameter#1 (float)     : ");
    status=scanf("%f", &p1);
    if(status<1)
      break;
    printf("Enter parameter#2 (signed int): ");
    status=scanf("%d", &p2);
    if(status<1)
      break;
    // send new parameters to R5
    gMsgPtr->param1=p1;
    gMsgPtr->param2=p2;
    status= rpmsg_send(&lept, gMsgPtr, sizeof(LOOP_PARAM_MSG_TYPE));
    if(status!=0)
      LPRINTF("ERROR sending RPMSG\n");
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
