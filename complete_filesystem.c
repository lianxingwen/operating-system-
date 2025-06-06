#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 文件系统常量定义
#define BLOCKSIZ 512
#define DISKFULL 65535
#define SYSOPENFILE 40
#define USERNUM 10
#define NOFILE 16
#define PWDNUM 10
#define NHINO 128
#define NICINOD 100
#define NICFREE 100
#define NADDR 6
#define DIRNUM 32
#define DIRSIZ 14

// 文件类型和权限
#define DIFILE 0100000
#define DIDIR 0040000
#define DIREAD 0000444
#define DIWRITE 0000222
#define DIEXEC 0000111
#define DEFAULTMODE 0000644

// 访问模式
#define READ 1
#define WRITE 2
#define EXEC 3

// 数据结构定义
struct pwd {
    unsigned short p_uid;
    char password[8];
    unsigned short p_gid;
};

struct user {
    unsigned short u_uid;
    unsigned short u_gid;
    unsigned short u_default_mode;
    unsigned short u_ofile[NOFILE];
};

struct inode {
    unsigned int i_ino;
    unsigned short di_mode;
    unsigned short di_nlink;
    unsigned short di_uid;
    unsigned short di_gid;
    unsigned int di_size;
    unsigned short di_addr[NADDR];
    unsigned short i_count;
    unsigned short i_flag;
};

struct hinode {
    unsigned int i_ino;
    struct inode *i_inode;
};

struct direct {
    unsigned int d_ino;
    char d_name[DIRSIZ];
};

struct dir {
    struct direct direct[DIRNUM];
    unsigned int size;
};

struct file {
    unsigned short f_flag;
    unsigned short f_count;
    struct inode *f_inode;
    unsigned int f_off;
};

struct filsys {
    unsigned short s_isize;
    unsigned int s_fsize;
    unsigned short s_nfree;
    unsigned int s_free[NICFREE];
    unsigned short s_ninode;
    unsigned int s_inode[NICINOD];
    unsigned short s_flock;
    unsigned short s_ilock;
    unsigned short s_fmod;
    unsigned short s_ronly;
    unsigned int s_time;
};

// 全局变量
struct hinode hinode[NHINO];
struct dir dir;
struct file sys_ofile[SYSOPENFILE];
struct filsys filsys;
struct pwd pwd[PWDNUM];
struct user user[USERNUM];
FILE *fd;
struct inode *cur_path_inode;
int current_user_id = -1;

// 函数声明
void format();
void install();
struct inode* iget(unsigned int dinodeid);
void iput(struct inode* pinode);
unsigned int balloc();
void bfree(unsigned int block_num);
unsigned int ialloc();
void ifree(unsigned int dinodeid);
int access_check(unsigned int user_id, struct inode* inode, unsigned short mode);
void _dir();
void mkdir(char* dirname);
void chdir(char* dirname);
unsigned short creat(unsigned int user_id, char* filename, unsigned short mode);
int login(unsigned short uid, char* password);
void logout(unsigned short uid);
void close_file(unsigned int user_id, unsigned short cfd);
void delete_file(char* filename);
void halt();
unsigned int namei(char* name);
unsigned short iname(char* name);
unsigned short aopen(int user_id, char* filename, unsigned short openmode);
unsigned int write_file(int fd_user, char* buf, unsigned int size);
unsigned int read_file(int fd_user, char* buf, unsigned int size);

// 格式化文件系统
void format() {
    FILE *fp;
    int i, j;
    struct inode inode_buf;
    
    fp = fopen("filesystem", "w+b");
    if (fp == NULL) {
        printf("无法创建文件系统文件\n");
        return;
    }
    
    // 初始化超级块
    filsys.s_isize = 1024;
    filsys.s_fsize = 8192;
    filsys.s_nfree = NICFREE;
    filsys.s_ninode = NICINOD;
    
    // 初始化空闲块链表
    for (i = 0; i < NICFREE; i++) {
        filsys.s_free[i] = 2048 + i;
    }
    
    // 初始化空闲inode数组
    for (i = 0; i < NICINOD; i++) {
        filsys.s_inode[i] = i + 2;
    }
    
    // 写入超级块
    fseek(fp, BLOCKSIZ, SEEK_SET);
    fwrite(&filsys, sizeof(struct filsys), 1, fp);
    
    // 初始化根目录inode
    memset(&inode_buf, 0, sizeof(struct inode));
    inode_buf.i_ino = 1;
    inode_buf.di_mode = DIDIR | 0755;
    inode_buf.di_nlink = 1;
    inode_buf.di_uid = 0;
    inode_buf.di_gid = 0;
    inode_buf.di_size = sizeof(struct direct) * 2;
    inode_buf.di_addr[0] = 1024;
    
    // 写入根目录inode
    fseek(fp, 2 * BLOCKSIZ + sizeof(struct inode), SEEK_SET);
    fwrite(&inode_buf, sizeof(struct inode), 1, fp);
    
    // 初始化根目录内容
    struct direct root_dir[2];
    strcpy(root_dir[0].d_name, ".");
    root_dir[0].d_ino = 1;
    strcpy(root_dir[1].d_name, "..");
    root_dir[1].d_ino = 1;
    
    fseek(fp, 1024 * BLOCKSIZ, SEEK_SET);
    fwrite(root_dir, sizeof(struct direct), 2, fp);
    
    fclose(fp);
    printf("文件系统格式化完成\n");
}

// 安装文件系统
void install() {
    int i;
    
    fd = fopen("filesystem", "r+b");
    if (fd == NULL) {
        printf("无法打开文件系统\n");
        return;
    }
    
    // 读取超级块
    fseek(fd, BLOCKSIZ, SEEK_SET);
    fread(&filsys, sizeof(struct filsys), 1, fd);
    
    // 初始化用户表
    for (i = 0; i < USERNUM; i++) {
        user[i].u_uid = 0;
        user[i].u_gid = 0;
        user[i].u_default_mode = DEFAULTMODE;
        for (int j = 0; j < NOFILE; j++) {
            user[i].u_ofile[j] = SYSOPENFILE + 1;
        }
    }
    
    // 初始化系统文件表
    for (i = 0; i < SYSOPENFILE; i++) {
        sys_ofile[i].f_count = 0;
        sys_ofile[i].f_flag = 0;
        sys_ofile[i].f_inode = NULL;
        sys_ofile[i].f_off = 0;
    }
    
    // 初始化密码表
    pwd[0].p_uid = 2116; strcpy(pwd[0].password, "dddd"); pwd[0].p_gid = 1;
    pwd[1].p_uid = 2117; strcpy(pwd[1].password, "bbbb"); pwd[1].p_gid = 1;
    pwd[2].p_uid = 2118; strcpy(pwd[2].password, "abcd"); pwd[2].p_gid = 1;
    pwd[3].p_uid = 2119; strcpy(pwd[3].password, "cccc"); pwd[3].p_gid = 1;
    pwd[4].p_uid = 2220; strcpy(pwd[4].password, "eeee"); pwd[4].p_gid = 1;
    
    // 初始化当前目录
    cur_path_inode = iget(1); // 根目录
    
    // 读取根目录内容
    fseek(fd, 1024 * BLOCKSIZ, SEEK_SET);
    fread(&dir, sizeof(struct direct), 2, fd);
    dir.size = 2;
    
    printf("文件系统安装完成\n");
}

// 获取inode
struct inode* iget(unsigned int dinodeid) {
    struct inode *newinode;
    int i;
    
    // 检查是否已在内存中
    for (i = 0; i < NHINO; i++) {
        if (hinode[i].i_ino == dinodeid && hinode[i].i_inode != NULL) {
            hinode[i].i_inode->i_count++;
            return hinode[i].i_inode;
        }
    }
    
    // 分配新的内存inode
    newinode = (struct inode*)malloc(sizeof(struct inode));
    if (newinode == NULL) {
        printf("内存不足\n");
        return NULL;
    }
    
    // 从磁盘读取inode
    fseek(fd, 2 * BLOCKSIZ + dinodeid * sizeof(struct inode), SEEK_SET);
    fread(newinode, sizeof(struct inode), 1, fd);
    newinode->i_count = 1;
    newinode->i_flag = 0;
    
    // 添加到hinode表
    for (i = 0; i < NHINO; i++) {
        if (hinode[i].i_inode == NULL) {
            hinode[i].i_ino = dinodeid;
            hinode[i].i_inode = newinode;
            break;
        }
    }
    
    return newinode;
}

// 释放inode
void iput(struct inode* pinode) {
    int i;
    
    if (pinode == NULL) return;
    
    pinode->i_count--;
    if (pinode->i_count == 0) {
        // 写回磁盘
        fseek(fd, 2 * BLOCKSIZ + pinode->i_ino * sizeof(struct inode), SEEK_SET);
        fwrite(pinode, sizeof(struct inode), 1, fd);
        
        // 从hinode表中移除
        for (i = 0; i < NHINO; i++) {
            if (hinode[i].i_inode == pinode) {
                hinode[i].i_ino = 0;
                hinode[i].i_inode = NULL;
                break;
            }
        }
        
        free(pinode);
    }
}

// 分配磁盘块
unsigned int balloc() {
    unsigned int free_block;
    
    if (filsys.s_nfree == 0) {
        return DISKFULL;
    }
    
    free_block = filsys.s_free[--filsys.s_nfree];
    
    // 写回超级块
    fseek(fd, BLOCKSIZ, SEEK_SET);
    fwrite(&filsys, sizeof(struct filsys), 1, fd);
    
    return free_block;
}

// 释放磁盘块
void bfree(unsigned int block_num) {
    if (filsys.s_nfree < NICFREE) {
        filsys.s_free[filsys.s_nfree++] = block_num;
        
        // 写回超级块
        fseek(fd, BLOCKSIZ, SEEK_SET);
        fwrite(&filsys, sizeof(struct filsys), 1, fd);
    }
}

// 分配inode
unsigned int ialloc() {
    unsigned int free_inode;
    
    if (filsys.s_ninode == 0) {
        return 0;
    }
    
    free_inode = filsys.s_inode[--filsys.s_ninode];
    
    // 写回超级块
    fseek(fd, BLOCKSIZ, SEEK_SET);
    fwrite(&filsys, sizeof(struct filsys), 1, fd);
    
    return free_inode;
}

// 释放inode
void ifree(unsigned int dinodeid) {
    if (filsys.s_ninode < NICINOD) {
        filsys.s_inode[filsys.s_ninode++] = dinodeid;
        
        // 写回超级块
        fseek(fd, BLOCKSIZ, SEEK_SET);
        fwrite(&filsys, sizeof(struct filsys), 1, fd);
    }
}

// 权限检查
int access_check(unsigned int user_id, struct inode* inode, unsigned short mode) {
    if (user[user_id].u_uid == 0) return 1; // root用户
    
    switch (mode) {
        case READ:
            return (inode->di_mode & DIREAD) ? 1 : 0;
        case WRITE:
            return (inode->di_mode & DIWRITE) ? 1 : 0;
        case EXEC:
            return (inode->di_mode & DIEXEC) ? 1 : 0;
        default:
            return 0;
    }
}

// 根据文件名查找inode编号
unsigned int namei(char* name) {
    int i;
    for (i = 0; i < dir.size; i++) {
        if (strcmp(dir.direct[i].d_name, name) == 0) {
            return dir.direct[i].d_ino;
        }
    }
    return 0;
}

// 在当前目录中为新文件分配目录项
unsigned short iname(char* name) {
    int i;
    for (i = 0; i < DIRNUM; i++) {
        if (dir.direct[i].d_ino == 0) {
            strcpy(dir.direct[i].d_name, name);
            return i;
        }
    }
    return 0;
}

// 显示目录内容
void _dir() {
    int i;
    struct inode *temp_inode;
    
    printf("\n目录内容:\n");
    printf("文件名\t\t类型\t大小\n");
    printf("--------------------------------\n");
    
    for (i = 0; i < dir.size; i++) {
        if (dir.direct[i].d_ino != 0) {
            temp_inode = iget(dir.direct[i].d_ino);
            printf("%-14s\t", dir.direct[i].d_name);
            
            if (temp_inode->di_mode & DIDIR) {
                printf("目录\t");
            } else {
                printf("文件\t");
            }
            
            printf("%d\n", temp_inode->di_size);
            iput(temp_inode);
        }
    }
    printf("\n");
}

// 创建目录
void mkdir(char* dirname) {
    unsigned int dirid;
    struct inode *inode;
    unsigned int block;
    struct direct buf[2];
    
    dirid = namei(dirname);
    if (dirid != 0) {
        printf("目录 %s 已存在\n", dirname);
        return;
    }
    
    // 分配inode
    dirid = ialloc();
    if (dirid == 0) {
        printf("无法分配inode\n");
        return;
    }
    
    inode = iget(dirid);
    inode->di_mode = DIDIR | 0755;
    inode->di_nlink = 1;
    inode->di_uid = user[current_user_id].u_uid;
    inode->di_gid = user[current_user_id].u_gid;
    inode->di_size = sizeof(struct direct) * 2;
    
    // 分配数据块
    block = balloc();
    if (block == DISKFULL) {
        ifree(dirid);
        iput(inode);
        printf("磁盘空间不足\n");
        return;
    }
    
    inode->di_addr[0] = block;
    
    // 创建 . 和 .. 目录项
    strcpy(buf[0].d_name, ".");
    buf[0].d_ino = dirid;
    strcpy(buf[1].d_name, "..");
    buf[1].d_ino = cur_path_inode->i_ino;
    
    // 写入目录内容
    fseek(fd, block * BLOCKSIZ, SEEK_SET);
    fwrite(buf, sizeof(struct direct), 2, fd);
    
    // 添加到当前目录
    int pos = iname(dirname);
    dir.direct[pos].d_ino = dirid;
    if (pos >= dir.size) dir.size = pos + 1;
    
    // 写回当前目录
    fseek(fd, cur_path_inode->di_addr[0] * BLOCKSIZ, SEEK_SET);
    fwrite(&dir, sizeof(struct direct), dir.size, fd);
    
    iput(inode);
    printf("目录 %s 创建成功\n", dirname);
}

// 切换目录
void chdir(char* dirname) {
    unsigned int dirid;
    struct inode *inode;
    
    dirid = namei(dirname);
    if (dirid == 0) {
        printf("目录 %s 不存在\n", dirname);
        return;
    }
    
    inode = iget(dirid);
    if (!(inode->di_mode & DIDIR)) {
        printf("%s 不是目录\n", dirname);
        iput(inode);
        return;
    }
    
    // 切换到新目录
    iput(cur_path_inode);
    cur_path_inode = inode;
    
    // 读取新目录内容
    fseek(fd, inode->di_addr[0] * BLOCKSIZ, SEEK_SET);
    dir.size = inode->di_size / sizeof(struct direct);
    fread(&dir, sizeof(struct direct), dir.size, fd);
    
    printf("已切换到目录 %s\n", dirname);
}

// 创建文件
unsigned short creat(unsigned int user_id, char* filename, unsigned short mode) {
    unsigned int di_ino;
    struct inode *inode;
    int i, j;
    
    di_ino = namei(filename);
    if (di_ino != 0) {
        printf("文件 %s 已存在\n", filename);
        return -1;
    }
    
    // 分配inode
    di_ino = ialloc();
    if (di_ino == 0) {
        printf("无法分配inode\n");
        return -1;
    }
    
    inode = iget(di_ino);
    inode->di_mode = mode;
    inode->di_nlink = 1;
    inode->di_uid = user[user_id].u_uid;
    inode->di_gid = user[user_id].u_gid;
    inode->di_size = 0;
    
    // 添加到当前目录
    int pos = iname(filename);
    dir.direct[pos].d_ino = di_ino;
    if (pos >= dir.size) dir.size = pos + 1;
    
    // 写回当前目录
    fseek(fd, cur_path_inode->di_addr[0] * BLOCKSIZ, SEEK_SET);
    fwrite(&dir, sizeof(struct direct), dir.size, fd);
    
    // 查找空闲的用户文件描述符
    for (i = 0; i < NOFILE; i++) {
        if (user[user_id].u_ofile[i] == SYSOPENFILE + 1) {
            break;
        }
    }
    
    // 查找空闲的系统文件表项
    for (j = 0; j < SYSOPENFILE; j++) {
        if (sys_ofile[j].f_count == 0) {
            break;
        }
    }
    
    if (i < NOFILE && j < SYSOPENFILE) {
        user[user_id].u_ofile[i] = j;
        sys_ofile[j].f_flag = WRITE;
        sys_ofile[j].f_count = 1;
        sys_ofile[j].f_inode = inode;
        sys_ofile[j].f_off = 0;
        return i;
    }
    
    iput(inode);
    return -1;
}

// 用户登录
int login(unsigned short uid, char* password) {
    int i, j;
    
    for (i = 0; i < PWDNUM; i++) {
        if (uid == pwd[i].p_uid && strcmp(password, pwd[i].password) == 0) {
            for (j = 0; j < USERNUM; j++) {
                if (user[j].u_uid == 0) break;
            }
            if (j == USERNUM) {
                printf("用户数已满\n");
                return 0;
            }
            
            user[j].u_uid = uid;
            user[j].u_gid = pwd[i].p_gid;
            user[j].u_default_mode = DEFAULTMODE;
            current_user_id = j;
            return 1;
        }
    }
    return 0;
}

// 用户登出
void logout(unsigned short uid) {
    int i;
    for (i = 0; i < USERNUM; i++) {
        if (user[i].u_uid == uid) {
            user[i].u_uid = 0;
            if (i == current_user_id) {
                current_user_id = -1;
            }
            break;
        }
    }
}

// 打开文件
unsigned short aopen(int user_id, char* filename, unsigned short openmode) {
    unsigned int di_ino;
    struct inode *inode;
    int i, j;
    
    di_ino = namei(filename);
    if (di_ino == 0) {
        printf("文件不存在: %s\n", filename);
        return -1;
    }
    
    inode = iget(di_ino);
    if (!access_check(user_id, inode, openmode)) {
        iput(inode);
        printf("没有访问权限\n");
        return -1;
    }
    
    // 查找空闲的用户文件描述符
    for (i = 0; i < NOFILE; i++) {
        if (user[user_id].u_ofile[i] == SYSOPENFILE + 1) {
            break;
        }
    }
    
    // 查找空闲的系统文件表项
    for (j = 0; j < SYSOPENFILE; j++) {
        if (sys_ofile[j].f_count == 0) {
            break;
        }
    }
    
    if (i < NOFILE && j < SYSOPENFILE) {
        user[user_id].u_ofile[i] = j;
        sys_ofile[j].f_flag = openmode;
        sys_ofile[j].f_count = 1;
        sys_ofile[j].f_inode = inode;
        sys_ofile[j].f_off = 0;
        return i;
    }
    
    iput(inode);
    return -1;
}

// 写文件
unsigned int write_file(int fd_user, char* buf, unsigned int size) {
    struct inode *inode;
    unsigned int block_num, offset_in_block, bytes_written = 0;
    unsigned int current_block;
    char block_buf[BLOCKSIZ];
    
    if (fd_user < 0 || fd_user >= NOFILE || user[current_user_id].u_ofile[fd_user] == SYSOPENFILE + 1) {
        return 0;
    }
    
    int sys_fd = user[current_user_id].u_ofile[fd_user];
    if (sys_fd >= SYSOPENFILE || sys_ofile[sys_fd].f_count == 0) {
        return 0;
    }
    
    inode = sys_ofile[sys_fd].f_inode;
    
    while (bytes_written < size) {
        block_num = sys_ofile[sys_fd].f_off / BLOCKSIZ;
        offset_in_block = sys_ofile[sys_fd].f_off % BLOCKSIZ;
        
        if (block_num >= NADDR) {
            break;
        }
        
        if (inode->di_addr[block_num] == 0) {
            current_block = balloc();
            if (current_block == DISKFULL) {
                break;
            }
            inode->di_addr[block_num] = current_block;
        } else {
            current_block = inode->di_addr[block_num];
        }
        
        // 读取当前块
        fseek(fd, current_block * BLOCKSIZ, SEEK_SET);
        fread(block_buf, 1, BLOCKSIZ, fd);
        
        // 计算本次写入的字节数
        unsigned int bytes_to_write = BLOCKSIZ - offset_in_block;
        if (bytes_to_write > size - bytes_written) {
            bytes_to_write = size - bytes_written;
        }
        
        // 写入数据到缓冲区
        memcpy(block_buf + offset_in_block, buf + bytes_written, bytes_to_write);
        
        // 写回磁盘
        fseek(fd, current_block * BLOCKSIZ, SEEK_SET);
        fwrite(block_buf, 1, BLOCKSIZ, fd);
        
        bytes_written += bytes_to_write;
        sys_ofile[sys_fd].f_off += bytes_to_write;
        
        if (sys_ofile[sys_fd].f_off > inode->di_size) {
            inode->di_size = sys_ofile[sys_fd].f_off;
        }
    }
    
    return bytes_written;
}

// 读文件
unsigned int read_file(int fd_user, char* buf, unsigned int size) {
    struct inode *inode;
    unsigned int block_num, offset_in_block, bytes_read = 0;
    unsigned int current_block;
    char block_buf[BLOCKSIZ];
    
    if (fd_user < 0 || fd_user >= NOFILE || user[current_user_id].u_ofile[fd_user] == SYSOPENFILE + 1) {
        return 0;
    }
    
    int sys_fd = user[current_user_id].u_ofile[fd_user];
    if (sys_fd >= SYSOPENFILE || sys_ofile[sys_fd].f_count == 0) {
        return 0;
    }
    
    inode = sys_ofile[sys_fd].f_inode;
    
    while (bytes_read < size && sys_ofile[sys_fd].f_off < inode->di_size) {
        block_num = sys_ofile[sys_fd].f_off / BLOCKSIZ;
        offset_in_block = sys_ofile[sys_fd].f_off % BLOCKSIZ;
        
        if (block_num >= NADDR || inode->di_addr[block_num] == 0) {
            break;
        }
        
        current_block = inode->di_addr[block_num];
        
        // 读取块
        fseek(fd, current_block * BLOCKSIZ, SEEK_SET);
        fread(block_buf, 1, BLOCKSIZ, fd);
        
        // 计算本次读取的字节数
        unsigned int bytes_to_read = BLOCKSIZ - offset_in_block;
        if (bytes_to_read > size - bytes_read) {
            bytes_to_read = size - bytes_read;
        }
        if (bytes_to_read > inode->di_size - sys_ofile[sys_fd].f_off) {
            bytes_to_read = inode->di_size - sys_ofile[sys_fd].f_off;
        }
        
        // 复制数据到用户缓冲区
        memcpy(buf + bytes_read, block_buf + offset_in_block, bytes_to_read);
        
        bytes_read += bytes_to_read;
        sys_ofile[sys_fd].f_off += bytes_to_read;
    }
    
    return bytes_read;
}

// 关闭文件
void close_file(unsigned int user_id, unsigned short cfd) {
    struct inode *inode;
    
    if (cfd >= NOFILE || user[user_id].u_ofile[cfd] == SYSOPENFILE + 1) {
        return;
    }
    
    int sys_fd = user[user_id].u_ofile[cfd];
    inode = sys_ofile[sys_fd].f_inode;
    
    sys_ofile[sys_fd].f_count--;
    if (sys_ofile[sys_fd].f_count == 0) {
        sys_ofile[sys_fd].f_inode = NULL;
    }
    
    iput(inode);
    user[user_id].u_ofile[cfd] = SYSOPENFILE + 1;
}

// 删除文件
void delete_file(char* filename) {
    unsigned int dinodeid;
    struct inode *inode;
    
    dinodeid = namei(filename);
    if (dinodeid == 0) {
        printf("文件不存在: %s\n", filename);
        return;
    }
    
    inode = iget(dinodeid);
    
    // 释放数据块
    for (int i = 0; i < NADDR && inode->di_addr[i] != 0; i++) {
        bfree(inode->di_addr[i]);
    }
    
    // 从目录中删除
    for (int i = 0; i < dir.size; i++) {
        if (dir.direct[i].d_ino == dinodeid) {
            dir.direct[i].d_ino = 0;
            strcpy(dir.direct[i].d_name, "");
            break;
        }
    }
    
    // 写回目录
    fseek(fd, cur_path_inode->di_addr[0] * BLOCKSIZ, SEEK_SET);
    fwrite(&dir, sizeof(struct direct), dir.size, fd);
    
    inode->di_nlink--;
    if (inode->di_nlink == 0) {
        ifree(dinodeid);
    }
    
    iput(inode);
}

// 关闭系统
void halt() {
    int i, j;
    
    // 关闭所有打开的文件
    for (i = 0; i < USERNUM; i++) {
        if (user[i].u_uid != 0) {
            for (j = 0; j < NOFILE; j++) {
                if (user[i].u_ofile[j] != SYSOPENFILE + 1) {
                    close_file(i, j);
                }
            }
        }
    }
    
    // 写回超级块
    fseek(fd, BLOCKSIZ, SEEK_SET);
    fwrite(&filsys, sizeof(struct filsys), 1, fd);
    
    // 释放当前目录inode
    if (cur_path_inode) {
        iput(cur_path_inode);
    }
    
    // 关闭文件系统
    fclose(fd);
    
    printf("文件系统已关闭，再见！\n");
}

// 主程序
int main() {
    char command[256];
    char cmd[64], arg1[64], arg2[64], arg3[256];
    int args_count;
    
    printf("=== 简单文件系统模拟器 ===\n");
    printf("正在初始化文件系统...\n");
    
    // 检查文件系统是否存在
    FILE *test_fd = fopen("filesystem", "rb");
    if (test_fd == NULL) {
        printf("文件系统不存在，正在创建...\n");
        format();
    } else {
        fclose(test_fd);
        printf("发现已存在的文件系统。\n");
    }
    
    // 安装文件系统
    install();
    
    printf("\n可用用户: 2116(密码:dddd), 2117(密码:bbbb), 2118(密码:abcd), 2119(密码:cccc), 2220(密码:eeee)\n");
    printf("输入 'help' 查看可用命令\n\n");
    
    // 主循环
    while (1) {
        if (current_user_id == -1) {
            printf("未登录> ");
        } else {
            printf("[用户%d]> ", user[current_user_id].u_uid);
        }
        
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        // 移除换行符
        command[strcspn(command, "\n")] = 0;
        
        // 解析命令
        args_count = sscanf(command, "%s %s %s %[^\n]", cmd, arg1, arg2, arg3);
        
        if (strcmp(cmd, "help") == 0) {
            printf("\n可用命令:\n");
            printf("login <用户ID> <密码>  - 登录系统\n");
            printf("logout                 - 登出当前用户\n");
            printf("dir                    - 显示当前目录内容\n");
            printf("mkdir <目录名>         - 创建目录\n");
            printf("cd <目录名>            - 切换目录\n");
            printf("create <文件名>        - 创建文件\n");
            printf("open <文件名> <模式>   - 打开文件 (模式: 1=读, 2=写)\n");
            printf("write <文件描述符> <内容> - 写入文件\n");
            printf("read <文件描述符> <字节数> - 读取文件\n");
            printf("close <文件描述符>     - 关闭文件\n");
            printf("delete <文件名>        - 删除文件\n");
            printf("exit                   - 退出系统\n\n");
        }
        else if (strcmp(cmd, "exit") == 0) {
            halt();
            break;
        }
        else if (strcmp(cmd, "login") == 0) {
            if (args_count >= 3) {
                unsigned short uid = atoi(arg1);
                if (login(uid, arg2)) {
                    printf("登录成功！欢迎用户 %d\n", uid);
                } else {
                    printf("登录失败！\n");
                }
            } else {
                printf("用法: login <用户ID> <密码>\n");
            }
        }
        else if (current_user_id == -1) {
            printf("请先登录！使用: login <用户ID> <密码>\n");
        }
        else if (strcmp(cmd, "logout") == 0) {
            logout(user[current_user_id].u_uid);
            printf("已登出。\n");
        }
        else if (strcmp(cmd, "dir") == 0) {
            _dir();
        }
        else if (strcmp(cmd, "mkdir") == 0) {
            if (args_count >= 2) {
                mkdir(arg1);
            } else {
                printf("用法: mkdir <目录名>\n");
            }
        }
        else if (strcmp(cmd, "cd") == 0) {
            if (args_count >= 2) {
                chdir(arg1);
            } else {
                printf("用法: cd <目录名>\n");
            }
        }
        else if (strcmp(cmd, "create") == 0) {
            if (args_count >= 2) {
                int fd_ret = creat(current_user_id, arg1, DIFILE | DEFAULTMODE);
                if (fd_ret >= 0) {
                    printf("文件 '%s' 创建成功，文件描述符: %d\n", arg1, fd_ret);
                } else {
                    printf("文件创建失败！\n");
                }
            } else {
                printf("用法: create <文件名>\n");
            }
        }
        else if (strcmp(cmd, "open") == 0) {
            if (args_count >= 3) {
                unsigned short mode = atoi(arg2);
                int fd_ret = aopen(current_user_id, arg1, mode);
                if (fd_ret >= 0) {
                    printf("文件 '%s' 打开成功，文件描述符: %d\n", arg1, fd_ret);
                } else {
                    printf("文件打开失败！\n");
                }
            } else {
                printf("用法: open <文件名> <模式> (1=读, 2=写)\n");
            }
        }
        else if (strcmp(cmd, "write") == 0) {
            if (args_count >= 3) {
                int fd_user = atoi(arg1);
                int bytes = write_file(fd_user, arg2, strlen(arg2));
                if (bytes > 0) {
                    printf("写入 %d 字节到文件描述符 %d\n", bytes, fd_user);
                } else {
                    printf("写入失败！\n");
                }
            } else {
                printf("用法: write <文件描述符> <内容>\n");
            }
        }
        else if (strcmp(cmd, "read") == 0) {
            if (args_count >= 3) {
                int fd_user = atoi(arg1);
                int size = atoi(arg2);
                char *buffer = malloc(size + 1);
                if (buffer) {
                    int bytes = read_file(fd_user, buffer, size);
                    if (bytes > 0) {
                        buffer[bytes] = '\0';
                        printf("从文件描述符 %d 读取 %d 字节: %s\n", fd_user, bytes, buffer);
                    } else {
                        printf("读取失败或已到文件末尾！\n");
                    }
                    free(buffer);
                }
            } else {
                printf("用法: read <文件描述符> <字节数>\n");
            }
        }
        else if (strcmp(cmd, "close") == 0) {
            if (args_count >= 2) {
                int fd_user = atoi(arg1);
                close_file(current_user_id, fd_user);
                printf("文件描述符 %d 已关闭\n", fd_user);
            } else {
                printf("用法: close <文件描述符>\n");
            }
        }
        else if (strcmp(cmd, "delete") == 0) {
            if (args_count >= 2) {
                delete_file(arg1);
                printf("文件 '%s' 已删除\n", arg1);
            } else {
                printf("用法: delete <文件名>\n");
            }
        }
        else {
            printf("未知命令: %s\n", cmd);
            printf("输入 'help' 查看可用命令\n");
        }
    }
    
    return 0;
}