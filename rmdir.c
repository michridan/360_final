#include "rmdir.h"

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
