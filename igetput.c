	/*获取释放i节点内容程序iget()/iput()*/
#include <stdio.h>
#include "filesys.h"

struct inode * iget (unsigned int dinodeid)    /* iget( ) */
{
	int existed=0, inodeid;
	long addr;	
	struct inode *temp, * newinode;
	inodeid=dinodeid % NHINO;
	//计算哈希桶索引

	//检查哈希桶是否为空
	if (hinode[inodeid].i_forw==NULL) existed = 0;//为空则不存在
	else
	  {
		//否则遍历链表寻找目标inode
		temp = hinode[inodeid].i_forw;
			while (temp)
			 {
				if (temp->i_ino==dinodeid)//找到对应inode
				// 命中,此处应将inodeid改为dinodeid
				/*existed */
				{
					existed = 1;
					temp->i_count++; // 引用计数增加
					return temp;
				}
				else    /*not existed */
					temp =temp->i_forw;//遍历下一个节点
			    };
	  }
	//磁盘读取流程
	/*	1. calculate the addr of the dinode in the file sys column */
	addr = DINODESTART + dinodeid * DINODESIZ;//计算磁盘偏移地址
	/*	2. malloc the new mode */
	newinode = (struct inode *)malloc(sizeof(struct inode));//申请内存空间
   if (newinode == NULL) {
       perror("Memory allocation failed");
       return NULL; // Or handle the error appropriately
   }
	/*	3.read the dinode to the mode */
	//读取磁盘数据
   fseek(fd, addr, SEEK_SET); // 定位磁盘位置
   fread(&(newinode->di_number), DINODESIZ, 1, fd); // 读取inode数据

   // 链表操作
   /* 4.put it into hinode[inodeid] queue */
   // 插入哈希链表
   newinode->i_forw = hinode[inodeid].i_forw;
   newinode->i_back = newinode;
   newinode->i_forw->i_back = newinode;
   hinode[inodeid].i_forw = newinode;
   /* 5.initialize the mode */
   // 初始化数据
   newinode->i_count = 1;						   // 初始引用计数
   newinode->i_flag = 0; /* flag for not update */ // 脏数据标记
   newinode->i_ino = dinodeid;					   // 设置inode编号
   return newinode;
}

void iput(struct inode* pinode) /* iput ( ) */

{
long addr;
unsigned int block_num;
int i;
if (pinode->i_count>1)
{
pinode->i_count--;
return;
}
else
{
if (pinode->di_number !=0)
{
/*write back the mode */
addr =DINODESTART + pinode->i_ino * DINODESIZ;
fseek(fd, addr, SEEK_SET);
fwrite (&pinode->di_number, DINODESIZ, 1 ,fd);
}
else
{
/*	rm the mode & the block of the file in the disk */
block_num=pinode->di_size/BLOCKSIZ;
for (i=0;i<block_num; i++)
{
	balloc(pinode->di_addr[i]);

}
		ifree(pinode->i_ino);
};

	/*	free the mode in the memory */
	if (pinode->i_forw==NULL)
		pinode->i_back->i_forw= NULL;
	else{
	pinode->i_forw->i_back=pinode->i_back;
	pinode->i_back->i_forw=pinode->i_forw;
	};
	free (pinode);
};
}