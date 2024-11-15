//
// R5 demo application
// IRQs from PL and 
// interproc communications with A53/linux
//
// this is the linux/A53 side
//

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <metal/alloc.h>
#include <metal/atomic.h>
#include <metal/io.h>
#include <metal/irq.h>
#include <metal/device.h>
#include <metal/utilities.h>
#include <metal/sys.h>
#include <openamp/remoteproc.h>
#include <openamp/rpmsg_virtio.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/un.h>
#include <openamp/virtio.h>
#include <openamp/rpmsg.h>
#include "a53linux_usropenamp.h"


struct remoteproc_priv
  {
  const char *ipi_name;
  const char *ipi_bus_name;
  const char *rsc_name;
  const char *rsc_bus_name;
  const char *shm_name;
  const char *shm_bus_name;
  struct metal_device *ipi_dev;
  struct metal_io_region *ipi_io;
  struct metal_device *shm_dev;
  struct metal_io_region *shm_io;
  struct remoteproc_mem shm_mem;
  unsigned int ipi_chn_mask;
  atomic_int ipi_nokick;
  };

#define RPU_CPU_ID          0
#define IPI_CHN_BITMASK	    0x00000100
#define IPI_DEV_NAME	      "ff340000.ipi"

#define DEV_BUS_NAME        "platform" 
#define SHM_DEV_NAME        "3ed20000.shm"
#define RSC_MEM_PA          0x3ED20000UL
#define RSC_MEM_SIZE        0x2000UL
#define VRING_MEM_PA        0x3ED40000UL
#define VRING_MEM_SIZE      0x8000UL
#define SHARED_BUF_PA       0x3ED48000UL
#define SHARED_BUF_SIZE     0x40000UL

#define _rproc_wait() metal_cpu_yield()

// --------- protos
struct rpmsg_device *create_rpmsg_vdev(void *platform, unsigned int vdev_index,
    unsigned int role, void (*rst_cb)(struct virtio_device *vdev),
    rpmsg_ns_bind_cb ns_bind_cb);
int platform_poll(void *platform);
void release_rpmsg_vdev(struct rpmsg_device *rpdev, void *platform);

#endif
