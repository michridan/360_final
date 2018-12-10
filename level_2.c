#include "level_2.h"

/**** globals defined in main_1.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256], dname[256], bname[256], destname[256];


///*** Level 2 functions ///***

int open_file(int mode)
{
    int i = 0;
    //gets ino for file to opne


    int ino = getino(pathname);
    if(!ino)
    {
        printf("error: file %s does not exist\n");
		return -1;
    }
    MINODE * mip = iget(dev, ino);
    if(!S_ISREG(mip->INODE.i_mode))
    {
        printf("error: %s is not a regular file\n")
        return -1;
    }
    while (i < NFD)
    {
        if(running.fd[i].mptr == mip && running.fd[i].mode != 0)
        {
            //Already open but not for read
            printf("error: file %s is already open\n", pathname);
            return -1;
        }
        i+=1;
    }
    i = 0;
    while(i < NFD)
    {
        if(running.fd[i].refCount == 0)
        {
            // allocate a FREE OpenFileTable (OFT) and fill in values
            running.fd[i] = (OFT){.mode = mode, .refCount = 1, .mptr = mip, .offset = (mode == 3 ? mip->INODE.i_size : 0)};
            if(mode == 1)
            {
                // W: truncate file to 0 size
                truncate(mip);
            }
            int n = 0;
            while(n < 10)
            {
                if(!running->fd[n])
                {
                    //get to first null fd
                    break;
                    n+=1;
                }
            }
            //update INODE's time field
            time_t t = time(0L);
            //for R: touch atime. 
            mip->INODE.i_atime = t;
            if(mode != 0)
            {
                //for W|RW|APPEND mode : touch atime and mtime
                mip->INODE.i_mtime = t;
            }
            mip->dirty = 1;
            return i;//return i as the file descriptor

        }
        i+=1;
    }
    return -1;
}

void myopen()
{
    int open_mode;
    int fd;
    //checks mode
    if(strcmp(destname, "R") == 0)
    {
        open_mode = 0;
    }
    else if(strcmp(destname, "W") == 0)
    {
        open_mode = 1;
    }
    else if(strcmp(destname, "RW") == 0)
    {
        open_mode = 2;
    }
    else if(strcmp(destname, "APPEND") == 0)
    {
        open_mode = 3;
    }
    else{
        open_mode = -1;
        printf("usage: open filename mode\nmodes: R (read), W (write), RW (read and write), APPEND\n");
        return;
    }
    //open file
    fd = open_file(open_mode);
    if(fd >= 0)
    {
        if(fd < NFD)
        {
            printf("file %s opend\n");
        }
    }
}

int myclose(int fd)
{
	if(fd < 0 || fd >= NFD)
	{
		printf("File descriptor out of range (0-15)\n");
		return -1;
	}

	if(!running->fd[fd])
	{
		printf("File descriptor %d is not in use\n", fd);
		return -1;
	}

	OFT *oftp = running->fd[fd];

	// Remove fd from the process
	running->fd[fd] = 0;

	if(--oftp->refCount == 0)
	{
		oftp->inodeptr->dirty = 1;
		iput(oftp->inodeptr);
	}

	return 0;
}

int lseek(int fd, int position)
{
	if(fd < 0 || fd > NFD)
		return -1;

	OFT *file = running->fd[fd];
	int old_offset = file->offset;
	
	if(position >= 0 && position < file->inodeptr->INODE.i_size)
	{
		file->offset = position;
	}

	return old_offset;
}

int pfd()
{
	int i;
	char *modes[4] {"READ", "WRITE", "R/W", "APPEND"};
	printf("fd\tmode\toffset\tINODE\n");

	for(i = 0; i < NFD; i++)
	{
		if(running->fd[i])
			printf("%d\t%s\t%d\t[%d, %d]\n", i, modes[running->fd[i]->mode], running->fd[i]->offset, running->fd[i]->inodeptr->dev, running->fd[i]->inodeptr->ino);
	}
	putchar('\n');
}
///*** End level 2 functions ///***
