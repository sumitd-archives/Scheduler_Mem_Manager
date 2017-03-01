#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include "log.h"
#include <ctype.h>

//Structures
struct sfs_dirent {
  int owner_uid;
  int owner_gid;
  mode_t mode;
  time_t date_created;
  time_t date_modified;
  char name[MAX_NAME];
  int direct[10];
  char path[MAX_PATH];
  int parent;
  char random[148]; //->512
};

typedef struct sfs_dirent dirent_t;

struct indirect_pointer {
  int d_block[128];
};

typedef struct indirect_pointer indirect_t;

struct i_node {
  //4  -> 4
  int owner_uid;
  //4  -> 8
  int owner_gid;
  //4  -> 12
  mode_t mode;
  //8  -> 20
  time_t date_created;
  //8  -> 28
  time_t date_modified;
  //8  -> 36
  time_t date_accessed;
  //4  -> 40
  int number_of_blocks;
  //4*20  -> 120
  int direct[20];
  //8  -> 128
  int parent; //Parent dirent_t block number
  //4  -> 132
  int links;
  //256 -> 388
  char path[MAX_PATH];
  //32 -> 420
  char name[MAX_NAME];
  //8*4 -> 452 - 1 level
  int indirect_1[8];
  //8*4 -> 484 - 2 level
  int indirect_2[8];
  int open;
  //TO use later
  int size;
  char random[16]; //->512
};

typedef struct i_node inode_t;

//Methods
//Generate timestamp for access,create,modify
long timestamp() {
  return time(NULL);
}

//Initialize dirent_t
void init_dirent(dirent_t *node, char* name, dirent_t *parent, mode_t mode, const char *path, int parent_block_number) {
  node->owner_uid = 1000; //use current user values
  node->owner_gid = 1000;

  if(mode == -1)
    node->mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IFDIR;
  else
    node->mode = mode;

  node->date_created = timestamp();
  node->date_modified = timestamp();

  memset(node->name, '\0', MAX_NAME);
  strncpy(node->name, name, strlen(name));

  memset(node->path, '\0', MAX_PATH);
  strncpy(node->path, path, strlen(path));

  int i;
  for(i=0;i<10;i++)
    node->direct[i] = -1;

  node->parent = parent_block_number;
}

//Initialize Inode_t
void init_inode(inode_t *inode, dirent_t *parent, int parent_block_number, char* filename, const char *path) {
  inode->owner_uid = 1000;
  inode->owner_gid = 1000;
  inode->mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IFREG;
  inode->date_created = timestamp();
  inode->date_modified = timestamp();
  inode->date_accessed = timestamp();
  inode->number_of_blocks = 1;
  inode->parent = parent_block_number;
  inode->links = 0; //Number of linked nodes
  inode->open = 0;
  inode->size = 0;

  memset(inode->path, '\0', MAX_PATH);
  strncpy(inode->path, path, strlen(path));

  memset(inode->name, '\0', MAX_NAME);
  strncpy(inode->name, filename, strlen(filename));

  //Direct data blocks
  int i;
  for(i = 0; i < 20; i++)
    inode->direct[i] = -1;

  for(i=0; i < 8; i++) {
    inode->indirect_1[i] = -1;
    inode->indirect_2[i] = -1;
  }
}

void init_indir(indirect_t *in) {
  int i;
  for(i = 0; i < 128; i++)
    in->d_block[i] = -1;
}

//Set statbuf for directory
void set_directory_stats(struct stat *statbuf, dirent_t* directory_node) {
  //statbuf->st_ino = directory_node->number;
  statbuf->st_mode = directory_node->mode;
  //statbuf->st_nlink = directory_node->links;
  statbuf->st_uid = directory_node->owner_uid;
  statbuf->st_gid = directory_node->owner_gid;
  statbuf->st_size = BLOCK_SIZE;
  //statbuf->st_atime = directory_node->date_accessed;
  statbuf->st_mtime = directory_node->date_modified;
  statbuf->st_ctime = directory_node->date_created;
  statbuf->st_blksize = BLOCK_SIZE;
  //statbuf->st_blocks = directory_node->number_of_blocks;
}

//Set statbuf for file
void set_file_stats(struct stat *statbuf, inode_t* file_node) {
  // statbuf->st_ino = file_node->number;
  statbuf->st_mode = file_node->mode;
  statbuf->st_nlink = file_node->links;
  statbuf->st_uid = file_node->owner_uid;
  statbuf->st_gid = file_node->owner_gid;
  statbuf->st_size = file_node->size;
  statbuf->st_atime = file_node->date_accessed;
  statbuf->st_mtime = file_node->date_modified;
  statbuf->st_ctime = file_node->date_created;
  statbuf->st_blksize = BLOCK_SIZE;
  statbuf->st_blocks = file_node->number_of_blocks;
}

//Read file mode
int filetypeletter(int mode)
{
    char    c;

    if (S_ISREG(mode))
        c = '-';
    else if (S_ISDIR(mode))
        c = 'd';
    else if (S_ISBLK(mode))
        c = 'b';
    else if (S_ISCHR(mode))
        c = 'c';
#ifdef S_ISFIFO
    else if (S_ISFIFO(mode))
        c = 'p';
#endif  /* S_ISFIFO */
#ifdef S_ISLNK
    else if (S_ISLNK(mode))
        c = 'l';
#endif  /* S_ISLNK */
#ifdef S_ISSOCK
    else if (S_ISSOCK(mode))
        c = 's';
#endif  /* S_ISSOCK */
#ifdef S_ISDOOR
    /* Solaris 2.6, etc. */
    else if (S_ISDOOR(mode))
        c = 'D';
#endif  /* S_ISDOOR */
    else
    {
        /* Unknown type -- possibly a regular file? */
        c = '?';
    }
    log_msg("\n Mode: %c \n", c);
    return(c);
}

static const char *rwx[] = {"---", "--x", "-w-", "-wx",
    "r--", "r-x", "rw-", "rwx"};

//Generate file perm string
/* Convert a mode field into "ls -l" type perms field. */
char* lsperms(int mode)
{
    static char bits[11];

    bits[0] = filetypeletter(mode);
    strcpy(&bits[1], rwx[(mode >> 6)& 7]);
    strcpy(&bits[4], rwx[(mode >> 3)& 7]);
    strcpy(&bits[7], rwx[(mode & 7)]);
    if (mode & S_ISUID)
        bits[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID)
        bits[6] = (mode & S_IXGRP) ? 's' : 'l';
    if (mode & S_ISVTX)
        bits[9] = (mode & S_IXUSR) ? 't' : 'T';
    bits[10] = '\0';
    return(bits);
}

int get_empty_block();
int block_index_to_direct_indirect_block_number(inode_t *fp, int block_index);
int set_block_index_to_direct_indirect_block_number(inode_t *fp, int block_index, int value);
int write_file_data_block(inode_t *fp, int block_number, const char *buf, int block_index, int is_new_block, int inode_number, int start_block, int seek_bytes, size_t size);
