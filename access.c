/*访问控制函数access()(文件名access.c)*/
// 判别用户对文件是否拥有某种特定访问权限（read,write,excute）

//修改:不同模式下对于权限的判断顺序有问题
//应先判断所有用户，再判断所有断组，再判断其他
#include <stdio.h>
#include "FILESYS.H"

unsigned int access(unsigned int user_id, struct inode *inode, unsigned short mode)
{
	switch (mode)
	{
	case READ:
		if ((inode->di_mode & UDIREAD) && (user[user_id].u_uid == inode->di_uid))
			return 1;
		if ((inode->di_mode & GDIREAD) && (user[user_id].u_gid == inode->di_gid))
			return 1;
		if (inode->di_mode & ODIREAD)
			return 1;
		return 0;

	case WRITE:
		if ((inode->di_mode & UDIWRITE) &&
			(user[user_id].u_uid == inode->di_uid))
			return 1;
		if ((inode->di_mode & GDIWRITE) && (user[user_id].u_gid == inode->di_gid))
			return 1;
		if (inode->di_mode & ODIWRITE)
			return 1;
		return 0;

	case EXICUTE:
		if ((inode->di_mode & UDIEXICUTE) &&
			(user[user_id].u_uid == inode->di_uid))
			return 1;
		if ((inode->di_mode & GDIEXICUTE) &&
			(user[user_id].u_gid == inode->di_gid))
			return 1;
		if (inode->di_mode & ODIEXICUTE)
			return 1;
		return 0;
	defualt:
		return 0;
	}
}