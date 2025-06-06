//i节点分配与释放函数
//磁盘i节点的分配与释放（当一个新文件被建立的时候，在给该文件分配磁盘存储区之前，
//应为该文件分配存放该文件说明信息的磁盘i节点，
//当从文件系统中删除某个文件时，应首先删除它的磁盘i节点项。）

/* i节点分配与释放函数*/
#include <stdio.h>
#include "filesys.h"
static struct dinode block_buf[BLOCKSIZ / DINODESIZ];
struct inode *ialloc()
{
	struct inode *temp_inode;
	unsigned int cur_di;
	int i, count, block_end_flag;

	if (filsys.s_pinode == NICINOD) // 内存中空闲i节点耗尽，需要从磁盘加载更多空闲i节点
	{
		i = 0;
		count = 0;
		block_end_flag = 1;
		filsys.s_pinode = NICINOD - 1;
		cur_di = filsys.s_rinode;// 当前扫描的磁盘i节点号
		while ((count < NICINOD) || (count <= filsys.s_ninode)) // 循环直到收集到 NICINOD 个空闲 i 节点，或当前系统中剩余的 i 节点数（s_ninode）不多于 count。
		{
			if (block_end_flag) // 为1说明上一个块处理完了
			{
				fseek(fd, DINODESTART + cur_di * DINODESIZ, SEEK_SET);
				fread(block_buf, 1, BLOCKSIZ, fd);
				block_end_flag = 0;
				i = 0;
			}
			while (block_buf[i].di_mode != DIEMPTY) // 此处判断错误，应该为当前磁盘块被占用时，跳过它到下一个节点，直到找到空闲的
			{
				cur_di++;
				i++;
			}
			if (i == BLOCKSIZ / DINODESIZ)
				block_end_flag = 1; // 此处判断也有问题，应为一个块所有节点都查看过了，再去下一个块，而不是空闲块最大数量
			else
			{
				filsys.s_inode[filsys.s_pinode--] = cur_di;
				count++;
			}
		}
		filsys.s_rinode = cur_di;
	}
	temp_inode = iget(filsys.s_inode[filsys.s_pinode]);
	fseek(fd, DINODESTART + filsys.s_inode[filsys.s_pinode] * DINODESIZ, SEEK_SET); // 定位到分配的那个节点
	fwrite(&temp_inode->di_number, 1, sizeof(struct dinode), fd);					// 将节点内容写入到那个位置
	filsys.s_pinode++;
	filsys.s_ninode--;
	filsys.s_fmod = SUPDATE;
	return temp_inode;
}

ifree(unsigned dinodeid) /* ifree */
{
	filsys.s_ninode++;
	if (filsys.s_pinode != NICINOD) /* notfull */
	{
		filsys.s_inode[filsys.s_pinode] = dinodeid;
		filsys.s_pinode++;
	}
	else /* full */
	{
		if (dinodeid < filsys.s_rinode)
		{
			filsys.s_inode[NICINOD] = dinodeid;
			filsys.s_rinode = dinodeid;
		}
	}
}