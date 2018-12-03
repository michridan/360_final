#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

#define BLKSIZE 1024

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

int fd;
int iblock;

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd,(long)blk*BLKSIZE, 0);
   read(fd, buf, BLKSIZE);
}

int search(INODE *ip, char *name)
{
	char buf[BLKSIZE], temp[EXT2_NAME_LEN];
	unsigned int block0 = ip->i_block[0];
    get_block(fd, block0, buf);
    char* cp = buf;
    dp = (DIR*)buf;

    while(cp - buf < BLKSIZE)
    {
		temp[0] = 0;
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
        if(strcmp(name, temp) == 0)
            return dp->inode;
        cp += dp->rec_len;
        dp = (DIR*)cp;
    }
    return 0;
	
}

int dir()
{
	char buf[BLKSIZE], temp[EXT2_NAME_LEN];
	int block0 = ip->i_block[0];
    get_block(fd, block0, buf);
    char* cp = buf;
    dp = (DIR*)buf;

    while(cp - buf < BLKSIZE)
    {
		temp[0] = 0;
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
		printf("%s\n", temp);
        cp += dp->rec_len;
        dp = (DIR*)cp;
    }
}

char *disk = "mydisk";
int main(int argc, char *argv[])
{ 
  if (argc > 1)
    disk = argv[1];

  fd = open(disk, O_RDONLY);
  if (fd < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dir();
}
