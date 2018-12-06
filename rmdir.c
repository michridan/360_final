#include "rmdir.h"


int rmdir(char *pathname)
{
	
	int ino = getino(&dev, pathname), empty = 1, i;
	
	if(!ino) // if not found
		return -1;

    MINODE *mip = iget(dev, ino), pip;
	DIR *dp;
	char *cp, buf[BLKSIZE], temp[BLKSIZE], *name = basename(pathname);

	if(running.uid != 0 && running.uid != mip->INODE.i_uid)
	{
		printf("You don't have permission to remove this directory.\n");
		iput(mip);
		return -1;
	}

	// Check if DIR
	if(S_ISDIR(mip->INODE.i_mode))
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
	ino = search(mip, "..");
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
		if(mip->INODE.i_block[i] == 0)
			break;
		get_block(dev, mip->INODE.i_block[i], buf);

		cp = buf;
		dp = (DIR *)cp;

		while(!found && cp < buf + BLKSIZE)
		{
			cp_prev = cp;

			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;

			if(!strcmp(temp, name))
			{
				found = 1;
				break;
			}

			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}

	// Erase name entry from parent directory
	if(cp + dp->rec_len == buf + BLKSIZE)
	{
		if(cp == buf)
		{
			// Case 1: Only entry in block, rearrange blocks
			for(i -= 1; i < 11; i++;)
			{
				mip->INODE.i_blocks[i] = mip->INODE.i_blocks[i + 1];
			}

			// Set last block to 0
			mip->INODE.i_blocks[11] = 0;
		}
		else
		{
			// Case 2: Last entry in block, add to rec_len of previous entry
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

			// Copy the entry over
			dp_prev->inode = dp->inode;
			dp_prev->rec_len = dp->rec_len;
			dp_prev->name_len = dp->name_len;
			strncpy(dp_prev->name, dp->name, dp->name_len);

			// Advance the previous entry pointer
			cp_prev += dp_prev->rec_len;
			dp_prev = (DIR *)cp_prev;
		}

		// add back the extra space to the last entry
		dp_prev->rec_len += extra;
	}
}
