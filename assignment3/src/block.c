/*
  Copyright (C) 2015 CS416/CS516

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "block.h"

int diskfile = -1;

void disk_open(const char *diskfile_path) {
    if (diskfile >= 0) {
        return;
    }

    diskfile = open(diskfile_path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (diskfile < 0) {
        perror("disk_open failed");
        exit(EXIT_FAILURE);
    }
}

void disk_close() {
    if (diskfile >= 0) {
        close(diskfile);
    }
}

/** Read a block from an open file
 *
 * Read should return (1) exactly @BLOCK_SIZE when succeeded, or (2) 0 when the requested block has never been touched before, or (3) a negtive value when failed. 
 * In cases of error or return value equals to 0, the content of the @buf is set to 0.
 */
int block_read(const int block_num, void *buf) {
    int retstat = 0;
    retstat = pread(diskfile, buf, BLOCK_SIZE, block_num * BLOCK_SIZE);
    if (retstat <= 0) {
        memset(buf, 0, BLOCK_SIZE);
        if (retstat < 0) {
          perror("\nblock_read failed\n");
          // char message[3000];
          // explain_message_pread(message, sizeof(message), diskfile, buf, BLOCK_SIZE, block_num * BLOCK_SIZE);
          // perror(message);
        }
    }

    return retstat;
}

/** Write a block to an open file
 *
 * Write should return exactly @BLOCK_SIZE except on error.
 */
int block_write(const int block_num, const void *buf) {
  int retstat = 0;
  retstat = pwrite(diskfile, buf, BLOCK_SIZE, block_num * BLOCK_SIZE);
  if (retstat < 0) {
    perror("\nblock_write failed\n");
  }
  return retstat;
}

void init_fat_entry(fe_t *entry, int in_use) {
  entry->in_use = in_use;
}

void init_bps_entry(block_t *entry, char* name) {
  if(name != NULL) {
    memset(entry->name, '\0', MAX_NAME);
    strncpy(entry->name, name, strlen(name));
  }
}

long effective_data_block_number(long number) {
  long value = 0;
  if(number < 0 || number > NUM_OF_BLOCKS) {
    perror("\nError: Invalid block number\n");
    value = 1 + NUM_OF_FAT_BLOCKS + NUM_OF_BP_BLOCKS; //Return root data block as default
  } else
    value = number + 1 + NUM_OF_FAT_BLOCKS + NUM_OF_BP_BLOCKS;
  return value;
}
