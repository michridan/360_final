#include "idealloc_bdealloc.h"

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
