/*文件关闭函数close( )(文件名close.c)*/
#include <stdio.h>
#include "filesys.h"
void close (unsigned int user_id,unsigned short cfd)
{
	struct inode *inode;
	// 1. 获取用户文件描述符对应的系统级文件表项中的 inode
	inode = sys_ofile[user[user_id].u_ofile[cfd]].f_inode;

	// 引用计数顺序错误
	//风险：iput(inode) 可能已释放 inode（当 i_count 减到0时），
	//后续操作 sys_ofile[...].f_count-- 可能访问已释放内存，
	//导致崩溃或数据损坏。修正：应先减少 f_count，再调用 iput
	sys_ofile[user[user_id].u_ofile[cfd]].f_count--;
	//系统文件表项引用减1
	iput(inode);
	// 释放inode资源（当引用计数为0时触发实际释放）
	user[user_id].u_ofile[cfd]=SYSOPENFILE + 1;
	// 将用户文件描述符标记为SYSOPENFILE + 1（超出有效范围的无效值）
}