/*
  Copyright (C) 2015 CS416/CS516

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#ifndef _BLOCK_H_
#define _BLOCK_H_

#define BLOCK_SIZE 512 //Size of block

#define BLOCK_DEVICE_SIZE (4*1024*1024*256) //1GB in bytes
#define NUM_OF_BLOCKS (BLOCK_DEVICE_SIZE/BLOCK_SIZE)

#define MAX_PATH 256
#define MAX_NAME 32

struct fat_entry {
	int in_use;
};

typedef struct fat_entry fe_t;

void init_fat_entry(fe_t *entry, int in_use);

#define NUM_OF_FAT_BLOCKS (NUM_OF_BLOCKS * sizeof(fe_t) / BLOCK_SIZE)
#define NUM_OF_FAT_ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(fe_t))

struct file_allocation_table {
	fe_t entry[NUM_OF_BLOCKS];
};

typedef struct file_allocation_table fat_t;

struct block_pointer {
  char name[MAX_NAME];
};

typedef struct block_pointer block_t;

#define NUM_OF_BP_BLOCKS (NUM_OF_BLOCKS * sizeof(block_t) / BLOCK_SIZE)
#define NUM_OF_BP_ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(block_t))

struct block_pointer_space {
	block_t block_pointer[NUM_OF_BLOCKS];
};

typedef struct block_pointer_space bps_t;

struct super_block {
	int block_size;
	long number_of_blocks;
	int num_fat_blocks;
	int root_block;
	char padding[492];
};

typedef struct super_block super;

#define INITIAL_SIZE (BLOCK_SIZE * (1+NUM_OF_FAT_BLOCKS+NUM_OF_BP_BLOCKS+1))

void disk_open(const char *diskfile_path);

void disk_close();

int block_read(const int block_num, void *buf);

int block_write(const int block_num, const void *buf);

void init_fat_entry(fe_t *entry, int in_use);

void init_bps_entry(block_t *entry, char* name);

long effective_data_block_number(long number);

#endif
