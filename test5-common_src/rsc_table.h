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

#ifndef RSC_TABLE_H_
#define RSC_TABLE_H_

#include <stddef.h>
#include <openamp/open_amp.h>

#define NO_RESOURCE_ENTRIES         8

// Place resource table in special ELF section
#define __section_t(S)          __attribute__((__section__(#S)))
#define __resource              __section_t(.resource_table)

#define RPMSG_VDEV_DFEATURES        (1 << VIRTIO_RPMSG_F_NS)

// VirtIO rpmsg device id
#define VIRTIO_ID_RPMSG_             7

#define NUM_VRINGS                  0x02
#define VRING_ALIGN                 0x1000
#define RING_TX                     0x3ed40000
#define RING_RX                     0x3ed44000
#define VRING_SIZE                  256

#define NUM_TABLE_ENTRIES           1


// Resource table for the given remote
struct remote_resource_table 
  {
  unsigned int version;
  unsigned int num;
  unsigned int reserved[2];
  unsigned int offset[NO_RESOURCE_ENTRIES];
  // rpmsg vdev entry
  struct fw_rsc_vdev rpmsg_vdev;
  struct fw_rsc_vdev_vring rpmsg_vring0;
  struct fw_rsc_vdev_vring rpmsg_vring1;
  }__attribute__((packed, aligned(0x100)));

void *get_resource_table(int rsc_id, int *len);

#endif
