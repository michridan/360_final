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
int close_file(int fd);
int myclose();
int mylseek();
int pfd();
void dup(int fd);
void dup2(int fd, int gd);
