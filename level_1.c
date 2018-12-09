#include "level_1.h"

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

//start for level_1.c
int mycd()
{
    INODE current;
    char comp[256];
    if(strcmp(pathname,"\0") == 0 || strcmp(pathname,"/") == 0)
    {
        running->cwd = root;
        return 0;
    }
    char * token = strtok(pathname, "/");
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
    int myino = search(c_cwd, ".");
    MINODE * pip = iget(dev, pino);
    findmyname(pip, myino, buffer);
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
    char * temp[128];
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
    char * temp[128];
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

        int remain = dp->rec_len - (4 * ((8 + dp->name_len + 3)/4));//cuts down to real length
        

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

        int remain = dp->rec_len - (4 * ((8 + dp->name_len + 3)/4));//cuts down to real length
        

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

         
        enter_name(dminode, ino, destname);

        linked->dirty = 1;
        linked->INODE.i_links_count += 1;
        iput(linked);
        iput(dminode);
    

}


int make_link()
{
     dir_base_name(pathname);

    int pino = getino(dname);
    if(pino == 0)
    {
        printf("error: %s doesn't exist\n", dname);
        return 1;
    }
   
    MINODE * pip = iget(dev, pino);
    if(!(S_ISDIR(pip->INODE.i_mode)))
    {
        printf("error: %s is not a DIR\n", dname);
        return 2;
    }
    //else is a dir
    if(search(pip, destname))
    {
        printf("error: %s already exists\n", destname);
        return 4;
    }
    int ino = ialloc(dev);
    int bno = 0;
    MINODE * mip = iget(dev, ino);
    INODE * ip = &(mip->INODE);
    *ip  = (INODE){.i_mode = 0x81A4, .i_uid = running->uid, .i_gid = running->gid, .i_size = BLKSIZE, .i_links_count = 1, .i_atime = time(0L), .i_ctime = time(0L), .i_mtime = time(0L), .i_blocks = 0, .i_block = {bno} };
    mip->dirty = 1;
    enter_name(pip, ino, destname);
    
    
    iput(mip);
    iput(pip);
    return ino;
}


void mysymlink(){
    int tino = getino(pathname);
    if(!tino)
    {
        printf("Soruce path %s does not exist\n", pathname);
        return;
    }
    int ino = make_link(destname);
    MINODE * newmip = iget(dev, ino);
    newmip->INODE.i_mode = 0xA1A4;
    strcpy((char*)(newmip->INODE.i_block), pathname);
    iput(newmip);
}




int remove_dir(void)
{
	if(!strcmp(pathname, ""))
	{
		printf("Usage: rmdir [pathname]\n");
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
		printf("You don't have permission to remove this directory.\n");
		iput(mip);
		return -1;
	}

	// Check if DIR
	if(!S_ISDIR(mip->INODE.i_mode))
	{
		printf("%s is not a directory.\n", name);
		iput(mip);
		return -1;
	}

	//Check if empty
	if(mip->INODE.i_links_count > 2)
		empty = 0;

	for(i = 0; i < 12 && empty; i++)
	{
		if(mip->INODE.i_block[i] == 0)
			break;
		get_block(dev, mip->INODE.i_block[i], buf);

		dp = (DIR *)buf;
		cp = buf;

		while ((cp < buf + BLKSIZE) && empty)
		{
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;

			if(strcmp(temp, ".") && strcmp(temp, "..")) // Any entries other than these two are "not empty"
				empty = 0;
			cp += dp->rec_len;
			dp = (DIR *)cp;	   
		}
	}

	if(!empty)
	{
		printf("%s is not empty.\n", name);
		iput(mip);
		return -1;
	}
		
	// Check if busy
	for(i = 0; i < NPROC; i++)
	{
		// We only have to check if a process is using mip as its cwd
		// if mip has any children, it will fail as being empty
		if(mip == proc[i].cwd)
		{
			printf("%s is in use.\n", name);
			iput(mip);
			return -1;
		}
	}

    // Deallocate its block and inode
	for (i=0; i<12; i++)
	{
		if (mip->INODE.i_block[i]==0)
			continue;

        bdealloc(mip->dev, mip->INODE.i_block[i]);
    }

    idealloc(mip->dev, mip->ino);
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

/*
 * Removes the entry [INO rlen nlen name] from parent's data block.
 */
int rm_child(MINODE *parent, char *name)
{
	int found = 0, i, extra;
	DIR *dp, *dp_prev;
	char *cp, *cp_prev, temp[BLKSIZE], buf[BLKSIZE];

	// Search parent for name, keep track of positions
	for(i = 0; !found && i < 12; i++)
	{
		if(parent->INODE.i_block[i] == 0)
			break;
		get_block(dev, parent->INODE.i_block[i], buf);

		cp = buf;
		dp = (DIR *)cp;

		while(!found && cp < buf + BLKSIZE)
		{

			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;

			if(!strcmp(temp, name))
			{
				found = 1;
				break;
			}

			cp_prev = cp;
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}

	i--;

	// Erase name entry from parent directory
	if(cp + dp->rec_len == buf + BLKSIZE)
	{
		if(cp == buf)
		{
			// Case 1: Only entry in block, rearrange blocks
			char erase[BLKSIZE] = {0};
			put_block(dev, parent->INODE.i_block[i], erase);
			bdealloc(dev, parent->INODE.i_block[i]);
			for(i; i < 11; i++)
			{
				parent->INODE.i_block[i] = parent->INODE.i_block[i + 1];
			}

			// Set last block to 0
			parent->INODE.i_block[11] = 0;
		}
		else
		{
			// Case 2: Last entry in block, add to rec_len of previous entry
			dp_prev = (DIR *)cp_prev;
			dp_prev->rec_len += dp->rec_len;
		}
	}
	else
	{
		// Case 3: Middle of block, move every entry after it backwards
		extra = dp->rec_len;
		cp_prev = cp;
		dp_prev = (DIR *)cp;

		while(cp + dp->rec_len < buf + BLKSIZE)
		{
			// Advance pointer to the entry to move
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		// add the extra space to the last entry
		dp->rec_len += extra;

		// Move the stuff after the old entry back to fill the gap
		memcpy(cp_prev, cp_prev + extra, (buf + BLKSIZE) - (cp_prev + extra) + 1);
	}
	put_block(dev, parent->INODE.i_block[i], buf);
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
		/*
		// Deallocate its block and inode
		for (i=0; i<12; i++)
		{
			if (mip->INODE.i_block[i]==0)
				continue;
			bdealloc(mip->dev, mip->INODE.i_block[i]);
		}
		*/
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

void utime()//touch
{

    dir_base_name(pathname);

    int ino = getino(bname);

    if(ino == 0)
    {

        create_file();
        return 1;
    }
      else
    {

        MINODE * mip = iget(dev, ino);

        
        mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
        mip->dirty = 1;
        iput(mip);



    }
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
	int mod, ino;

	sscanf(pathname, "%o", &mod);

	if((ino = getino(destname)) == 0)
		return 0;

	MINODE *mip = iget(dev, ino);

	// clear current permissions
	mip->INODE.i_mode &= 0777000;

	// replace with new permissions
	mip->INODE.i_mode |= mod;

	// Set mtime and mark as dirty
	mip->INODE.i_mtime = time(0L);
	mip->dirty = 1;
	iput(mip);
	return 1;
}