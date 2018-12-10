#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "util.h"
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>

int open_file(int mode);
void myopen();
