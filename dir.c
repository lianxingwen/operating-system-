#include <stdio.h>
#include <string.h>
#include "filesys.h"
_dir() /* _dir */ // 显示当前目录下的所有条目，包括文件和子目录的权限、大小及数据块信息
{
	unsigned int di_mode;
	int i, j, one;
	struct inode *temp_inode;
	printf("\nCURRENT DIRECTORY ..\n");
	for (i = 0; i < dir.size; i++) // 遍历目录项
	{
		if (dir.direct[i].d_ino != DIEMPTY) // 有效条目
		{
			printf("%DIRSIZs", dir.direct[i].d_name); // 打印文件名
			temp_inode = iget(dir.direct[i].d_ino);	  // 获取inode
			di_mode = temp_inode->di_mode;

			//打印9位权限
			for (j = 0; j < 9; j++) // 内循环的i改为j
			{
				one = di_mode % 2; // 取二进制位
				di_mode = di_mode / 2;
				if (one)
					printf("x");
				else
					printf("-");
			}

			if (temp_inode->di_mode && DIFILE == 1) // 是文件
			//(temp_inode->di_mode & DIFILE) == DIFILE
			{
				printf("%ld\n", temp_inode->di_size); // 文件大小
				printf("block chain:");
				for (i = 0; i < temp_inode->di_size / BLOCKSIZ + 1; i++) // 打印块链
					printf("%4d", temp_inode->di_addr[i]);
				printf("\n");
			}
			else // 是目录
				printf("<dir>\n");
			iput(temp_inode); // 释放inode
		}
	}
}

mkdir(char *dirname) /* mkdir新建一个目录 */
{
	int dirid, dirpos;

	struct inode *inode;

	struct direct buf[BLOCKSIZ / (DIRSIZ + 2)];
	unsigned int block;

	dirid = namei(dirname); // 检查目录名是否存在
	if (dirid != NULL)
	{
		inode = iget(dirid);
		if (inode->di_mode & DIDIR) // 同名目录已存在
			printf("\n%s directory already existed! ! \n");
		else
			printf("\n%s is a file name, &can't create a dir the same name", dirname);

		iput(inode);
		return;
	}

	dirpos = iname(dirname); // 分配目录项位置
	if (dirpos == 0)
	{
		printf("\nDirectory is full\n");
		return;
	} // 增加一个判断

	inode = ialloc(); // 分配新inode
	// inode->i_ino=dirid;这里直接去掉

	dir.direct[dirpos].d_ino = inode->i_ino; // 绑定目录项
	dir.size++;

	/*	fill the new dir buf */
	// 初始化新目录数据块（含"."和".."）
	strcpy_s(buf[0].d_name, sizeof(buf[0].d_name), ".");  // strcpy (buf[0].d_name,".");
	buf[0].d_ino = dirid;								  // 第一个指向自己
	strcpy_s(buf[1].d_name, sizeof(buf[1].d_name), ".."); // strcpy(buf[1].d_name,"..");
	buf[1].d_ino = cur_path_inode->i_ino;				  // 第二个指向父目录
	block = balloc();									  // 分配数据块

	if (block == DISKFULL)
	{
		ifree(inode); // 释放已分配的inode
		printf("\nNo free blocks available\n");
		return;
	} // 添加一个判断

	//写入磁盘
	fseek(fd, DATASTART + block * BLOCKSIZ, SEEK_SET);
	fwrite(buf, 1, BLOCKSIZ, fd);

	inode->di_size = 2 * (DIRSIZ + 2); // "."和".."大小
	inode->di_number = 1;
	inode->di_mode = user[user_id].u_default_mode;
	inode->di_uid = user[user_id].u_uid;
	inode->di_gid = user[user_id].u_gid;
	inode->di_addr[0] = block; // 数据块地址
	iput(inode);
	return;
}

chdir(char *dirname) /* chdir目录切换 */
{
	unsigned int dirid;
	struct inode *inode;
	unsigned short block;
	int i, j, low = 0, high = 0;
	dirid = namei(dirname); // 查找目标目录
	if (dirid == NULL)
	{
		printf("\n%s does not existed\n", dirname);
		return;
	}
	inode = iget(dirid);
	// 权限检查（用户是否有访问权限）
	if (!access(user_id, inode, user[user_id].u_default_mode))
	{
		printf("\nhas not access to the directory %s", dirname);
		iput(inode);
		return;
	}


	/* pack the current directory */
	// 打包当前目录（整理有效条目）
	//此处逻辑有误，应该将有效项前移，无效项后移
	for (i = 0; i < dir.size; i++)
	{
		for (j = 0; j < DIRNUM; j++)
			if (dir.direct[j].d_ino == 0)
				break;
		memcpy(&dir.direct[i], &dir.direct[j], DIRSIZ + 2);
		dir.direct[j].d_ino = 0;
	}
	//以下为修改后代码
	// int new_size = 0; // 记录整理后的有效条目数
	// for (int i = 0; i < DIRNUM; i++)
	// { // 遍历所有目录条目
	// 	if (dir.direct[i].d_ino != 0)
	// 	{ // 有效条目
	// 		if (i != new_size)
	// 		{ // 非连续有效条目需要前移
	// 			memcpy(&dir.direct[new_size], &dir.direct[i], DIRSIZ + 2);
	// 			dir.direct[i].d_ino = 0; // 原位置标记为空
	// 		}
	// 		new_size++; // 更新有效条目数
	// 	}
	// }
	// dir.size = new_size; // 更新目录有效条目总数

	/*	write back the current directory */
	// 释放旧目录数据块
	for (i = 0; i < cur_path_inode->di_size / BLOCKSIZ + 1; i++)
	{
		bfree(cur_path_inode->di_addr[i]);
	}
	// 写入新目录内容到磁盘
	for (i = 0; i < dir.size; i += BLOCKSIZ / (DIRSIZ + 2))
	{

		block = balloc();
		cur_path_inode->di_addr[i] = block;
		fseek(fd, DATASTART + block * BLOCKSIZ, SEEK_SET);
		fwrite(&dir.direct[j], 1, BLOCKSIZ, fd);
		//是否应该为i？
	}
	cur_path_inode->di_size = dir.size * (DIRSIZ + 2);
	iput(cur_path_inode);
	// 切换到新目录
	cur_path_inode = inode;

	/*	read the change dir from disk */
	// 加载新目录内容到内存
	j = 0;

	for (i = 0; i < inode->di_size / BLOCKSIZ + 1; i++)
	{
		fseek(fd, DATASTART + inode->di_addr[i] * BLOCKSIZ, SEEK_SET);
		fread(&dir.direct[j], 1, BLOCKSIZ, fd);
		j += BLOCKSIZ / (DIRSIZ + 2); // 计算目录项偏移
	};

	return;
}