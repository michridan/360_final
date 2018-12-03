/********* showblock.c code ********/
/*
/
/lost+found
/X/
/Y/
/Z/
/X/tiny
/Y/bigfile
/Z/hugefile
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

#define BLKSIZE 1024

GD    *gp;
SUPER *sp;
INODE *ip;
char dbuf[BLKSIZE];
char buf[BLKSIZE];
DIR *dp;
char *cp;
int dev;

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

int search(INODE *inp, char *target)
{
	int i;
	char temp[256];

	for (i=0; i < 12; i++)
	{  // assume at most 12 direct blocks
		if (inp->i_block[i] == 0)
			break;
		get_block(dev, inp->i_block[i], dbuf);
	    dp = (DIR *)dbuf;
		cp = dbuf;

	    while (cp < dbuf + BLKSIZE)
		{
		   strncpy(temp, dp->name, dp->name_len);
	       temp[dp->name_len] = 0;

		   printf("Checking %s\n", temp);
		   if(!strcmp(temp, target))
				return dp->inode;
	       cp += dp->rec_len;
		   dp = (DIR *)cp;
		   
	    }
	}
	return 0;
}

int showblock(char *path[], int n)
{
    int ino, blk, offset, i;
    int iblk;
    char ibuf[BLKSIZE];
	char *cp2;
	DIR *dp2;

	get_block(dev, 2, buf);
	gp = (GD *)buf;

	iblk = gp->bg_inode_table;
	
	get_block(dev, iblk, buf);
	ip = (INODE *)buf + 1;

    for (i=0; i < n; i++)
    {
        ino = search(ip, path[i]);
        if (ino==0)
		{
           printf("can't find %s\n", path[i]); exit(1);
        }
        // Mailman's algorithm: Convert (dev, ino) to inode pointer
        blk    = (ino - 1) / 8 + iblk;  // disk block contain this INODE 
        offset = (ino - 1) % 8;         // offset of INODE in this block
        get_block(dev, blk, buf);
        ip = (INODE *)buf + offset;    // ip -> new INODE
    }

	printf("Direct Block Numbers:\n");
	for(i = 0; i < 12; i++)
	{
		if(ip->i_block[i] == 0)
		{
			putchar('\n');
			return 0;
		}
		printf("%d, ", ip->i_block[i]);
	}
	putchar('\n');
	char dbuf[1024];
	printf("Indirect Block Numbers:\n");
	if(ip->i_block[12] == 0)
	{
		putchar('\n');
		return 0;
	}

	get_block(dev, ip->i_block[12], dbuf);
	cp = dbuf;
	dp = (DIR *)dbuf;

	while(cp < dbuf + BLKSIZE)
	{
		unsigned int val = *((unsigned int*)cp);
		if(val == 0)
			break;
		printf("%d, ", val);
		cp += 4;
	}
	putchar('\n');

	printf("Double Indirect Block Numbers:\n");
	if(ip->i_block[13] == 0)
	{
		putchar('\n');
		return 0;
	}

	get_block(dev, ip->i_block[13], buf);
	cp = buf;

	while(cp < dbuf + BLKSIZE)
	{
		unsigned int val = *((unsigned int*)cp);
		if(val == 0)
			break;
		get_block(dev, val, dbuf);
		
		cp2 = dbuf;
		while(cp2 < dbuf + BLKSIZE)
		{
			unsigned int val2 = *((unsigned int*)cp2);
			if(val2 == 0)
				break;
			printf("%d, ", val2);
			cp2 += 4;
		}
		cp += 4;
	}
	putchar('\n');

	return 0;
}

char *disk = "diskimage";
char *name = "/";

int main(int argc, char *argv[ ])
{ 
	char temp[1024], *path[64];
	int i = 0;
	if (argc > 2)
	{
		disk = argv[1];
		name = argv[2];
	}
	else if (argc > 1)
		name = argv[1];

	dev = open(disk, O_RDONLY);
	if (dev < 0){
		printf("open failed\n");
		exit(1);
	}

	printf("tokenizing path...\n");
	strcpy(temp, name);
	path[i] = strtok(temp, "/");
	printf("%s ", path[i]);
	for(i = 1; path[i] = strtok(0, "/"); i++)
		printf("%s ", path[i]);

	putchar('\n');
	showblock(path, i);
}
