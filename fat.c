#include <stdint.h>
#include <unistd.h>
#include <strings.h>
#include <stdio.h>
#include "fat.h"

static uint8_t *write16(uint8_t *buf, uint16_t val) {
  *buf++ = (val & 0xff);
  *buf++ = (val  >> 8) & 0xff;
  return buf;
}

int datagen(uint32_t blknum, uint8_t *buf, const vfile_t *filesys, uint32_t index) {
  const uint8_t *data = (const uint8_t *) filesys->table[index].data;
  uint32_t len = filesys->table[index].len;

  for (int i = 0; (i < 512) && (i  + blknum*512 < len); i++)
    buf[i] = data[i+blknum*512];
  return 0;
}

int fatgen(uint32_t blknum, uint8_t *buf, const vfile_t *filesys, uint32_t idx) {
  (void) idx;
  uint32_t cluster = blknum * 256;
  uint32_t lastcluster  = cluster + 256;

  if (!cluster) {
    buf = write16(buf, 0xFFF8);
    buf = write16(buf, 0xFFFF);
    cluster += 2;
  }

  int index = 4;
  for (; cluster < lastcluster; cluster++) {

    // end of file system

    if (cluster >= filesys->maxcluster) {
      buf = write16(buf, 0xFFF7);
      continue;
    }

    // find correct entry in table

    while (cluster >= filesys->table[index+1].start_cluster) index++;
    
    int cmp = (filesys->table[index].start_cluster +
	       filesys->table[index].len/(512*filesys->blocks_per_cluster)) - cluster;
    if (cmp == 0)  // last cluster
      buf = write16(buf,0xFFFF);
    if (cmp > 0)   // not last
      buf = write16(buf,cluster+1);
    if (cmp < 0)  // after last
      buf = write16(buf,0);
  }
  return 0;
}

int read_vdisk(const void *instance, uint32_t startblk, uint8_t *buf, uint32_t n) {
  const vfile_t *ft = (vfile_t *) instance;
  int index = 0;
  while (n--) {
    bzero(buf,512);
    while (ft->table[index + 1].start_block <= startblk) index++;
    if (startblk < UINT32_MAX) {
      if (ft->table[index].fun) {
	ft->table[index].fun(startblk - ft->table[index].start_block, buf, ft, index);
      }
      startblk++;
    }
    buf += 512;
  }
  return 0;
}

