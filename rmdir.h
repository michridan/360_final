#ifndef RMDIR_H
#define RMDIR_H

#include "type.h"
#include "util.h"
#include "idealloc_bdealloc.h"
#include <time.h>

int rmdir(char *pathname);
int rm_child(MINODE *parent, char *name);

#endif
