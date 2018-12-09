/****** util.c *****/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

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
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

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


int get_block(int dev, int blk, char *buf)
{
	lseek(dev, blk*BLKSIZE, 0);
	return (read(dev, buf, BLKSIZE) < 0) ? 0 : 1;
}   

int put_block(int dev, int blk, char *buf)
{
	lseek(dev, blk*BLKSIZE, 0);
	return (write(dev, buf, BLKSIZE) != BLKSIZE) ? 0 : 1;
}   

/*
 * Gets an indirect block at the given offset
 * Returns 0 if no block exists
 */
int get_indirect(int dev, int i_blk, int blk, char *buf)
{
	char i_buf[BLKSIZE];
	int i;

	get_block(dev, i_blk, i_buf);
	if(!(i = (int)i_buf[blk * 4]))
		return 0;
	get_block(dev, i, buf);

	return i;
}

/*
 * Gets a double indirect block at the given offset
 * Returns 0 if no block exists
 */
int get_double_indirect(int dev, int di_blk, int i_blk, int blk, char *buf)
{
	char i_buf[BLKSIZE], di_buf[BLKSIZE];
	int i;

	get_block(dev, di_blk, di_buf);
	if(!(i = (int)di_buf[i_blk * 4]))
		return 0;
	get_block(dev, i, i_buf);
	if(!(i = (int)i_buf[blk * 4]))
		return 0;
	get_block(dev, i, buf);

	return i;
}

/*
 * Gets a triple indirect block at the given offset
 * Returns 0 if no block exists
 */
int get_triple_indirect(int dev, int ti_blk, int di_blk, int i_blk, int blk, char *buf)
{
	char i_buf[BLKSIZE], di_buf[BLKSIZE], ti_buf[BLKSIZE];
	int i;

	get_block(dev, ti_blk, ti_buf);
	if(!(i = (int)ti_buf[di_blk * 4]))
		return 0;
	get_block(dev, i, di_buf);
	if(!(i = (int)di_buf[i_blk * 4]))
		return 0;
	get_block(dev, i, i_buf);
	if(!(i = (int)i_buf[blk * 4]))
		return 0;
	get_block(dev, i, buf);

	return i;
}

int tokenize(char *pathname)
{
	int i=0;
	char *temp;
  // tokenize pathname into n components: name[0] to name[n-1];
	if(pathname[0] == '/')
		name[i++] = "/";

	temp = strtok(pathname, "/");
	while(temp)
	{
		name[i++] = temp;
		temp = strtok(0, "/");
	}

	return i;
}


MINODE *iget(int dev, int ino)
{
	MINODE *mip = 0;
	INODE *ip;
	int i = 0, blk, offset;
	char buf[BLKSIZE];
  // return minode pointer to loaded INODE
	for(i = 0; i < NMINODE; i++)
	{
		if(minode[i].refCount > 0 && (minode[i].dev == dev && minode[i].ino == ino))
		{
			minode[i].refCount++;
			return &minode[i];
		}
	}

  // needed entry not in memory:
	for(i = 0; i < NMINODE && mip == 0; i++)
	{
		if(minode[i].refCount == 0)
		{
			mip = &minode[i];
			mip->refCount = 1;
			mip->dev = dev;
			mip->ino = ino;
		}
	}

    // get INODE of ino a char buf[BLKSIZE]    
    blk    = (ino-1) / 8 + inode_start;
    offset = (ino-1) % 8;

//  printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

    get_block(dev, blk, buf);
    ip = (INODE *)buf + offset;
    mip->INODE = *ip;  // copy INODE to mp->INODE

    return mip;
}


int iput(MINODE *mip) // dispose a used minode by mip
{
	int blk, offset;
	char buf[BLKSIZE];
	INODE *ip;

	mip->refCount--;
 
	if (mip->refCount > 0) 
		return;
	if (!mip->dirty)       
		return;
 
	// Write YOUR CODE to write mip->INODE back to disk
	blk = (mip->ino - 1) / 8 + inode_start;
	offset = (mip->ino - 1) % 8;
	
	get_block(dev, blk, buf);
	ip = (INODE *)buf + offset;
	*ip = mip->INODE; // Write mip->INODE to correct point of buf
	put_block(dev, blk, buf);
} 

int search_block(char *buf, char *name)
{
	char temp[256];
	DIR *dp;
	char *cp;
	
	dp = (DIR *)buf;
	cp = buf;

	while (cp < buf + BLKSIZE)
	{
		strncpy(temp, dp->name, dp->name_len);
	    temp[dp->name_len] = 0;

		if(!strcmp(temp, name))
			return dp->inode;
	    cp += dp->rec_len;
		dp = (DIR *)cp;	   
	}

	return 0;
}

/*
 * Search a Directory MINODE for an entry with the given name
 * returns ino if found, 0 otherwise
 */
int search(MINODE *mip, char *name)
{
	int i, j, k, ino, max = 256;
	char temp[256], buf[BLKSIZE];

	// Search 12 direct blocks
	for (i=0; i < 12; i++)
	{  
		if (mip->INODE.i_block[i] == 0)
			break;
		get_block(dev, mip->INODE.i_block[i], buf);

		if (ino = search_block(buf, name))
			return ino;
	}

	if(!mip->INODE.i_block[12])
		return 0;

	// Search indirect block
	for (i = 0; i < max; i++)
	{
		if(get_indirect(dev, mip->INODE.i_block[12], i, buf))
		{
			if(ino = search_block(buf, name))
				return ino;
		}
		else
			return 0;
	}

	if(!mip->INODE.i_block[13])
		return 0;

	// Search double indirect block
	for(i = 0; i < max; i++)
	{
		for(j = 0; j < max; j++)
		{
			if(get_double_indirect(dev, mip->INODE.i_block[13], i, j, buf))
			{
				if(ino = search_block(buf, name))
					return ino;
			}
			else
				return 0;
		}
	}

	if(!mip->INODE.i_block[14])
		return 0;
		
	// Search triple indirect block
	for(i = 0; i < max; i++)
	{
		for(j = 0; j < max; j++)
		{
			for(k = 0; k < max; k++)
			{
				if(get_triple_indirect(dev, mip->INODE.i_block[14], i, j, k, buf))
				{
					if(ino = search_block(buf, name))
						return ino;
				}
				else
					return 0;
			}
		}
	}

	return 0;
}

/*
 * returns the ino of the file at pathname, or 0 if not found
 */
int getino(char *pathname)
{ 
	char temp[256];
	MINODE *mip;
	int n, i, ino;

	strcpy(temp, pathname);
	n = tokenize(temp);

	if(!strcmp(name[0], "/"))
	{
		i = 1;
		mip = iget(dev, root->ino);
	}
	else
	{
		i = 0;
		mip = iget(dev, running->cwd->ino);
	}

	// In case there isn't any searching to be done
	ino = mip->ino;

	while(i < n)
	{
		if(!S_ISDIR(mip->INODE.i_mode))
		{
			printf("%s is not a directory!", name[i - 1]);
			return 0;
		}
		ino = search(mip, name[i]);
		if(!ino)
		{
			printf("%s not found\n", name[i]);
			return 0;
		}
		
		iput(mip);
		mip = iget(dev, ino);
		i++;
	}
	
	return ino;
}



// THESE two functions are for pwd(running->cwd), which prints the absolute
// pathname of CWD. 
int findmyname_block(char *buf, int ino, char *name)
{
	char temp[256];
	DIR *dp;
	char *cp;
	
	dp = (DIR *)buf;
	cp = buf;

	while (cp < buf + BLKSIZE)
	{
		strncpy(temp, dp->name, dp->name_len);
	    temp[dp->name_len] = 0;

		if(dp->inode == ino)
		{
			strcpy(name, temp);
			return dp->inode;
		}

	    cp += dp->rec_len;
		dp = (DIR *)cp;	   
	}

	return 0;
}

int findmyname(MINODE *parent, u32 myino, char *myname) 
{
	MINODE *mip = parent;
	int i, j, k, ino, max = 256;
	char temp[256], buf[BLKSIZE];

	// Search 12 direct blocks
	for (i=0; i < 12; i++)
	{  
		if (mip->INODE.i_block[i] == 0)
			break;
		get_block(dev, mip->INODE.i_block[i], buf);
		if (ino = findmyname_block(buf, myino, myname))
			return ino;
	}

	if(!mip->INODE.i_block[12])
		return 0;

	// Search indirect block
	for (i = 0; i < max; i++)
	{
		if(get_indirect(dev, mip->INODE.i_block[12], i, buf))
		{
			if(ino = findmyname_block(buf,  myino, myname))
				return ino;
		}
		else
			return 0;
	}

	if(!mip->INODE.i_block[13])
		return 0;

	// Search double indirect block
	for(i = 0; i < max; i++)
	{
		for(j = 0; j < max; j++)
		{
			if(get_double_indirect(dev, mip->INODE.i_block[13], i, j, buf))
			{
				if(ino = findmyname_block(buf, myino, myname))
					return ino;
			}
			else
				return 0;
		}
	}

	if(!mip->INODE.i_block[14])
		return 0;
		
	// Search triple indirect block
	for(i = 0; i < max; i++)
	{
		for(j = 0; j < max; j++)
		{
			for(k = 0; k < max; k++)
			{
				if(get_triple_indirect(dev, mip->INODE.i_block[14], i, j, k, buf))
				{
					if(ino = findmyname_block(buf, mip->INODE.i_block[i], myname))
						return ino;
				}
				else
					return 0;
			}
		}
	}

	return 0;
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

int decFreeBlocks(int dev)
{
  char buf[BLKSIZE];

  // dec free blocks count in SUPER and GD
  get_block(dev, 1, buf);
  ((SUPER*)buf)->s_free_blocks_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  ((GD*)buf)->bg_free_blocks_count--;
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
            decFreeBlocks(dev);
            put_block(dev, bmap, buf);
            
            return i+1;
        }
        i+=1;
    }
    printf("no more free blocks\n");
    return 0;
}

int incFreeInodes(int dev)
{
	char buf[BLKSIZE];

	// increment free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	((SUPER*)buf)->s_free_inodes_count++;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	((GD*)buf)->bg_free_inodes_count++;
	put_block(dev, 2, buf);
}

int incFreeBlocks(int dev)
{
	char buf[BLKSIZE];

	// increment free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	((SUPER*)buf)->s_free_blocks_count++;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	((GD*)buf)->bg_free_blocks_count++;
	put_block(dev, 2, buf);
}

int idealloc(int dev, int ino)
{
	char buf[BLKSIZE];

	// read inode_bitmap block
	get_block(dev, imap, buf);

	clr_bit(buf, ino - 1);
	incFreeInodes(dev);

	put_block(dev, imap, buf);

	return 1;
}

int bdealloc(int dev, int bno)
{
	char buf[BLKSIZE];

	// read _bitmap block
	get_block(dev, bmap, buf);

	clr_bit(buf, bno - 1);
	incFreeBlocks(dev);

	put_block(dev, bmap, buf);

	return 1;
}

void truncate(MINODE *mip)
{
	int i, j, k, max = 256;
	char buf[BLKSIZE], ibuf[BLKSIZE];
	// Deallocate 12 direct blocks
	for (i=0; i<12; i++)
	{
		if (mip->INODE.i_block[i]==0)
			continue;
		bdealloc(mip->dev, mip->INODE.i_block[i]);
	}
	// deallocate indirect blocks
    if(mip->INODE.i_block[12] != 0)
    {
        get_block(mip->dev, mip->INODE.i_block[12], (char*)buf);
        for(int i = 0; i < 256; i++)
		{
			if (buf[i * 4] == 0)
				continue;
			bdealloc(mip->dev, buf[i * 4]);
		}
    }
	// deallocate double indirect blocks
    if(mip->INODE.i_block[13] != 0)
    {
        get_block(mip->dev, mip->INODE.i_block[13], ibuf);
        for(int i = 0; i < 256; i++)
        {
			if(ibuf[i * 4] == 0)
				continue;
            get_block(mip->dev, ibuf[i * 4], buf);
            for(int j = 0; j < 256; j++)
            {
				if (buf[j * 4] == 0)
					continue;
				bdealloc(mip->dev, buf[j * 4]);
            }
        }
	}
}

void dir_base_name(char *path)
{
    char temp[256];
    strcpy(temp, path);
    strcpy(dname, dirname(temp));
    strcpy(temp, path);
    strcpy(bname, basename(temp));
}
