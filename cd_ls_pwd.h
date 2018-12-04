/******** cd_ls_pwd.h ***********/

#ifndef CD_LS_PWD_H
#define CD_LS_PWD_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"
#include "util.h"

/**** globals defined in main.c file ****/

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk, inode_start;
extern char line[256], cmd[32], pathname[256];

int ls_file(int ino);
int ls_dir(char *dirname);
int ls(char *pathname);
int mychdir(char *pathname);
int rpwd(MINODE *wd);
int pwd(MINODE *wd);

#endif
