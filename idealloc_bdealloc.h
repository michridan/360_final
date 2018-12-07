#ifndef IDEALLOC_BDEALLOC_H
#define IDEALLOC_BDEALLOC_H

#include "util.h"

int incFreeInodes(int dev);
int incFreeBlocks(int dev);
int idealloc(int dev, int ino);
int bdealloc(int dev, int bno);

#endif
