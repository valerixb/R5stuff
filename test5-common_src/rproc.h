//
// R5 demo application
// IRQs from PL and 
// interproc communications with A53/linux
//
// constant ARMR5 provides proper code for
// A53/linux and R5/baremetal sides 
//

#ifndef RPROC_H_
#define RPROC_H_

#include <metal/atomic.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/utilities.h>
#include <openamp/rpmsg_virtio.h>
#include "platform.h"
#include "common.h"

#ifdef ARMR5
// ########### R5 side
#include <metal/assert.h>

#else
// ########### linux side
#include <metal/io.h>
#include <metal/alloc.h>
#include <openamp/remoteproc.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/un.h>

#endif

// -------  IPI REGs OFFSETs
// IPI trigger register offset
#define IPI_TRIG_OFFSET          0x00000000
// IPI observation register offset
#define IPI_OBS_OFFSET           0x00000004
// IPI interrupt status register offset
#define IPI_ISR_OFFSET           0x00000010
// IPI interrupt mask register offset
#define IPI_IMR_OFFSET           0x00000014
// IPI interrupt enable register offset
#define IPI_IER_OFFSET           0x00000018
// IPI interrupt disable register offset
#define IPI_IDR_OFFSET           0x0000001C

// processor operations from r5 to a53. It defines
// notification operation and remote processor management operations
extern const struct remoteproc_ops rproc_ops;

// --------- protos
static int rproc_irq_handler(int vect_id, void *data);
static struct remoteproc *rproc_init(struct remoteproc *rproc,
    const struct remoteproc_ops *ops, void *arg);
static void rproc_remove(struct remoteproc *rproc);
static void *rproc_mmap(struct remoteproc *rproc, metal_phys_addr_t *pa,
    metal_phys_addr_t *da, size_t size, unsigned int attribute, struct metal_io_region **io);
static int rproc_notify(struct remoteproc *rproc, uint32_t id);

#endif
