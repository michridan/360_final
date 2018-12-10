#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"

/**** globals defined in main.c file ****/

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256], bname[256], dname[256];

int tst_bit(char *buf, int bit);
int set_bit(char *buf, int bit);
int clr_bit(char *buf, int bit);
int get_block(int dev, int blk, char *buf);
int put_block(int dev, int blk, char *buf);
int get_indirect(int dev, int i_blk, int blk, char *buf);
int get_double_indirect(int dev, int di_blk, int i_blk, int blk, char *buf);
int get_triple_indirect(int dev, int ti_blk, int di_blk, int i_blk, int blk, char *buf);
int tokenize(char *pathname);
MINODE *iget(int dev, int ino);
int iput(MINODE *mip);
int search_block(char *buf, char *name);
int search(MINODE *mip, char *name);
int getino(char *pathname);
int findmyname_block(char *buf, int ino, char *name);
int findmyname(MINODE *parent, u32 myino, char *myname);
int decFreeInodes(int dev);
int decFreeblocks(int dev);
int ialloc(int dev);
int balloc(int dev);
int incFreeInodes(int dev);
int incFreeBlocks(int dev);
int idealloc(int dev, int ino);
int bdealloc(int dev, int bno);
void truncate(MINODE *mip);
void dir_base_name(char *path);

#endif