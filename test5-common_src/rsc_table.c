//
// R5 demo application
// IRQs from PL and 
// interproc communications with A53/linux
//
// this file populates resource table 
// for BM remote for use by the Linux host
//
// this file is common to R5/baremetal and A53/linux
//

#include "rsc_table.h"

struct remote_resource_table __resource resources = 
  {
  // Version
  1,
  // number of table entries
  NUM_TABLE_ENTRIES,
  // reserved fields
  {0, 0,},
  // offsets of rsc entries
  { offsetof(struct remote_resource_table, rpmsg_vdev),},
  // virtio device entry
  {
  RSC_VDEV, VIRTIO_ID_RPMSG_, 31, RPMSG_VDEV_DFEATURES, 0, 0, 0,
  NUM_VRINGS, {0, 0},
  },
  // vring rsc entry - part of vdev rsc entry
  {RING_TX, VRING_ALIGN, VRING_SIZE, 1, 0},
  {RING_RX, VRING_ALIGN, VRING_SIZE, 2, 0},
  };

void *get_resource_table(int rsc_id, int *len)
  {
  (void) rsc_id;
  *len = sizeof(resources);
  return &resources;
  }
