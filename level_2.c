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
extern OFT oft[NOFT];


///*** Level 2 functions ///***

int open_file(int mode)
{
    int i = 0, j = 0, success = 0;
    //gets ino for file to opne

    int ino = getino(pathname);
	OFT *file = 0;
    if(!ino)
    {
        printf("error: file %s does not exist\n", pathname);
    }
    MINODE * mip = iget(dev, ino);
    if(!S_ISREG(mip->INODE.i_mode))
    {
        printf("error: %s is not a regular file\n", pathname);
        return -1;
    }
    while (i < NOFT)
    {
        if(oft[i].refCount != 0 && oft[i].mptr->ino == mip->ino)
        {
			if(oft[i].mode != 0 || mode != 0)
			{
				//Already open but not for read
				printf("error: file %s is already open with a conflicting mode\n", pathname);
				return -1;
			}
			else
			{
				// File already opened, but both will be for read
				file = &oft[i];
			}
        }
        i+=1;
    }
    i = 0;
    while(i < NOFT)
    {
        // allocate a FREE OpenFileTable (OFT) and fill in values
        if(oft[i].refCount == 0)
        {
            oft[i].mode = mode;
            oft[i].refCount = 1;
            oft[i].mptr = mip;
            oft[i].offset = (mode == 3 ? mip->INODE.i_size : 0);
			file = &oft[i];
		}
		if(file)
		{
			// Add file to the process
            while(j < NFD && !success)
            {
                if(!running->fd[j])
                {
					running->fd[j] = file;
					success = 1;
                    break;
                }
                j++;
            }
			if(!success)
			{
				printf("current process has too many files open\n");
				return -1;
			}

			// Prepare the inode for the format it's being opened in
            if(mode == 1)
            {
                // W: truncate file to 0 size
                truncate(mip);
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
            return j;//return j as the file descriptor

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
            printf("file %s opend with fd %d\n", pathname, fd);
        }
    }
}

int close_file(int fd)
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
		oftp->mptr->dirty = 1;
		iput(oftp->mptr);
	}

	return 0;
}

int myclose()
{
	if(!isdigit(pathname[0]))
	{
		printf("Usage: close [FILE DESCRIPTOR]\n");
		return -1;
	}

	int fd = atoi(pathname);
	
	close_file(fd);

	return 0;
}

int mylseek()
{
	if(!isdigit(pathname[0]) || !isdigit(destname[0]))
	{
		printf("Usage: lseek [FILE DESCRIPTOR] [POSITION]\n");
		return -1;
	}

	int fd = atoi(pathname), position = atoi(destname);

	if(fd < 0 || fd > NFD)
	{
		printf("Offset out of range\n");
		return -1;
	}

	OFT *file = running->fd[fd];
	int old_offset = file->offset;
	
	if(position >= 0 && position < file->mptr->INODE.i_size)
	{
		file->offset = position;
	}

	return old_offset;
}

int pfd()
{
	int i;
	char *modes[4] = {"READ", "WRITE", "R/W", "APPEND"};
	printf("fd\tmode\toffset\tINODE\n");

	for(i = 0; i < NFD; i++)
	{
		if(running->fd[i])
			printf("%d\t%s\t%d\t[%d, %d]\n", i, modes[running->fd[i]->mode], running->fd[i]->offset, running->fd[i]->mptr->dev, running->fd[i]->mptr->ino);
	}
	putchar('\n');
}

void dup(int fd)
{
    if(running->fd[fd]->refCount <= 0)
    {
        printf("error: file descriptor not opened\n");
        return;
    }
    else
    {
        int i = 0;
        while(running->fd[i])
        {
            //get to FIRST empty fd[ ] slot
            i += 1;
        }
        //duplicates (copy) fd[fd] into FIRST empty fd[ ] slot;
        running->fd[i]->mode = running->fd[fd]->mode;
        running->fd[i]->refCount = running->fd[fd]->refCount;
        running->fd[i]->mptr = running->fd[fd]->mptr;
        running->fd[i]->offset = running->fd[fd]->offset;
        //increment OFT's refCount by 1;
        running->fd[fd]->refCount +=1;
    }
}

void dup2(int fd, int gd)
{
    close_file(gd);
    running->fd[gd]->mode = running->fd[fd]->mode;
    running->fd[gd]->refCount = running->fd[fd]->refCount;
    running->fd[gd]->mptr = running->fd[fd]->mptr;
    running->fd[gd]->offset = running->fd[fd]->offset;
}

///*** End level 2 functions ///***
