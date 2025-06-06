/* Global variable definitions for the file system */
#include <stdio.h>
#include "FILESYS.H"

/* Global variables */
struct hinode hinode[NHINO];
struct dir dir;
struct file sys_ofile[SYSOPENFILE];
struct filsys filsys;
struct pwd pwd[PWDNUM];
struct user user[USERNUM];
FILE *fd;
struct inode *cur_path_inode;
int user_id;
