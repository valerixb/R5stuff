//
// R5 demo application
// IRQs from PL and 
// interproc communications with A53/linux
//
// this is the R5 side
//

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include "r5usropenamp.h"
#include "xparameters.h"
#include "xil_cache.h"
#include <openamp/remoteproc.h>
#include <openamp/virtio.h>
#include <openamp/rpmsg.h>
#include <metal/atomic.h>
#include <metal/assert.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/utilities.h>
#include <openamp/rpmsg_virtio.h>
#include <errno.h>
#include "rsc_table.h"


#define XPAR_XIPIPSU_0_BASE_ADDRESS 0xff310000
#define XPAR_XIPIPSU_0_INT_ID 65

// Interrupt vectors
#define IPI_IRQ_VECT_ID     XPAR_XIPIPSU_0_INT_ID
#define POLL_BASE_ADDR      XPAR_XIPIPSU_0_BASE_ADDRESS
#define IPI_CHN_BITMASK     0x01000000

#define KICK_DEV_NAME         "poll_dev"
#define KICK_BUS_NAME         "generic"

#define SHARED_MEM_PA  0x3ED40000UL
#define SHARED_MEM_SIZE 0x100000UL
#define SHARED_BUF_OFFSET 0x8000UL

#define WAIT_FOR_INTERRUPT() asm volatile("wfi")

struct remoteproc_priv
  {
  const char *kick_dev_name;
  const char *kick_dev_bus_name;
  struct metal_device *kick_dev;
  struct metal_io_region *kick_io;
  unsigned int ipi_chn_mask;
  atomic_int ipi_nokick;
  };

struct rpmsg_device *create_rpmsg_vdev(void *platform, unsigned int vdev_index,
    unsigned int role,
    void (*rst_cb)(struct virtio_device *vdev),
    rpmsg_ns_bind_cb ns_bind_cb);
int platform_poll(void *platform);
void release_rpmsg_vdev(struct rpmsg_device *rpdev, void *platform);

#endif
