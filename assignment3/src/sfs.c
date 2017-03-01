/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/

#include "params.h"
#include "block.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "sfs.h"
#include "log.h"
#include "utils.h"

//Praveen

//// FS Structure

super block_0 = {
  .block_size = BLOCK_SIZE,
  .number_of_blocks = NUM_OF_BLOCKS + NUM_OF_FAT_BLOCKS + NUM_OF_BP_BLOCKS + 1, //data + fat + bps + super
  .num_fat_blocks = NUM_OF_FAT_BLOCKS,
  .root_block = NUM_OF_BP_BLOCKS + NUM_OF_FAT_BLOCKS + 1,
};

fat_t fat_base;
fat_t *fat;

bps_t bps_base;
bps_t *bps;

dirent_t root_node;
dirent_t *root;

int max_blocks = 20 + 8 * 128 + 8 * 128 * 128; //Direct + Indirect 1 + Indirect 2 - max number of data blocks per file

/* End */

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

//Prototypes
void print_statbuf(struct stat* statbuf);

//End Prototypes

long get_block_start(long block_num, long NUM_OF_XX_PER_BLOCK) {
  long factor = block_num / NUM_OF_XX_PER_BLOCK;
  return factor * NUM_OF_XX_PER_BLOCK;
}

int read_fat_block(long block_num) {
  long i = block_num;
  int retval = block_read((i/NUM_OF_FAT_ENTRIES_PER_BLOCK) + 1, fat->entry + get_block_start(i, NUM_OF_FAT_ENTRIES_PER_BLOCK));
  if(retval < 0)
    log_msg("\n Error reading fat block \n");
  return retval;
}

int read_bps_block(long block_num) {
  long i = block_num;
  int retval = block_read((i/NUM_OF_BP_ENTRIES_PER_BLOCK) + 1 + NUM_OF_FAT_BLOCKS, bps->block_pointer + get_block_start(i, NUM_OF_BP_ENTRIES_PER_BLOCK));
  if(retval < 0)
    log_msg("\n Error reading bps block \n");
  return retval;
}

int write_fat_block(long block_num) {
  long i = block_num;
  int retval = block_write((i/NUM_OF_FAT_ENTRIES_PER_BLOCK) + 1, fat->entry + get_block_start(i, NUM_OF_FAT_ENTRIES_PER_BLOCK));
  if(retval < 0)
    log_msg("\n Error updating fat block \n");
  return retval;
}

int write_bps_block(long block_num) {
  long i = block_num;
  int retval = block_write((i/NUM_OF_BP_ENTRIES_PER_BLOCK) + 1 + NUM_OF_FAT_BLOCKS, bps->block_pointer + get_block_start(i, NUM_OF_BP_ENTRIES_PER_BLOCK));
  if(retval < 0)
    log_msg("\n Error updating bps block \n");
  return retval;
}


void init_fs() {
  int retval = block_write(0, &block_0);
  fat = &fat_base;
  bps = &bps_base;
  init_fat_entry(fat->entry + 0, 1); //Root directory entry
  init_bps_entry(bps->block_pointer + 0, "/\0"); //Root Directory entry
  long i;
  for(i=2;i<=NUM_OF_BLOCKS;i++) {
    init_fat_entry(fat->entry + (i-1), 0);
    init_bps_entry(bps->block_pointer + (i-1), '\0');
    if(i % NUM_OF_FAT_ENTRIES_PER_BLOCK == 0) {
      retval = write_fat_block(i - 1);
    }
    if(i % NUM_OF_BP_ENTRIES_PER_BLOCK == 0) {
      retval = write_bps_block(i - 1);
    }
  }
  //root directory
  root = &root_node;
  init_dirent(root, "/\0", NULL, -1, "/\0", -1);
  block_write(block_0.root_block, root);
}

void read_fs() {
  int retval;
  fat = &fat_base;
  memset(fat, 0, sizeof(fat_t));
  bps = &bps_base;
  memset(bps, 0, sizeof(bps_t));
  long i;
  for(i = 1; i <= NUM_OF_BLOCKS; i ++) {
    if(i % NUM_OF_FAT_ENTRIES_PER_BLOCK == 0) {
      retval = read_fat_block(i - 1);
    }
    if(i % NUM_OF_BP_ENTRIES_PER_BLOCK == 0) {
      retval = read_bps_block(i - 1);
    }
  }

  //root directory
  root = &root_node;
  memset(root, 0, sizeof(dirent_t));
  retval = block_read(effective_data_block_number(0), root);
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *sfs_init(struct fuse_conn_info *conn) {
  fprintf(stderr, "in sfs-init\n");
  log_msg("\nsfs_init()\n");

  log_conn(conn);
  log_fuse_context(fuse_get_context());

  struct stat fileStat;
  if(stat(SFS_DATA->diskfile,&fileStat) < 0) {
    log_msg("\nCannot stat diskFile\n");
    return SFS_DATA;
  }

  disk_open(SFS_DATA->diskfile);

  if(fileStat.st_size < INITIAL_SIZE) {
    init_fs();
  } else {
    read_fs();
  }

  if(stat(SFS_DATA->diskfile,&fileStat) < 0) {
    log_msg("\nCannot stat diskFile\n");
    return SFS_DATA;
  }

  //print_statbuf(&fileStat);
  log_msg("\nInitial size: %d\n", INITIAL_SIZE);

  return SFS_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata) {
  log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
  disk_close();
}

void print_statbuf(struct stat *statbuf) {
  log_msg("\nstatbuf: 0x%08x\n", statbuf);
  log_msg("\nst_dev: %d\n", statbuf->st_dev);
  log_msg("\nst_ino: %d\n", statbuf->st_ino);
  log_msg("\nst_mode: %s\n", lsperms(statbuf->st_mode));
  log_msg("\nst_nlink: %d\n", statbuf->st_nlink);
  log_msg("\nst_uid: %d\n", statbuf->st_uid);
  log_msg("\nst_gid: %d\n", statbuf->st_gid);
  log_msg("\nst_rdev: %d\n", statbuf->st_rdev);
  log_msg("\nst_size: %d bytes\n", statbuf->st_size);
  log_msg("\nst_atime: %d\n", statbuf->st_atime);
  log_msg("\nst_mtime: %d\n", statbuf->st_mtime);
  log_msg("\nst_ctime: %d\n", statbuf->st_ctime);
  log_msg("\nst_blksize: %d\n", statbuf->st_blksize);
  log_msg("\nst_blocks: %d\n", statbuf->st_blocks);
}

int find_block_with_name(const char *name, dirent_t *node) {
  int i, num = -1;
  block_t base;
  block_t *current_block = &base;
  for(i = 0; i < 10; i++) {
    num = node->direct[i];
    if(num == -1 || num < 0 || num > NUM_OF_BLOCKS) {
      num = -1;
      continue;
    }
    read_bps_block(num);
    memcpy(current_block, bps->block_pointer + num, sizeof(block_t));
    if(strcmp(current_block->name, name) == 0)
      break;
    else
      num = -1;
  }
  return num;
}

int find_block_number(const char *path) {
  if(strcmp(path, "/") == 0)
    return 0; //root path

  //Split path into fragments
  int i, retval;
  char** tokens, temp_path[MAX_PATH], *path_temp;
  path_temp = temp_path;
  memset(path_temp, '\0', MAX_PATH);
  path_temp = strdup(path);
  tokens = str_split(path_temp, '/');

  //find the data block number
  dirent_t next, *next_node;
  next_node = &next;

  dirent_t *current_node = root;

  int block = -1;

  for (i = 0; *(tokens + i); i++)
  {
    //log_msg("\nPath part= %s\n", *(tokens + i));

    block = find_block_with_name(*(tokens + i), current_node);

    if(block == -1 || block < 0 || block > NUM_OF_BLOCKS) {
      block = -1;
      break;
    } else {
      memset(next_node, 0, sizeof(dirent_t));
      retval = block_read(effective_data_block_number(block), next_node);
      if(retval != 512)
        log_msg("\n Didn't read everything \n");
      if(S_ISDIR(next_node->mode)) {
        current_node = next_node;
      } else {
        return block;
      }
    }
  }

  return block;
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf) {
  int retstat = 0, retval, data_block;

  log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n", path, statbuf);

  data_block = find_block_number(path);

  if(data_block == -1) {
    log_msg("\n File not found \n");
    errno = ENOENT;
    return -ENOENT;
  }

  //log_msg("\n effective_data_block_number: %ld\n", effective_data_block_number(data_block));

  if(fat->entry[data_block].in_use != 1) {
    log_msg("\n%d\n", fat->entry[data_block].in_use);
    log_msg("\nBlock not in use\n");
    errno = ENOENT;
    return -ENOENT;
  }

  dirent_t directory_node, *dir_node;
  inode_t file_node, *f_node;

  f_node = &file_node;
  dir_node = &directory_node;

  retval = block_read(effective_data_block_number(data_block), dir_node);
  retval = block_read(effective_data_block_number(data_block), f_node);

  if(S_ISDIR(dir_node->mode)) {
    set_directory_stats(statbuf, dir_node);
  } else if (S_ISREG(f_node->mode)) {
    set_file_stats(statbuf, f_node);
  } else {
    log_msg("\nfound something else\n");
  }

  //print_statbuf(statbuf);

  return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  int retstat = 0, retval;
  log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n", path, mode, fi);

  inode_t file_node, *fil_node;
  fil_node = &file_node;

  dirent_t parent, *par_dir;
  par_dir = &parent;

  char **tokens = str_split(strdup(path+1), '/');
  char *filename;
  char parent_path[MAX_PATH];

  int i;
  for(i = 0; *(tokens + i); i++) {
    filename = *(tokens + i);
  }

  memset(parent_path, '\0', MAX_PATH);
  strncpy(parent_path, "/\0", strlen("/\0"));

  strncat(parent_path, path + 1, strlen(path) - strlen(filename) -1);
  //log_msg("\n parent_path %s : %d\n", parent_path, strlen(parent_path));

  if(parent_path == NULL || strcmp(parent_path, "") == 0) {
    log_msg("\n Attempted to create under %s \n", path);
    strncpy(parent_path, "/\0", MAX_PATH);
  }

  // log_msg("\nparent path: %s\n", parent_path);
  // log_msg("\nfilename: %s\n", filename);

  int parent_block_num = find_block_number(parent_path);

  //log_msg("\n parent_block_num %d \n", parent_block_num);

  retval = block_read(effective_data_block_number(parent_block_num), par_dir);

  init_inode(fil_node, par_dir, parent_block_num, filename, path);

  i = 0;
  fe_t *entry;
  while(1 && i < NUM_OF_BLOCKS) {
    entry = fat->entry + i;
    if(entry->in_use == 0) {
      break;
    }
    i++;
  }

  if(i == 0 || i == NUM_OF_BLOCKS) {
    retstat = -ENOMEM;
    goto end_sfs_create;
  }

  int assoc_with_parent = 0;

  int j;
  for(j = 0; j < 10; j++) {
    if(par_dir->direct[j] == -1) {
      par_dir->direct[j] = i;
      assoc_with_parent = 1;
      //log_msg("\n direct %d = %d \n", j, par_dir->direct[j]);
      break;
    }
  }

  if(assoc_with_parent == 0) {
    log_msg("\n Cannot add more files into this directory\n");
    retstat = -ENOMEM;
  } else {
    fil_node->open = 1;
    fi->fh = 1; //Not sure how I will use it.
    int f_retval = block_write(effective_data_block_number(i), fil_node);
    int d_retval = block_write(effective_data_block_number(parent_block_num), par_dir);
    if(f_retval > 0 && d_retval > 0) {
      fat->entry[i].in_use = 1;
      memset(bps->block_pointer[i].name, '\0', MAX_NAME);
      strncpy(bps->block_pointer[i].name, filename, strlen(filename));
      write_fat_block(i);
      write_bps_block(i);
      retstat = 0;

      if(parent_block_num == 0) {
        memcpy(root, par_dir, sizeof(dirent_t));
      }

      //log_msg("\n Written file \n");
    }
    else
      retstat = -ENOENT;
  }
end_sfs_create:
  return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path) {
  int retstat = 0;
  log_msg("sfs_unlink(path=\"%s\")\n", path);

  int data_block = find_block_number(path);

  if(data_block == -1) {
    log_msg("File Not found");
    return -ENOENT;
  }

  dirent_t parent, *dp;
  inode_t fil, *fp;
  fp = &fil;
  dp = &parent;

  int freed = 0;

  block_read(effective_data_block_number(data_block), fp);
  int parent_block = fp->parent;

  //Update FAT table
  fat->entry[data_block].in_use = 0;
  freed++;
  //Reset BPS
  init_bps_entry(bps->block_pointer + data_block, '\0');
  //Remove parent reference to file
  block_read(effective_data_block_number(fp->parent), dp);
  int i;

  for(i=0;i<10;i++)
    if(dp->direct[i] == data_block) {
      dp->direct[i] = -1;
      //log_msg("\n Removed reference from parent directory \n ");
      break;
    }
  //Update data blocks of file referenced by direct
  int block_no;

  for(i=0;i<max_blocks;i++) {
    block_no = block_index_to_direct_indirect_block_number(fp, i);
    if(block_no == -1) {
      break;
    } else {
      fat->entry[block_no].in_use = 0;
      write_fat_block(block_no);
      freed++;
    }
  }
  //Free indirect pointer references
  for(i=0;i<8;i++) {
    if(fp->indirect_1[i] != -1) {
      fat->entry[fp->indirect_1[i]].in_use = 0;
      write_fat_block(fp->indirect_1[i]);
      freed++;
    }
  }
  //Write back everything
  block_write(effective_data_block_number(parent_block), dp); //Parent
  write_fat_block(data_block);
  write_bps_block(data_block);
  if(fp->parent == 0) {
    memcpy(root, dp, sizeof(dirent_t));
  }

  log_msg("\n Freed: %d \n", freed);

  return retstat;
}

/** Change the access and/or modification times of a file */
int sfs_utime(const char *path, struct utimbuf *ubuf)
{
  int retstat = 0;

  log_msg("\nsfs_utime(path=\"%s\", ubuf=0x%08x)\n",
    path, ubuf);

  int data_block = find_block_number(path);

  dirent_t dir, *dp;
  inode_t fil, *fp;

  dp = &dir;
  fp = &fil;

  block_read(effective_data_block_number(data_block), dp);
  block_read(effective_data_block_number(data_block), fp);

  if(S_ISDIR(dp->mode)) {
    ubuf->actime = dp->date_modified;
    ubuf->modtime = dp->date_modified;
    retstat = 0;
  } else if(S_ISREG(fp->mode)) {
    ubuf->actime = fp->date_accessed;
    ubuf->modtime = fp->date_modified;
    retstat = 0;
  } else {
    retstat = -1;
  }

  return retstat;
}


/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi) {
  int retstat = 0;
  log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
          path, fi);

  int data_block = find_block_number(path);

  if(data_block == -1) {
    log_msg("\n File not found \n");
    fi->fh = -1;
    return -ENOENT;
  }

  inode_t fil, *fp;
  fp = &fil;

  block_read(effective_data_block_number(data_block), fp);
  fp->date_accessed = timestamp();
  block_write(effective_data_block_number(data_block), fp);

  fi->fh = 1;

  return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi) {
  int retstat = 0;
  log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
          path, fi);

  if(fi->fh == 1) {
    fi->fh = 0;
    int data_block = find_block_number(path);

    inode_t *fp, f;
    fp = &f;
    block_read(effective_data_block_number(data_block), fp);
    fp->open = 0;
    block_write(effective_data_block_number(data_block), fp);

    retstat = 0;
  }
  return retstat;
}

/*
Read / Write Utils
*/

int get_empty_block() {
  int j = 0;
  for(;j < NUM_OF_BLOCKS; j++)
    if(fat->entry[j].in_use == 0)
      return j;
  return -1;
}

int block_index_to_direct_indirect_block_number(inode_t *fp, int block_index) {
  int j, k, block_number = -1;
  if(block_index < 20) {
    //direct
    for(j = 0; j < 20; j++)
      if(j == block_index)
        return fp->direct[j];
  } else if(block_index >= 20 && block_index < 20 + 8 * 128) {
    int idx = block_index / 128;
    int idx2 = block_index % 128;
    for(j = 0; j < 8; j++)
      if(j == idx && fp->indirect_1[j] != -1) {
        indirect_t indir, *in;
        in = &indir;
        block_read(effective_data_block_number(fp->indirect_1[j]), in);
        for(k = 0; k < 128; k++)
          if(k == idx2) {
            return in->d_block[k];
          }
      }
  } else if(block_index >= 20 + 8 * 128 && block_index < max_blocks) {
    //in_2
    log_msg("\n Not implemented yet \n");
  }
  return -1;
}

int set_block_index_to_direct_indirect_block_number(inode_t *fp, int block_index, int value) {
  int j, k, block_number = -1;
  if(block_index < 20) {
    //direct
    for(j = 0; j < 20; j++)
      if(j == block_index) {
        fp->direct[j] = value;
        break;
      }
  } else if(block_index >= 20 && block_index < 20 + 8 * 128) {
    int idx = block_index / 128;
    int idx2 = block_index % 128;
    for(j = 0; j < 8; j++)
      if(j == idx && fp->indirect_1[j] != -1) {
        indirect_t indir, *in;
        in = &indir;
        block_read(effective_data_block_number(fp->indirect_1[j]), in);
        for(k = 0; k < 128; k++)
          if(k == idx2) {
            in->d_block[k] = value;
            break;
          }
        block_write(effective_data_block_number(fp->indirect_1[j]), in);
      }
  } else if(block_index >= 20 + 8 * 128 && block_index < max_blocks) {
    //in_2
    log_msg("\n Not implemented yet \n");
  }
  return -1;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  int retstat = 0;
  log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
          path, buf, size, offset, fi);

  log_msg("\nSize: %d\n",size);
  if(fi->fh != 1) {
    log_msg("\n File not open \n");
    return -EIO;
  }

  int file_inode = find_block_number(path);

  if(file_inode == -1) {
    log_msg("\n File not found \n");
    return -ENOENT;
  }

  inode_t fil, *fp;
  fp = &fil;

  block_read(effective_data_block_number(file_inode), fp);

  int i, j = 0, retval;
  size_t bytes_read = 0;

  size_t actual_size = fp->size;

  if(actual_size < size)
    size = actual_size;

  int no_of_blocks = size / BLOCK_SIZE;
  if(size % BLOCK_SIZE != 0)
    no_of_blocks++;

  char out_buf[BLOCK_SIZE * no_of_blocks + 5], tmp_buf[BLOCK_SIZE], *bptr;
  bptr = tmp_buf;

  int start_block, start_addr;

  if(offset > size) {
    log_msg("\n Invalid offset \n");
    return -EIO;
  }

  start_block = offset / BLOCK_SIZE;
  start_addr = offset % BLOCK_SIZE;

  //while writing write only the number of bytes that was in the buffer, not 512 bytes.
  //Include a eof character at end of buffer. Use this while reading.
  //Increase number of block references in file

  strncpy(out_buf, "START", 5);

  for(i=start_block; i < no_of_blocks; i++) {
    retval = block_read(effective_data_block_number(block_index_to_direct_indirect_block_number(fp, i)), bptr);
    //log_msg("\nRead: %s\n", bptr);
    if(retval < 0)
      return -EIO;
    strncat(out_buf, bptr, BLOCK_SIZE);
    bytes_read += retval;
  }

  strncpy(buf, out_buf + 5 + start_addr, size);
  bytes_read -= start_addr;

  // log_msg("\nFull string: %s\n", buf);

  fp->date_accessed = timestamp();
  //Update file accessed time
  block_write(effective_data_block_number(file_inode), fp);

  retstat = bytes_read;
  log_msg("\n Return Read: %d \n", retstat);
  return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */

int write_file_data_block(inode_t *fp, int block_number, const char *buf, int block_index, int is_new_block, int inode_number, int start_block, int seek_bytes, size_t size) {
  int written = 0, retval;
  int diff = (block_index - start_block) * BLOCK_SIZE - seek_bytes;

  if(block_index == start_block && seek_bytes != 0) {
    char wr_buf[BLOCK_SIZE], *wrbuf;
    wrbuf = wr_buf;
    if(is_new_block == 1)
      memset(wrbuf, 0, BLOCK_SIZE);
    else
      block_read(effective_data_block_number(block_number), wrbuf);
    wrbuf += seek_bytes;
    strncpy(wrbuf, buf, BLOCK_SIZE - seek_bytes);
    if (seek_bytes + size < BLOCK_SIZE) {
         wrbuf[seek_bytes + size] = EOF;
    }
    retval = block_write(effective_data_block_number(block_number), wrbuf);
    fp->size += retval - seek_bytes;
    written += retval - seek_bytes;
  } else {
    char wrbuf[BLOCK_SIZE];
    strncpy(wrbuf , buf+diff , BLOCK_SIZE);
    if ((size - diff) < BLOCK_SIZE) {
        wrbuf[size-diff] = EOF;
    }
    retval = block_write(effective_data_block_number(block_number), wrbuf);
    if((size - diff) < BLOCK_SIZE) {
       written += size-diff;
       fp->size += size-diff;
    } else {
       written += retval;
       fp->size += retval;
    }
  }

  if(retval < 0)
    return -errno;

  fp->date_modified = timestamp();

  if(is_new_block == 1) {
    set_block_index_to_direct_indirect_block_number(fp, block_index, block_number);
    fat->entry[block_number].in_use = 1;
    fp->number_of_blocks += 1;
    write_fat_block(block_number);
  }

  retval = block_write(effective_data_block_number(inode_number), fp);

  if(retval < 0)
    return -errno;

  return written;
}


int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
  int retstat = 0;
  log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
          path, buf, size, offset, fi);

  if(fi->fh != 1) {
    log_msg("\n File not open \n");
    return -EIO;
  }

  int file_inode = find_block_number(path);

  if(file_inode == -1) {
    log_msg("\n File not found \n");
    return -ENOENT;
  }

  inode_t fil, *fp;
  fp = &fil;

  block_read(effective_data_block_number(file_inode), fp);

  long total_size = (size + offset);

  int no_blocks = total_size / BLOCK_SIZE;
  if(total_size % BLOCK_SIZE != 0)
    no_blocks++;

  if(no_blocks > max_blocks) {
    log_msg("\n File size exceeds %d \n", max_blocks * BLOCK_SIZE);
    return -ENOMEM;
  }

  int i, j = 0, retval, new_block;
  size_t written = 0;

  int start_block = offset/BLOCK_SIZE;
  int seek_bytes = offset % BLOCK_SIZE;

  for(i = start_block; i < no_blocks && i < max_blocks; i++) {
    retval = 0;
    if(i < 20) {
      //direct
      if(fp->direct[i] == -1) {
        new_block = get_empty_block();
        if(new_block == -1)
          return -ENOMEM;
        retval = write_file_data_block(fp, new_block, buf, i, 1, file_inode, start_block, seek_bytes, size);
        if(retval < 0)
          return retval;
        written += retval;
      } else {
        retval = write_file_data_block(fp, fp->direct[i], buf, i, 0, file_inode, start_block, seek_bytes, size);
        if(retval < 0)
          return retval;
        written += retval;
      }
    } else if(i >= 20 && i < 20 + 8 * 128) {
      //in_1
      int indir_no = i / 128;
      if(fp->indirect_1[indir_no] == -1) {
        indirect_t *in, indir;
        in = &indir;
        init_indir(in);
        int indir_block = get_empty_block();
        if(indir_block == -1)
          return -ENOMEM;
        block_write(effective_data_block_number(indir_block), in);
        fp->indirect_1[indir_no] = indir_block;
        fat->entry[indir_block].in_use = 1;
        write_fat_block(indir_block);
        block_write(effective_data_block_number(file_inode), fp);
      }

      int block_number = block_index_to_direct_indirect_block_number(fp, i);
      if(block_number == -1) {
        new_block = get_empty_block();
        if(new_block == -1)
          return -ENOMEM;
        retval = write_file_data_block(fp, new_block, buf, i, 1, file_inode, start_block, seek_bytes, size);
        if(retval < 0)
          return retval;
        written += retval;
      } else {
        retval = write_file_data_block(fp, fp->direct[i], buf, i, 0, file_inode, start_block, seek_bytes, size);
        if(retval < 0)
          return retval;
        written += retval;
      }
    } else if(i >= 20 + 8 * 128 && i < max_blocks) {
      //in_2
      log_msg("\n Not Implemented yet \n");
    }
  }

  retstat = written;
  log_msg("\n Size: %d \n", size);
  log_msg("\n Return : %d \n", retstat);
  return retstat;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode) {
  int retstat = 0;
  log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
          path, mode);

  dirent_t new_node, *node;
  node = &new_node;

  dirent_t parent, *par_dir;
  par_dir = &parent;

  char **tokens = str_split(strdup(path+1), '/');
  char *name;
  char parent_path[MAX_PATH];

  int i;
  for(i = 0; *(tokens + i); i++) {
    name = *(tokens + i);
  }

  memset(parent_path, '\0', MAX_PATH);
  strncpy(parent_path, "/\0", MAX_PATH);

  strncat(parent_path, path + 1, strlen(path) - strlen(name) -1);

  // log_msg("\n parent_path %s : %d\n", parent_path, strlen(parent_path));

  if(parent_path == NULL || strcmp(parent_path, "") == 0) {
    log_msg("\n Attempted to create under %s \n", path);
    strncpy(parent_path, "/\0", MAX_PATH);
  }

  // log_msg("\nparent path: %s\n", parent_path);
  // log_msg("\ndirectory name: %s\n", name);

  int parent_block_num = find_block_number(parent_path);
  int retval = block_read(effective_data_block_number(parent_block_num), par_dir);

  // log_msg("\n Mode: %s \n", lsperms(mode));
  init_dirent(node, name, par_dir, mode | S_IFDIR, path, parent_block_num);

  // log_msg("\n Effective Path %s \n", node->path);

  int dir_block = get_empty_block();

  if(dir_block == -1)
    return -ENOMEM;

  int assoc_with_parent = 0, j;

  for(j = 0; j < 10; j++) {
    if(par_dir->direct[j] == -1) {
      par_dir->direct[j] = dir_block;
      assoc_with_parent = 1;
      break;
    }
  }

  if(assoc_with_parent == 0) {
    log_msg("\n Cannot add more files / dir into this directory\n");
    retstat = -ENOMEM;
  } else {
    int f_retval = block_write(effective_data_block_number(dir_block), node);
    int d_retval = block_write(effective_data_block_number(parent_block_num), par_dir);
    if(f_retval > 0 && d_retval > 0) {
      fat->entry[dir_block].in_use = 1;
      memset(bps->block_pointer[dir_block].name, '\0', MAX_NAME);
      strncpy(bps->block_pointer[dir_block].name, name, strlen(name));
      write_fat_block(dir_block);
      write_bps_block(dir_block);
      retstat = 0;

      if(parent_block_num == 0) {
        memcpy(root, par_dir, sizeof(dirent_t));
      }

      // log_msg("\n Written new directory \n");
    }
    else
      retstat = -ENOENT;
  }
  return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path) {
  int retstat = 0;
  log_msg("sfs_rmdir(path=\"%s\")\n", path);

  int data_block = find_block_number(path);

  if(data_block == -1) {
    log_msg("File Not found");
    return -ENOENT;
  }

  dirent_t parent, *dp;
  dirent_t dir, *p;
  p = &dir;
  dp = &parent;

  int freed = 0;

  block_read(effective_data_block_number(data_block), p);
  int parent_block = p->parent;

  //Update FAT table
  fat->entry[data_block].in_use = 0;
  freed++;
  //Reset BPS
  init_bps_entry(bps->block_pointer + data_block, '\0');
  //Remove parent reference to file
  block_read(effective_data_block_number(p->parent), dp);
  int i;

  for(i=0;i<10;i++) {
    if(dp->direct[i] == data_block) {
      dp->direct[i] = -1;
      //log_msg("\n Removed reference from parent directory \n ");
      break;
    }
  }

  inode_t fil, *fp;
  fp = &fil;
  dirent_t sub, *sd;
  sd = &sub;
  for(i=0; i<10; i++) {
    memset(fp, 0, sizeof(inode_t));
    memset(sd, 0, sizeof(dirent_t));
    if(p->direct[i] != -1) {
      //Remove files / directories in directory
      block_read(effective_data_block_number(p->direct[i]), fp);
      block_read(effective_data_block_number(p->direct[i]), sd);
      if(S_ISDIR(sd->mode)) {
        sfs_rmdir(sd->path);
      } else if(S_ISREG(fp->mode)) {
        sfs_unlink(fp->path);
      } else {
        log_msg("\n Found something else \n");
      }
    }
  }
  //Write back everything
  block_write(effective_data_block_number(parent_block), dp); //Parent
  write_fat_block(data_block);
  write_bps_block(data_block);
  if(p->parent == 0) {
    memcpy(root, dp, sizeof(dirent_t));
  }

  log_msg("\n Freed: %d \n", freed);

  return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi) {
  int retstat = 0;

  log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n",
          path, fi);

  fi->fh = 1;

  return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                struct fuse_file_info *fi) {
  log_msg("\nsfs_readdir(path: %s)\n", path);

  int retstat = 0, retval;

  //Read contents of directory
  int dir_block_num = find_block_number(path);

  if(dir_block_num == -1) {
    log_msg("\n Dir not found \n");
    return -1;
  }

  if(fat->entry[dir_block_num].in_use != 1) {
    log_msg("\n%d\n", fat->entry[dir_block_num].in_use);
    log_msg("\nBlock not in use\n");
    errno = ENOENT;
    return -1;
  }

  dirent_t directory_node, *dir_node, sub_dir, *s_dir;
  inode_t file_node, *fil_node;

  dir_node = &directory_node;
  fil_node = &file_node;
  s_dir = &sub_dir;

  retval = block_read(effective_data_block_number(dir_block_num), dir_node);

  if(S_ISDIR(dir_node->mode)) {
    int i;
    char name[MAX_NAME];
    struct stat statbuf_base;
    struct stat *statbuf = &statbuf_base;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for(i=0; i<10 ; i++) {
      if(dir_node->direct[i] != -1) {
        memset(statbuf, 0, sizeof(struct stat));
        memset(s_dir, 0, sizeof(dirent_t));
        memset(fil_node, 0, sizeof(inode_t));
        memset(name, '\0', MAX_NAME);
        block_read(effective_data_block_number(dir_node->direct[i]), s_dir);
        block_read(effective_data_block_number(dir_node->direct[i]), fil_node);
        if(S_ISDIR(s_dir->mode)) {
          set_directory_stats(statbuf, s_dir);
          strncpy(name, s_dir->name, strlen(s_dir->name));
        } else {
          set_file_stats(statbuf, fil_node);
          strncpy(name, fil_node->name, strlen(fil_node->name));
        }
        retval = filler(buf, name, statbuf, 0);
        // log_msg("\n Name: %s \n", name);
        // log_msg("\n retval: %d \n", retval);
      }
    }
  } else {
    log_msg("\n Not a directory \n");
    return -1;
  }

  return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi) {
  int retstat = 0;

  fi->fh = 0;

  return retstat;
}

struct fuse_operations sfs_oper = {
        .init = sfs_init, //done
        .destroy = sfs_destroy, //done

        .getattr = sfs_getattr, //done
        .utime = sfs_utime, //done
        .create = sfs_create, //done
        .unlink = sfs_unlink, //done
        .open = sfs_open, //done
        .release = sfs_release, //done
        .read = sfs_read, //done
        .write = sfs_write, //done

        .rmdir = sfs_rmdir, //done
        .mkdir = sfs_mkdir, //done

        .opendir = sfs_opendir, //done
        .readdir = sfs_readdir, //done
        .releasedir = sfs_releasedir //done
};

void sfs_usage() {
    fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
    abort();
}

int main(int argc, char *argv[]) {
    int fuse_stat;
    struct sfs_state *sfs_data;

    // sanity checking on the command line
    if ((argc < 3) || (argv[argc - 2][0] == '-') || (argv[argc - 1][0] == '-'))
        sfs_usage();

    sfs_data = malloc(sizeof(struct sfs_state));
    if (sfs_data == NULL) {
        perror("main calloc");
        abort();
    }

    // Pull the diskfile and save it in internal data
    sfs_data->diskfile = argv[argc - 2];
    argv[argc - 2] = argv[argc - 1];
    argv[argc - 1] = NULL;
    argc--;

    sfs_data->logfile = log_open();

    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
    fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);

    return fuse_stat;
}
