/*删除文件函数delete( )(文件名delete.c)*/
#include <stdio.h>
#include "FILESYS.H"
void delete_file(char *filename)
{
	unsigned int dinodeid;
	struct inode *inode;
	dinodeid = namei(filename);
	if (dinodeid != 0)
		inode = iget(dinodeid);
	else
	{
		printf("\ncan not find this file\n");
		return;
	}
	inode->di_number--;
	iput(inode);
} // 也许需要删除目录中的对应目录项？可能加个返回值会更好
//应该删除对应目录项，释放对应数据块