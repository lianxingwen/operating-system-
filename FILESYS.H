#include <stdio.h>
#pragma warning(disable:4996)
#define _CRT_SECURE_NO_WARNINGS
#define BLOCKSIZ 512 // 每块大小
#define SYSOPENFILE 40 // 系统打开文件表最大项数
#define DIRNUM 128	   // 每个目录所包含的最大目录项数（文件数）
#define DIRSIZ 14	   // 每个目录项名字部分所占字节数，另加i节点号2个字节
#define PWDSIZ 12	   // 口令字
#define PWDNUM 32	   // 最多可设32个口令登录
#define NOFILE 20	   // 每个用户最多可打开20个文件，即用户打开文件最大次数
#define NADDR 10	   // 每个i节点最多指向10块，addr[0]~addr[9]
#define NHINO 128 // 共128个Hash链表，提供索引i节点（必须为2的幂）
#define USERNUM 10	  // 最多允许10个用户登录
#define DINODESIZ 32  // 每个磁盘i节点所占字节
#define DINODEBLK 32  // 所有磁盘i节点共占32个物理块
#define FILEBLK 512	  // 共有512个目录文件物理块
#define NICFREE 50	  // 超级块中空闲块数组的最大块数
#define NICINOD 50	  // 超级块中空闲节点的最大块数
#define DINODESTART 2 * BLOCKSIZ		  // i节点起始地址
#define DATASTART (2 + DINODEBLK) * BLOCKSIZ // 目录、文件区起始地址

//文件类型标识
#define DIEMPTY     00000//空类型(未定义类型)
#define DIFILE      01000//普通文件类型
#define DIDIR       02000//目录类型

//权限控制:用户/组/其他 +读/写/执行
//用户权限
#define UDIREAD     00001//用户读
#define UDIWRITE    00002//用户写
#define UDIEXICUTE  00004//用户执行
//组权限
#define GDIREAD     00010//组读
#define GDIWRITE    00020//组写
#define GDIEXICUTE  00040//组执行
//其他权限
#define ODIREAD     00100//其他读
#define ODIWRITE    00200//其他写
#define ODIEXICUTE  00400//其他执行

//文件操作模式
#define READ        1//读模式
#define WRITE       2//写模式
#define EXICUTE     3//执行模式
#define DEFAULTMODE 00777//默认权限
#define IUPDATE     00002
#define SUPDATE     00001
#define FREAD       00001//只读
#define FWRITE      00002//只写
#define FAPPEND     00004//追加模式
#define DISKFULL    65535//磁盘已满错误码
#define SEEK_SET    0//文件定位起点

struct inode {
	struct inode* i_forw;//链表指针
	struct inode* i_back;//链表指针
	char i_flag;//状态标志
	unsigned int i_ino;//编号
	unsigned int i_count;//计数器(引用次数)
	unsigned short di_number;//是否修改
	unsigned short di_mode;//文件类型和权限
	unsigned short di_uid;//所有者的用户ID
	unsigned short di_gid;//所有者的组ID
	unsigned short di_size;//文件大小
	unsigned int di_addr[NADDR];//数据块地址数组
		};//内存中的索引节点

struct dinode{
	unsigned short di_number;	 // 硬链接数
	unsigned short di_mode;		 // 文件类型和权限
	unsigned short di_uid;		 // 所有者的用户ID
	unsigned short di_gid;		 // 所有者的组ID
	unsigned short di_size;		 // 文件大小
	unsigned int di_addr[NADDR]; // 数据块地址数组
		};//磁盘中的索引节点

struct direct{
		char d_name[DIRSIZ];//文件名
		unsigned int d_ino;//对应文件的inode编号
		};//目录项

struct filsys{
	unsigned short s_isize; // Inode 区域占用的磁盘块数
	unsigned long s_fsize;	// 文件系统总磁盘块数
	unsigned int s_nfree;	// 空闲块数量
	unsigned short s_pfree;//空闲块指针
	//指向 s_free[NICFREE] 数组中 当前可分配的空闲磁盘块位置
	unsigned int s_free[NICFREE]; // 空闲块列表
	unsigned int s_ninode;//空闲inode数量。
	unsigned short s_pinode;	   // 空闲 inode 指针
	//指向 s_inode[NICINOD] 数组中 当前可分配的空闲 inode 位置
	unsigned int s_inode[NICINOD]; // 空闲inode列表
	unsigned int s_rinode;		   // 保留 inode 编号
	// 记录 最近被释放的 inode 编号，用于优化 inode 回收和重用
	char s_fmod;//脏标志(是否要写回磁盘)
		};//全局信息管理(超级块)

struct pwd{
		unsigned short p_uid;//用户ID
		unsigned short p_gid;//组ID
		char password[PWDSIZ];//密码
		};//用户密码条目

struct dir{
		struct direct direct[DIRNUM];//目录项数组
		int size;//目录项数量
		};//目录文件

struct hinode{
		struct inode *i_forw;//哈希冲突链表的指针
		};//inode哈希表项

struct file{
		char f_flag;//打开模式
		unsigned int f_count;//引用计数
		struct inode *f_inode;//指向内存中的inode
		unsigned long f_off;//当前读写偏移量
		};//打开的文件(进程打开的文件实例)

struct user{
		unsigned short u_default_mode;//创建文件的默认权限
		unsigned short u_uid;//进程的有效用户ID
		unsigned short u_gid;//进程的组ID
		unsigned short u_ofile[NOFILE];//进程打开的文件描述符列表
		};//进程用户上下文
//进程运行时的用户权限和资源状态

extern struct hinode hinode[NHINO];
extern struct dir dir;
extern struct file sys_ofile[SYSOPENFILE];
extern struct filsys filsys;
extern struct pwd pwd[PWDNUM];
extern struct user user[USERNUM];
extern        FILE *fd;
extern struct inode *cur_path_inode;
extern int    user_id;
extern struct inode *iget(unsigned int dinodeid);
extern  void  iput(struct inode* pinode);
extern unsigned int balloc();
extern  void  bfree(unsigned int block_num);
extern struct inode * ialloc();
extern  void ifree(unsigned dinodeid);
extern unsigned int namei(char* name);
extern unsigned short iname(char* name);
extern unsigned int access(unsigned int user_id,struct inode* inode,unsigned short mode);
extern  void   _dir();
extern  void   mkdir(char* dirname);
extern  void   chdir(char* dirname);
extern unsigned short aopen(int user_id, char* filename, unsigned short openmode);
extern unsigned short    creat(unsigned int user_id, char* filename, unsigned short mode);
extern unsigned int read(int fd, char* buf, unsigned int size);
extern unsigned int write(int fd, char* buf, unsigned int size);
extern int    login(unsigned short uid, char* passwd);
extern int    logout(unsigned short uid);
extern  void     install();
extern  void    format();
extern  void    close(unsigned int user_id, unsigned short cfd);
extern  void      halt();
extern  void      delete(char* filename);