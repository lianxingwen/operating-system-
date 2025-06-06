/* Name to inode number translation functions */
#include <stdio.h>
#include <string.h>
#include "FILESYS.H"

/* Convert filename to inode number */
unsigned int namei(char *name) {
    int i;
    
    /* Search in current directory */
    for (i = 0; i < dir.size; i++) {
        if (strcmp(dir.direct[i].d_name, name) == 0) {
            return dir.direct[i].d_ino;
        }
    }
    
    /* Not found */
    return 0;
}

/* Add filename to directory and return inode number */
unsigned short iname(char *name) {
    int i;
    
    /* Find empty slot in directory */
    for (i = 0; i < DIRNUM; i++) {
        if (dir.direct[i].d_ino == 0) {
            strcpy(dir.direct[i].d_name, name);
            if (i >= dir.size) {
                dir.size = i + 1;
            }
            return i;
        }
    }
    
    /* Directory full */
    return 0;
}