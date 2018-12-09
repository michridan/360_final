#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#include "util.h"
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>

int mycd();
void pwd_helper(MINODE * c_cwd);
void pwd(void);
void mydirname(char * path);
void mybasename(char * path);
int enter_name(MINODE *pip, int myino, char * myname);
int mymkdir(MINODE *pip);
int make_dir();
void file_ls(char * f_name, int ino);
void DIR_ls(char * d_name, int ino);
int ls();
void parse_line(char * line);
int create_name(MINODE *pip, int myino, char * myname);
int mycreate(MINODE *pip);
int create_file();
void mylink();
int make_link();
void mysymlink();
int remove_dir(void);
int rm_child(MINODE *parent, char *name);
int unlink(void);
void utime();
int mystat();
int ch_mod();
