
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include "util.h"
// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

#define BLKSIZE 1024

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

/********** globals *************/
int fd;
int imap, bmap;  // IMAP and BMAP block number
int ninodes, nblocks, nfreeInodes, nfreeBlocks;
/*
int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

int put_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  write(fd, buf, BLKSIZE);
}
*/
int tst_bit(char *buf, int bit)
{
    return buf[bit/8] & (1 << (bit%8));
    /*
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
  */
}

int set_bit(char *buf, int bit)
{
     buf[bit/8] |= (1 << (bit%8));
    /*
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
    */
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

int decFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  ((SUPER*)buf)->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  //gp = (GD *)buf;
  //gp->bg_free_inodes_count--;
  ((GD*)buf)->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int ialloc(int dev)
{

  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);
    printf("HERE\n");
  for (int i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
        printf("HERE v2\n");
       set_bit(buf,i);
       printf("HERE v3\n");
       decFreeInodes(dev);
    printf("HERE v4\n");
       put_block(dev, imap, buf);
        printf("HERE v5\n");
       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
}

int balloc(int dev)
{
    int i = 0;
    char buf[BLKSIZE];
    get_block(dev, bmap, buf);
    while(i < nblocks)
    {
        if(tst_bit(buf, i)==0)
        {
            set_bit(buf, i);
            decFreeInodes(dev);
            put_block(dev, bmap, buf);
            
            return i+1;
        }
        i+=1;
    }
    printf("no more free blocks\n");
    return 0;
}



