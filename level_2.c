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
    }
    MINODE * mip = iget(dev, ino);
    if(!S_ISREG(mip->INODE.i_mode))
    {
        printf("error: %s is not a refular file\n")
        return -1;
    }
    while (i < 64)
    {
        if(oft[i].mptr == mip && oft[i].mode != 0)
        {
            //Already open but not for read
            printf("error: file %s is already open\n", pathname);
            return -1;
        }
        i+=1;
    }
    i = 0;
    while(i < 64)
    {
        if(oft[i].refCount == 0)
        {
            oft[i] = (OFT){.mode = mode, .refCount = 1, .mptr = mip, .offset = (mode == 3 ? mip->inode.i_size : 0)};
            if(mode == 1)
            {
                mip->INODE.i_size = 0;
            }
            int n = 0;
            while(n < 10)
            {
                if(!running->fd[n];)
                break;
                n+=1;
            }
            time_t t = time(0L);
            mip->INODE.i_atime = t;
            if(mode != 0)
            {
                mip->INODE.i_mtime = t;
            }
            mip->dirty = 1;
            return i;

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
        if(fd < 10)
        {
            printf("file %s opend\n");
        }
    }
}

///*** End level 2 functions ///***