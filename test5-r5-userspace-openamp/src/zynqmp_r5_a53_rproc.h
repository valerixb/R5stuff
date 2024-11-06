//
// R5 demo application
// IRQs from PL and 
// interproc communications with A53/linux
//
// this is the R5 side
//

#ifndef ZYNQMP_R5_A53_RPROC_H_
#define ZYNQMP_R5_A53_RPROC_H_

#include <metal/atomic.h>
#include <metal/assert.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/utilities.h>
#include <openamp/rpmsg_virtio.h>
#include "r5usropenamp.h"

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
extern const struct remoteproc_ops zynqmp_r5_a53_proc_ops;

// --------- protos
static int zynqmp_r5_a53_proc_irq_handler(int vect_id, void *data);
static struct remoteproc *zynqmp_r5_a53_proc_init(struct remoteproc *rproc, 
    const struct remoteproc_ops *ops, void *arg);
static void zynqmp_r5_a53_proc_remove(struct remoteproc *rproc);
static void *zynqmp_r5_a53_proc_mmap(struct remoteproc *rproc, metal_phys_addr_t *pa,
    metal_phys_addr_t *da, size_t size, unsigned int attribute, struct metal_io_region **io);
static int zynqmp_r5_a53_proc_notify(struct remoteproc *rproc, uint32_t id);

#endif