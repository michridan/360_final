#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "util.h"
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>
#include "level_1.h"

MINODE minode[NMINODE];
MINODE *root;

PROC   proc[NPROC], *running;

char gpath[256];   // hold tokenized strings
char *name[64];    // token string pointers
int  n;            // number of token strings 




int fd, dev, d_start;
int  nblocks, ninodes, bmap, imap, inode_start;
char line[256], cmd[32], pathname[256], dname[256], bname[256], destname[256], store[256];
OFT oft[NOFT];

void mount_root(char * name)
{

    dev = open(name, O_RDWR);

   
    char buffer[1024];
    
    get_block(dev, 1, buffer);//gets super block into buffer

    SUPER* s = (SUPER*)buffer;//sets variable for super

    if(s->s_magic != 0xEF53)//makes sure disk is of EXT2 filesystem
    {
        printf("error: disk is not of EXT2 type filesystem\n");
        exit(0);
    }
    d_start = s->s_first_data_block; //locates first data block
    nblocks = s->s_blocks_count; //sets total number of blocks (used and free)
    ninodes = s->s_inodes_count; //sets total number of inodes (used and free)
    get_block(dev, 2, buffer);
    GD * g = (GD * )buffer;
    bmap = g->bg_block_bitmap; //gets list of the states of blocks (used = 1, free = 0)
    imap = g->bg_inode_bitmap; //gets list of the states of inodes (used = 1, free = 0)
    inode_start = g->bg_inode_table; //keeps track of every directory, regular file, symbolic 
    //link, or special file; their location, size, type and access rights
    root = proc[0].cwd = proc[1].cwd = iget(dev, 2); //sets root and cwd
}

void init()
{
    proc[0].uid = 0; //sets user id for first process
    proc[1].uid = 1;//sets uder id for second process
    running = proc;//sets the running process
    int i = 0;
    while(i < NMINODE)//iterates through inodes
    {

        minode[i].refCount = 0;//sets each minode's link count to 0
        i+=1;
    }
}

int main(int argc, char * argv[]){


    if(argc < 2)
    {
        printf("usage: ./a.out [diskname]\n");
        return 0;
    }

    init();

    mount_root(argv[1]);

    char input[256];
    printf("Enter help for command list\n");
    printf("Welcome enter a command:\n");
    
    while(1)
    {
        printf("\n~>");

        fgets(input, 256, stdin);
        
        input[strlen(input) - 1] = '\0';
        printf("input: %s\n", input);
        //sscanf(input, "%s %s %s", cmd, pathname, destname);
        parse_line(input);
        //printf("pathname : %s\n", pathname);
        //printf("cwd: %s path: %s\n", cmd, pathname);
       //puts(cmd);
       if(strcmp(cmd, "ls") == 0)
       {
           puts("FUCK ls\n");
       }
        if(strcmp(cmd, "cd") == 0)
        {
            mycd();
        }
        else if(strcmp(cmd, "mkdir") == 0)
        {
            make_dir();
        }
        else if(strcmp(cmd, "rmdir") == 0)
        {
            remove_dir();
        }
        else if(strcmp(cmd, "ls") == 0)
        {
            ls();
        }
        else if(strcmp(cmd, "pwd") == 0)
        {
            pwd();
        }

        else if(strcmp(cmd, "creat") == 0)
        {
            create_file();
        }
        else if(strcmp(cmd, "link") == 0)
        {

            mylink();
        }
        else if(strcmp(cmd, "unlink") == 0)
        {

            unlink();
        }
        else if(strcmp(cmd, "symlink") == 0)
        {
            mysymlink();
        }
        else if(strcmp(cmd, "touch") == 0)
        {
            utime();
        }
        else if(strcmp(cmd, "chmod") == 0)
        {
            ch_mod();
        }
        else if(strcmp(cmd, "stat") == 0)
        {

            mystat();
        }
        else if(strcmp(cmd, "open") == 0)
        {
            myopen();
        }
        else if(strcmp(cmd, "close") == 0)
        {

            myclose();
        }
        else if(strcmp(cmd, "read") == 0)
        {
            read_file();
        }
         else if(strcmp(cmd, "write") == 0)
        {

            write_file();
        }
        else if(strcmp(cmd, "lseek") == 0)
		{
			mylseek();
		}
		else if(strcmp(cmd, "pfd") == 0)
		{
			pfd();
		}
		else if(strcmp(cmd, "cat") == 0)
		{
			mycat();
		}
        else if(strcmp(cmd, "quit") == 0)
        {   
            int i = 0;
            while(i < NMINODE)
            {
                if(minode[i].refCount > 0 && minode[i].dirty == 1)
                {
                    minode[i].refCount = 1;
                    iput(&(minode[i]));
                }
                i += 1;
            }
            break;
        }
        else if (strcmp(cmd, "help") == 0)
        {
           printf("Available commands:\n{ cd | mkdir | rmdir | ls | pwd | creat | link | symlink | unlink | touch | stat | open | close | read | write | lseek | pfd | quit }\n");
        }
    
        else
        {
            printf("Invalid command\n");
            printf("Enter help for command list\n");
        }
    }
}
