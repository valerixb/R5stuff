//
// R5 demo application
// IRQs from PL and 
// interproc communications with A53/linux
//
// constant ARMR5 provides proper code for
// A53/linux and R5/baremetal sides 
//

#include "rproc.h"

static int rproc_irq_handler(int vect_id, void *data)
  {
  struct remoteproc *rproc = data;
  struct remoteproc_priv *prproc;
  unsigned int ipi_intr_status;

  (void)vect_id;    // avoid warning on unused parameter
  if(!rproc)
    return METAL_IRQ_NOT_HANDLED;
  prproc = rproc->priv;
  ipi_intr_status = (unsigned int)metal_io_read32(prproc->ipi_io, IPI_ISR_OFFSET);
  if(ipi_intr_status & prproc->ipi_chn_mask)
    {
    atomic_flag_clear(&prproc->ipi_nokick);
    metal_io_write32(prproc->ipi_io, IPI_ISR_OFFSET, prproc->ipi_chn_mask);
    return METAL_IRQ_HANDLED;
    }
  return METAL_IRQ_NOT_HANDLED;
  }

static struct remoteproc *rproc_init(struct remoteproc *rproc,
    const struct remoteproc_ops *ops, void *arg)
  {
  struct remoteproc_priv *prproc = arg;
  struct metal_device *dev;
  unsigned int irq_vect;
  int ret;
#ifndef ARMR5
// ########### linux side
  metal_phys_addr_t mem_pa;
#endif

  if(!rproc || !prproc || !ops)
    return NULL;

  rproc->priv = prproc;
  rproc->ops = ops;
  prproc->ipi_dev = NULL;

#ifndef ARMR5
// ########### linux side

  prproc->shm_dev = NULL;

  // get shared memory device
  ret = metal_device_open(prproc->shm_bus_name, prproc->shm_name, &dev);
  if(ret)
    {
    LPERROR("failed to open shm device: %d.\n", ret);
    goto err1;
    }
  LPRINTF("Successfully open shm device.\n");
  prproc->shm_dev = dev;
  prproc->shm_io = metal_device_io_region(dev, 0);
  if(!prproc->shm_io)
    goto err2;

  mem_pa = metal_io_phys(prproc->shm_io, 0);
  remoteproc_init_mem(&prproc->shm_mem, "shm", mem_pa, mem_pa,
                      metal_io_region_size(prproc->shm_io), prproc->shm_io);
  remoteproc_add_mem(rproc, &prproc->shm_mem);
  LPRINTF("Successfully added shared memory\n");
#endif

  // get IPI device
  ret = metal_device_open(prproc->ipi_bus_name, prproc->ipi_name, &dev);
  if(ret)
    {
    LPRINTF("failed to open IPI device: %d\n", ret);
    goto err2;
    }
  prproc->ipi_dev = dev;
  prproc->ipi_io = metal_device_io_region(dev, 0);
  if(!prproc->ipi_io)
    goto err3;
  LPRINTF("Successfully probed IPI device\n");
  atomic_store(&prproc->ipi_nokick, 1);
  // register interrupt handler and enable interrupt
  irq_vect = (uintptr_t)dev->irq_info;
  metal_irq_register(irq_vect, rproc_irq_handler, rproc);
  metal_irq_enable(irq_vect);
  metal_io_write32(prproc->ipi_io, IPI_IER_OFFSET, prproc->ipi_chn_mask);
  LPRINTF("Successfully initialized Linux r5 remoteproc.\n");
  return rproc;
  
  err3:
    metal_device_close(prproc->ipi_dev);
  err2:
#ifndef ARMR5
// ########### linux side
    metal_device_close(prproc->shm_dev);
#endif
  err1:
    return NULL;
  }


static void rproc_remove(struct remoteproc *rproc)
  {
  struct remoteproc_priv *prproc;
  struct metal_device *dev;

  if(!rproc)
    return;
  prproc = rproc->priv;
  metal_io_write32(prproc->ipi_io, IPI_IDR_OFFSET, prproc->ipi_chn_mask);
  dev = prproc->ipi_dev;
  if(dev)
    {
    metal_irq_disable((uintptr_t)dev->irq_info);
    metal_irq_unregister((uintptr_t)dev->irq_info);
    metal_device_close(dev);
    }
#ifndef ARMR5
// ########### linux side
  if(prproc->shm_dev)
    metal_device_close(prproc->shm_dev);
#endif
  }


static void *rproc_mmap(struct remoteproc *rproc, metal_phys_addr_t *pa,
    metal_phys_addr_t *da, size_t size,	unsigned int attribute, struct metal_io_region **io)
  {
#ifdef ARMR5
// ########### R5 side
  struct remoteproc_mem *mem;
#else
// ########### linux side
  struct remoteproc_priv *prproc;
#endif
  metal_phys_addr_t lpa, lda;
  struct metal_io_region *tmpio;

  if(!rproc)
    return NULL;
  lpa = *pa;
  lda = *da;

  if(lpa == METAL_BAD_PHYS && lda == METAL_BAD_PHYS)
    return NULL;
  if(lpa == METAL_BAD_PHYS)
    lpa = lda;
  if(lda == METAL_BAD_PHYS)
    lda = lpa;

#ifdef ARMR5
// ########### R5 side

  if(!attribute)
    attribute = NORM_SHARED_NCACHE | PRIV_RW_USER_RW;
  mem = metal_allocate_memory(sizeof(*mem));
  if(!mem)
    return NULL;
  tmpio = metal_allocate_memory(sizeof(*tmpio));
  if(!tmpio)
    {
    metal_free_memory(mem);
    return NULL;
    }
  remoteproc_init_mem(mem, NULL, lpa, lda, size, tmpio);
  // virtual address (va) is the same as physical address (pa) in this platform
  metal_io_init(tmpio, (void *)lpa, &mem->pa, size,
    sizeof(metal_phys_addr_t) << 3, attribute, NULL);
  remoteproc_add_mem(rproc, mem);
  
#else
// ########### linux side

  (void)attribute;    // avoid warning on unused parameter
  (void)size;          // avoid warning on unused parameter
  prproc = rproc->priv;
  tmpio = prproc->shm_io;
  if(!tmpio)
    return NULL;

#endif

  *pa = lpa;
  *da = lda;
  if(io)
    *io = tmpio;
  return metal_io_phys_to_virt(tmpio, lpa);
  }

static int rproc_notify(struct remoteproc *rproc, uint32_t id)
  {
  struct remoteproc_priv *prproc;

  (void)id;    // avoid warning on unused parameter
  if(!rproc)
    return -1;
  prproc = rproc->priv;
  metal_io_write32(prproc->ipi_io, IPI_TRIG_OFFSET, prproc->ipi_chn_mask);
  return 0;
  }

// processor operations from r5 to a53. It defines
// notification operation and remote processor management operations
const struct remoteproc_ops rproc_ops = 
  {
  .init = rproc_init,
  .remove = rproc_remove,
  .mmap = rproc_mmap,
  .notify = rproc_notify,
  .start = NULL,
  .stop = NULL,
  .shutdown = NULL,
  };
