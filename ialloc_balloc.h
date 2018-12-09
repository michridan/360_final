#ifndef IALLOC_BALLOC_H
#define IALLOC_BALLOC_H

#include "util.h"

int decFreeInodes(int dev);
int decFreeblocks(int dev);
int ialloc(int dev);
int balloc(int dev);

#endif
