//
// R5 demo application
// IRQs from PL and 
// interproc communications with A53/linux
//
// constant ARMR5 provides proper code for
// A53/linux and R5/baremetal sides 
//

#include "platform.h"

static struct rpmsg_virtio_shm_pool shpool;


struct rpmsg_device *create_rpmsg_vdev(void *platform, unsigned int vdev_index,
    unsigned int role, void (*rst_cb)(struct virtio_device *vdev),
    rpmsg_ns_bind_cb ns_bind_cb)
  {
  struct remoteproc *rproc = platform;
  struct rpmsg_virtio_device *rpmsg_vdev;
  struct virtio_device *vdev;
  void *shbuf;
  struct metal_io_region *shbuf_io;
  int ret;

  rpmsg_vdev = metal_allocate_memory(sizeof(*rpmsg_vdev));
  if(!rpmsg_vdev)
    return NULL;
#ifdef ARMR5
// ########### R5 side
  shbuf_io = remoteproc_get_io_with_pa(rproc, SHARED_MEM_PA);
#else
// ########### linux side
  shbuf_io = remoteproc_get_io_with_pa(rproc, SHARED_MEM_PA + SHARED_BUF_OFFSET);
#endif

  if(!shbuf_io)
    goto err1;
  shbuf = metal_io_phys_to_virt(shbuf_io, SHARED_MEM_PA + SHARED_BUF_OFFSET);

  LPRINTF("creating remoteproc virtio\n");
  vdev = remoteproc_create_virtio(rproc, vdev_index, role, rst_cb);
  if(!vdev)
    {
    LPRINTF("remoteproc_create_virtio failed\n");
    goto err1;
    }

  LPRINTF("initializing rpmsg shared buffer pool\n");
  // only RPMsg virtio driver needs to initialize the shared buffers pool
  rpmsg_virtio_init_shm_pool(&shpool, shbuf, SHARED_BUF_SIZE);

  LPRINTF("initializing rpmsg vdev\n");
  // RPMsg virtio device can set shared buffers pool argument to NULL
  ret = rpmsg_init_vdev(rpmsg_vdev, vdev, ns_bind_cb, shbuf_io, &shpool);
  if(ret)
    {
    LPRINTF("failed rpmsg_init_vdev\r\n");
    goto err2;
    }
  LPRINTF("initializing rpmsg vdev\r\n");
  return rpmsg_virtio_get_rpmsg_device(rpmsg_vdev);

  err2:
    remoteproc_remove_virtio(rproc, vdev);
  err1:
    metal_free_memory(rpmsg_vdev);
    return NULL;
  }

int platform_poll(void *priv)
  {
  struct remoteproc *rproc = priv;
  struct remoteproc_priv *prproc;
  unsigned int flags;
  int ret;

  prproc = rproc->priv;
  while(1) 
    {
    flags = metal_irq_save_disable();
    // if an IPI has occurred
    if(!(atomic_flag_test_and_set(&prproc->ipi_nokick)))
      {
      metal_irq_restore_enable(flags);
      ret = remoteproc_get_notification(rproc, RSC_NOTIFY_ID_ANY);
      if (ret)
        return ret;
      break;
      }
    _rproc_wait();
    metal_irq_restore_enable(flags);
    }
  return 0;
  }

void release_rpmsg_vdev(struct rpmsg_device *rpdev, void *platform)
  {
  struct rpmsg_virtio_device *rpvdev;
  struct remoteproc *rproc;

  rpvdev = metal_container_of(rpdev, struct rpmsg_virtio_device, rdev);
  rproc = platform;

  rpmsg_deinit_vdev(rpvdev);
  remoteproc_remove_virtio(rproc, rpvdev->vdev);
  }
