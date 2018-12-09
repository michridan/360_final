#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "util.h"
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>
#include "rmdir.h"
#include "ialloc_balloc.h"

MINODE minode[NMINODE];
MINODE *root;

PROC   proc[NPROC], *running;

char gpath[256];   // hold tokenized strings
char *name[64];    // token string pointers
int  n;            // number of token strings 

int fd, dev, d_start;
int  nblocks, ninodes, bmap, imap, inode_start;
char line[256], cmd[32], pathname[256], dname[256], bname[256], destname[256];


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

void dir_base_name(char *path)
{

    char temp[256];
    strcpy(temp, path);
    strcpy(dname, dirname(temp));
    strcpy(temp, path);
    strcpy(bname, basename(temp));


    
}

int mycd()
{
    INODE current;
    char comp[256];
    if(strcmp(pathname,"\0") == 0 || strcmp(pathname,"/") == 0)
    {
        running->cwd = root;
        return 0;
    }
    //char * token = strtok(pathname, "/");
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    if(S_ISDIR(mip->INODE.i_mode))
    {
        iput(running->cwd);
        running->cwd = mip;
    }
    else{
        printf("error: the path %s does not lead to a DIR\n", pathname);
        return 1;
    }
}

void pwd_helper(MINODE * c_cwd)
{
    if(c_cwd == root)
    {
        return;
    }
    char buffer[256];
    int pino = search(c_cwd, "..");
    MINODE * pip = iget(dev, pino);
    findmyname(pip, c_cwd->ino, buffer);
    pwd_helper(pip);
    iput(pip);
    printf("/%s", buffer);
}

void pwd(void)
{
    
    if(running->cwd == root)
        printf("/");
    else
        pwd_helper(running->cwd);
    printf("\n");
}

void mydirname(char * path){
    char temp[128];
    strcpy(temp, path);
    strcpy(path, "");
    char * tokens[69];
    int i = 0;
    tokens[i] = strtok(temp, "/");
    while(tokens[i] != NULL)
    {
        i += 1;
        tokens[i] = strtok(NULL, "/");
    }
    i = 0;
    char * ret[128]; 
    while(tokens[i + 1] != NULL)
    {   
        strcat(path, tokens[i]);
        strcat(path, "/");
        i+=1;
    }
    return;
}

void mybasename(char * path){
    //last token
    char temp[128];
    strcpy(temp, path);
    char * tokens[69];
    int i = 0;
    tokens[i] = strtok(temp, "/");
    while(tokens[i] != NULL)
    {
        i += 1;
        tokens[i] = strtok(NULL, "/");
        if(tokens[i+1] == NULL)
        {
            strcpy(path, "");
            strcpy(path, tokens[i-1]);
        }
    }


}


int enter_name(MINODE *pip, int myino, char * myname)
{

    int i = 0;
    int ideal_length = 4 * ((8 + strlen(myname)+3) / 4);
    while (i < 12)
    {
       
        char buffer[BLKSIZE];
        DIR * dp = (DIR * )buffer;
        char * cp = buffer;
        int need_length = 4 * ( ( 8 + strlen(myname) + 3) / 4 ) ;
        if (pip->INODE.i_block[i] == 0)
        {
            //allocates a new block for future use
            pip->INODE.i_block[i] = balloc(dev);
            get_block(pip->dev, pip->INODE.i_block[i], buffer);
            //sets members of new block
            *dp = (DIR){.inode = myino, .rec_len = BLKSIZE, .name_len = strlen(myname)};
            //copies name 
            strncpy(dp->name, myname, dp->name_len);
            break;
        }
         
        get_block(pip->dev, pip->INODE.i_block[i], buffer);
        while(cp + dp->rec_len < buffer + BLKSIZE)
        {
            //go to next directory entry
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        printf("i: %d\n", i);
        int remain = dp->rec_len - (4 * ((8 + dp->name_len + 3)/4));//cuts down to real length
        
        printf("i: %d\n", i);
        if(remain >= need_length)
        {
            dp->rec_len = (4 * (( 8 + dp->name_len + 3)/4));
            //go to next directory entry
            cp += dp->rec_len;
            dp = (DIR *)cp;
            *dp = (DIR){.inode = myino, .rec_len = remain, .name_len = strlen(myname)};
            //sets data values dor block
            strncpy(dp->name, myname, dp->name_len);
            put_block(pip->dev, pip->INODE.i_block[i], buffer);
            //^ writes data block to disk
            pip->dirty = 1;//marks as dirty
            return 0;
        }
        i+=1;
    }    
}


int mymkdir(MINODE *pip)
{

    
    int ino = ialloc(dev);    
    int bno = balloc(dev);

 
    MINODE * mip = iget(dev, ino); //in order to write contents into INODE in memory
    INODE * ip = &(mip->INODE);
    *ip  = (INODE){.i_mode = 0x41ED, .i_uid = running->uid, .i_gid = running->gid, .i_size = BLKSIZE, .i_links_count = 2, .i_atime = time(0L), .i_ctime = time(0L), .i_mtime = time(0L), .i_blocks = 2, .i_block = {bno} };

    //makes mip->INODE have DIR contents
    mip->dirty = 1;
    char buffer[BLKSIZE] ={0};
    char * cp = buffer;
    DIR * dp = (DIR*)buffer;

    *dp = (DIR){.inode = ino, .rec_len = 12, .name_len = 1, .name = "."};
    //go to next directory entry to make .
    cp += dp->rec_len;
    dp = (DIR*)cp;
    *dp = (DIR){.inode = pip->ino, .rec_len = BLKSIZE - 12, .name_len = 2, .name = ".."};
    //entries for . and .. written into buffer
    //now write buffer to the disk block bno

    put_block(dev, bno, buffer);
    //now enter the nane of new DIR into parent's DIR

    enter_name(pip, ino, bname);

    iput(mip);
    return ino;
}

int make_dir(){
    dir_base_name(pathname);

    int pino = getino(dname);
    if(pino == 0)
    {
        printf("error: %s doesn't exist\n", dname);
        return 1;
    }
   
    MINODE * pip = iget(dev, pino);
    if(search(pip, bname))
    {
        printf("error: DIR %s already exists\n", bname);
        return 3;
    }
    if(!(S_ISDIR(pip->INODE.i_mode)))
    {
        printf("error: %s is not a DIR\n", dname);
        return 2;
    }
    //else is a dir
    else{
        pip->INODE.i_links_count += 1;//adding another link to file so increment # of links
        mymkdir(pip);
       
        pip->dirty = 1;
    }
    iput(pip);
}

void file_ls(char * f_name, int ino)
{
    int i = 8;
    MINODE * mip = iget(dev, ino);
    const char * perm = "xwrxwrxwr-------";


    if(S_ISREG(mip->INODE.i_mode))
    {
        putchar('-');
    }
    else if(S_ISDIR(mip->INODE.i_mode))
    {
        putchar('d');
    }
    else if(S_ISLNK(mip->INODE.i_mode))
    {
        putchar(('l'));
    }
    while(i >= 0)
    {
        //putchar(((mip->inode.i_mode) & (1 << i)) ? perm[i] : '-');
        
        //check bit i of  i_mode is 1 or 0
        if(mip->INODE.i_mode & (1 << i))
        {
            putchar(perm[i]);
        }
        else
        {
            putchar('-');
        }
        
        i -= 1;
    }
    time_t t = (time_t)(mip->INODE.i_ctime);
    char temp[256];
    strcpy(temp, ctime(&t));
    //append null to end
    temp[strlen(temp) - 1] = '\0';
    //printf info of file
    printf(" %4d %4d %4d %4d %s %s", (int)(mip->INODE.i_links_count), (int)(mip->INODE.i_gid), (int)(mip->INODE.i_uid), (int)(mip->INODE.i_size), temp, f_name);
    if(S_ISLNK(mip->INODE.i_mode))
    {
        //if link, display linked file
        printf(" -> %s", (char * )(mip->INODE.i_block));
    }
    printf("\n");
    iput(mip);

}

void DIR_ls(char * d_name, int ino)
{
    char buffer[BLKSIZE];
    MINODE * mip = iget(dev, ino);
    int i = 0;
    while(i<12 && mip->INODE.i_block[i])
    {   
        //read blocks into buffer
        get_block(dev, mip->INODE.i_block[i], buffer);
        //iterate through data block entries
        char * cp = buffer;
        DIR * dp = (DIR * ) buffer;
        while(cp < BLKSIZE + buffer)
        {
            
            char temp[256];
            //get name of each file in data block
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            //ls the file
            file_ls(temp, dp->inode);
            //go to next directory entry
            cp += dp->rec_len;
            dp = (DIR *)cp;
            
        }   

        i+=1;
    }
    iput(mip);
}



int ls()
{
    char temp[256];
    int ino;
    if(strlen(pathname) == 0)
    {
        strcpy(pathname, ".");
        ino = running->cwd->ino;
        //gets ino for cwd to print contents of current path
    }
    else
    {
        strcpy(temp, pathname);
        ino = getino(temp);
        //get ino for specified path
    }

    MINODE * mip = iget(dev, ino);
    //gets MINODE for specified path
    if(S_ISDIR(mip->INODE.i_mode))
    {
        //get ino for the DIR
        int d_ino = getino(pathname);
        //path leads to DIR so ls the DIR
        DIR_ls(pathname, d_ino);
    }
    else
    {
        //path leads to file so ls the file
        file_ls(pathname, ino);
    }
   iput(mip);
 
}

void parse_line(char * line)
{
       
    strcpy(cmd, "");
            
    strcpy(pathname, "");

    strcpy(destname, "");
        
    strcpy(cmd, strtok(line, " \n"));

    char * temp = strtok(NULL, "\n ");

    if (!temp)
    {
        return;
    }
    strcpy(pathname, temp);
    temp = strtok(NULL, "\n ");

    if (!temp)
    {
        return;
    }
    strcpy(destname, temp);
}

int create_name(MINODE *pip, int myino, char * myname)
{

    int i = 0;
    int ideal_length = 4 * ((8 + strlen(myname)+3) / 4);
    while (i < 12)
    {
       
        char buffer[BLKSIZE];
        DIR * dp = (DIR * )buffer;
        char * cp = buffer;
        int need_length = 4 * ( ( 8 + strlen(myname) + 3) / 4 ) ;
        if (pip->INODE.i_block[i] == 0)
        {
            //allocates a new block for future use
            pip->INODE.i_block[i] = balloc(dev);
            get_block(pip->dev, pip->INODE.i_block[i], buffer);
            //sets members of new block
            *dp = (DIR){.inode = myino, .rec_len = BLKSIZE, .name_len = strlen(myname)};
            //copies name 
            strncpy(dp->name, myname, dp->name_len);
            break;
        }
         
        get_block(pip->dev, pip->INODE.i_block[i], buffer);
        while(cp + dp->rec_len < buffer + BLKSIZE)
        {
            //go to next directory entry
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        printf("i: %d\n", i);
        int remain = dp->rec_len - (4 * ((8 + dp->name_len + 3)/4));//cuts down to real length
        
        printf("i: %d\n", i);
        if(remain >= need_length)
        {
            dp->rec_len = (4 * (( 8 + dp->name_len + 3)/4));
            //go to next directory entry
            cp += dp->rec_len;
            dp = (DIR *)cp;
            *dp = (DIR){.inode = myino, .rec_len = remain, .name_len = strlen(myname)};
            //sets data values dor block
            strncpy(dp->name, myname, dp->name_len);
            put_block(pip->dev, pip->INODE.i_block[i], buffer);
            //^ writes data block to disk
            pip->dirty = 1;//marks as dirty
            return 0;
        }
        i+=1;
    }    
}


int mycreate(MINODE *pip)
{

    
    int ino = ialloc(dev);    
    int bno = 0;

 
    MINODE * mip = iget(dev, ino); //in order to write contents into INODE in memory
    INODE * ip = &(mip->INODE);
    *ip  = (INODE){.i_mode = 0x81A4, .i_uid = running->uid, .i_gid = running->gid, .i_size = BLKSIZE, .i_links_count = 1, .i_atime = time(0L), .i_ctime = time(0L), .i_mtime = time(0L), .i_blocks = 0, .i_block = {bno} };

    //makes mip->INODE have DIR contents
    mip->dirty = 1;
    
    //now enter the nane of new DIR into parent's DIR

    create_name(pip, ino, bname);

    iput(mip);
    return ino;
}

int create_file(){
    dir_base_name(pathname);

    int pino = getino(dname);
    if(pino == 0)
    {
        printf("error: %s doesn't exist\n", dname);
        return 1;
    }
   
    MINODE * pip = iget(dev, pino);
    if(search(pip, bname))
    {
        printf("error: DIR %s already exists\n", bname);
        return 3;
    }
    if(!(S_ISDIR(pip->INODE.i_mode)))
    {
        printf("error: %s is not a DIR\n", dname);
        return 2;
    }
    //else is a dir
    else{
        pip->INODE.i_links_count += 1;//adding another link to file so increment # of links
        mycreate(pip);
       
        pip->dirty = 1;
    }
    iput(pip);
}

void mylink()
{
    
    printf("destname: %s\n", destname);
    int ino = getino(pathname);//get ino for file to link
    
    //check if file exists
    if(!ino){
        printf("Link target %s does not exist\n", pathname);
        return;
    }
    MINODE * linked = iget(dev, ino); //get MINODE for file to link
    
    if(S_ISDIR(linked->INODE.i_mode))
    {
        printf("Can't link to a DIR\n");
        return;
    }
    
    dir_base_name(pathname);
    
    int dino = getino(dname);
    //bname f1

    if(!dino)
    {
        printf("DIR %s does not exist\n", dname);
        return;
    }

    MINODE * dminode = iget(dev, dino);//get MINODE for dir containing file to be linked

    if(!S_ISDIR(dminode->INODE.i_mode))
    {
        printf("Can't link to a DIR\n");
        return;
    }
   // if(search(dminode, bname))
    //{
     //   printf("File %s already exists", bname);
      //  return;
    //}
    

         
        enter_name(dminode, ino, destname);

        linked->dirty = 1;
        linked->INODE.i_links_count += 1;
        iput(linked);
        iput(dminode);
    

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

int unlink(void)
{
	if(!strcmp(pathname, ""))
	{
		printf("Usage: unlink [pathname]\n");
		return -1;
	}

	int ino = getino(pathname), empty = 1, i;
	
	if(!ino)
	{
		printf("%s not found\n", pathname);
		return -1;
	}

    MINODE *mip = iget(dev, ino), *pip;
	DIR *dp;
	char *cp, buf[BLKSIZE], temp[BLKSIZE], *dname = dirname(pathname), *name = basename(pathname);

	if(running->uid != 0 && running->uid != mip->INODE.i_uid)
	{
		printf("You don't have permission to remove %s.\n", name);
		iput(mip);
		return -1;
	}

	// Check if REG or LNK
	if(!S_ISREG(mip->INODE.i_mode) && !S_ISLNK(mip->INODE.i_mode))
	{
		printf("%s is not a regular file or symbolic link.\n", name);
		iput(mip);
		return -1;
	}

	// Decrement links_count and deallocate if zero
	if(--mip->INODE.i_links_count == 0)
	{
		truncate(mip);
		idealloc(mip->dev, mip->ino);
	}
    iput(mip);

	// get parent's ino and Minode 
	ino = getino(dname);
	pip = iget(mip->dev, ino);

	// remove child's entry from parent directory

	rm_child(pip, name);

	pip->INODE.i_links_count--;
	pip->dirty = 1;
	pip->INODE.i_atime = (u32)time(NULL);
	pip->INODE.i_mtime = pip->INODE.i_atime;

    iput(pip);

    return 1;
}

int mystat()
{
	int ino = getino(pathname);

	if(!ino)
	{
		return 0;
	}

	MINODE *mip = iget(dev, ino);
	INODE st = mip->INODE;

	dir_base_name(pathname);

	printf("Name: %s\n", bname);
	printf("Size: %d\n", st.i_size);
	printf("Links: %d\n", st.i_links_count);
	printf("User: %d\n", st.i_uid);
	printf("Group: %d\n", st.i_gid);
	printf("Blocks: %d\n", st.i_blocks);
	printf("Ino: %d\n", mip->ino);
	printf("Dev: %d\n", mip->dev);
	printf("Access Time: %s\n", ctime((time_t *)&st.i_atime));
	printf("Mod Time: %s\n", ctime((time_t *)&st.i_mtime));
	printf("Create Time: %s\n", ctime((time_t *)&st.i_ctime));

	iput(mip);
	return 1;
}

int ch_mod()
{
	char path[256] = {0};
	int mod, ino;

	sscanf(pathname, "%d %s", &mod, path);

	if((ino = getino(path)) == 0)
		return 0;

	MINODE *mip = iget(dev, ino);

	// clear current permissions
	//mip->INODE.i_mode &= 
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
        printf("~>");

        fgets(input, 256, stdin);
        input[strlen(input) - 1] = '\0';
        
        //sscanf(input, "%s %s %s", cmd, pathname, destname);
        parse_line(input);
        //printf("cwd: %s path: %s\n", cmd, pathname);
        printf("cmd: %s path: %s\n", cmd, pathname);
       puts(cmd);
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
		else if(strcmp(cmd, "unlink") == 0)
		{
			unlink();
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
            printf("about to call\n");
            mylink();
        }
		else if(strcmp(cmd, "stat") == 0)
		{
			mystat();
		}
        else if(strcmp(cmd, "quit") == 0)
        {
            break;
        }
        else if (strcmp(cmd, "help") == 0)
        {
           printf("{Available commands: cd | mkdir | rmdir | ls | pwd | quit}\n");
        }
        else
        {
            printf("Invalid command\n");
            printf("Enter help for command list\n");
        }
    }

}
