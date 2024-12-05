//
// R5 demo application
// IRQs from PL and 
// interproc communications with A53/linux
//
// constant ARMR5 provides proper code for
// A53/linux and R5/baremetal sides 
//

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <metal/atomic.h>
#include <metal/device.h>
#include <metal/utilities.h>
#include <metal/irq.h>
#include <openamp/remoteproc.h>
#include <openamp/virtio.h>
#include <openamp/rpmsg.h>
#include <openamp/rpmsg_virtio.h>
#include <errno.h>
#include "common.h"

#define SHARED_MEM_PA       0x3ED40000UL
#define SHARED_MEM_SIZE     0x40000UL
#define SHARED_BUF_OFFSET   0x10000UL
#define SHARED_BUF_SIZE     (SHARED_MEM_SIZE - SHARED_BUF_OFFSET)


#ifdef ARMR5
// ########### R5 side
#include <metal/assert.h>
#include "xparameters.h"
#include "xil_cache.h"

#define _rproc_wait() asm volatile("wfi")

#else
// ########### linux side
#include <metal/alloc.h>
#include <metal/io.h>
#include <metal/sys.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/un.h>

#define _rproc_wait() metal_cpu_yield()

#endif


struct remoteproc_priv
  {
  const char *ipi_name;
  const char *ipi_bus_name;
  struct metal_device *ipi_dev;
  struct metal_io_region *ipi_io;
  unsigned int ipi_chn_mask;
  atomic_int ipi_nokick;
#ifndef ARMR5
// ########### linux side
  struct remoteproc_mem shm_mem;
  const char *rsc_name;
  const char *rsc_bus_name;
  const char *shm_name;
  const char *shm_bus_name;
  struct metal_device *shm_dev;
  struct metal_io_region *shm_io;
#endif
  };


// --------- protos
struct rpmsg_device *create_rpmsg_vdev(void *platform, unsigned int vdev_index,
    unsigned int role, void (*rst_cb)(struct virtio_device *vdev),
    rpmsg_ns_bind_cb ns_bind_cb);
int platform_poll(void *platform);
void release_rpmsg_vdev(struct rpmsg_device *rpdev, void *platform);

#endif
