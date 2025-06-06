/* 磁盘块分配与释放函数*/
//正序填充还是倒序填充?
#include <stdio.h>
#include <string.h>
#include "FILESYS.H"

static unsigned int block_buf[BLOCKSIZ]; // 静态缓冲区，主要用于临时存储从磁盘读取或写入的块数据
unsigned int balloc()
{
	unsigned int free_block, free_block_num;
	unsigned int	i;
	if (filsys.s_nfree==0)
	{
	printf ("\nDisk Full!!! \n");
	return DISKFULL;
};

//取出当前空闲块
	free_block=filsys.s_free[filsys.s_pfree];

	if (filsys.s_pfree==NICFREE-1)//若s_free耗尽，读取下一组空闲块
	{
		//添加了一个fseek，作用暂未知
	    fseek(fd, free_block * BLOCKSIZ, SEEK_SET);
		//

		//fread(block_buf, 1, BLOCKSIZ, fd);//参数设置错误，增加错误处理
		if (fread(block_buf, BLOCKSIZ, 1, fd) != 1)
		{
			printf("Error reading block %u\n", free_block);
			return DISKFULL;
		}

		// 解析块内容，后续空闲块数量
		//free_block_num = block_buf[NICFREE];
		free_block_num = (free_block_num > NICFREE) ? NICFREE : free_block_num;//防溢出
		//将后续块号填充至s_free
		//倒序填充似乎错误
		for (i=0; i<free_block_num; i++)
		{
			filsys.s_free[NICFREE-1-i]=block_buf[i];
			}
		filsys.s_pfree = NICFREE - free_block_num;
		// for (int i = 0; i < free_block_num; i++)
		// {
		// 	unsigned int block_num = *((unsigned int *)(block_buf + sizeof(unsigned int) + i * sizeof(unsigned int)));
		// 	filsys.s_free[i] = block_num;
		// }
		// filsys.s_nfree = free_block_num;
		// filsys.s_pfree = 0; // 重置指针到数组头部
	}
	else {
		filsys.s_pfree++;
		filsys.s_nfree--;
	}//else逻辑未打括号
	filsys.s_fmod=SUPDATE;
	return free_block;
}

// void bfree(unsigned int block_num)
// {
// 	int i;
// 	// if (filsys.s_pfree == 0) s_free 已满的判断逻辑错误
// 	if (filsys.s_pfree >= NICFREE)
// 	//  若 s_free 已满，将当前 s_free 写入待释放块
// 	{
// 		block_buf[NICFREE]=NICFREE;
// 		for (i=0; i<NICFREE; i++)
// 		{
// 			block_buf[i]=filsys.s_free[NICFREE-1-i];
// 		}
// 		filsys.s_pfree=NICFREE-1;
// 	}
// 	fwrite(block_buf, 1, BLOCKSIZ, fd);
// 	filsys.s_nfree++;
// 	filsys.s_fmod=SUPDATE;
// }

//deepseek生成，逻辑看不懂
void bfree(unsigned int block_num)
{
	// 若超级块的s_free数组已满（指针超过最大索引）
	if (filsys.s_pfree >= NICFREE)
	{
		// 1. 将当前s_free数组内容写入待释放的磁盘块block_num
		memset(block_buf, 0, BLOCKSIZ); // 清空缓冲区

		// 将空闲块数量写入block_buf[0]（NICFREE是数组容量）
		*((unsigned int *)block_buf) = NICFREE;

		// 将s_free数组中的块号反向拷贝到block_buf（兼容原逻辑）
		for (int i = 0; i < NICFREE; i++)
		{
			*((unsigned int *)block_buf + 1 + i) = filsys.s_free[NICFREE - 1 - i];
		}

		// 定位到block_num的磁盘位置并写入数据
		fseek(fd, block_num * BLOCKSIZ, SEEK_SET);
		fwrite(block_buf, BLOCKSIZ, 1, fd);

		// 2. 重置指针到数组起始位置
		filsys.s_pfree = 0;
	}

	// 3. 将待释放的块号block_num加入s_free数组
	filsys.s_free[filsys.s_pfree] = block_num;
	filsys.s_pfree++;		 // 指针后移
	filsys.s_nfree++;		 // 空闲块总数增加
	filsys.s_fmod = SUPDATE; // 标记超级块需要写回磁盘
}