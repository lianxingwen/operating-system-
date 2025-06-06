/*	文件创建函数creat()(creat.c)*/
#include <stdio.h>
#include "FILESYS.H"
unsigned short creat(unsigned int user_id, char *filename, unsigned short mode)
{
	unsigned int di_ith, di_ino;
	struct inode *inode;
	int i, j;
	di_ino = namei(filename); // ???
	if (di_ino != 0)		  /* already existed */
	{
		inode = iget(di_ino);
		if (access(user_id, inode, WRITE) == 0)
		{
			iput(inode);
			printf("\rcreat access not allowed \n");
			return -1;
		}
		/* free all the block of the old file */
		for (i = 0; i < inode->di_size / BLOCKSIZ + 1; i++)
		{
			bfree(inode->di_addr[i]);
		}

		/* to do: add code here to update the pointer of the sys_file */
		for (i = 0; i < SYSOPENFILE; i++) // 找到所有指向该文件（inode）的已打开系统文件表项，
			if (sys_ofile[i].f_inode == inode)
			{
				sys_ofile[i].f_off = 0; // 确保所有指向该节点地都从头开始读写（覆盖旧文件内容）
			}
		for (i = 0; i < NOFILE; i++)
			if (user[user_id].u_ofile[i] == SYSOPENFILE + 1)
			{ // 找到空闲的用户文件描述符槽位
				// user[user_id].u_uid=inode->di_uid;
				inode->di_uid = user[user_id].u_uid;
				// user[user_id].u_gid=inode->di_gid;
				inode->di_gid = user[user_id].u_gid;
				// 写反了
				//修改文件属主
				for (j = 0; j < SYSOPENFILE; j++)
					// 为当前用户分配一个系统级打开文件表项
					if (sys_ofile[j].f_count == 0) // 寻找f_count == 0（未被引用）的条目
					{
						user[user_id].u_ofile[i] = j; // 用户描述符i对应系统表项j
						sys_ofile[j].f_flag = mode;

						sys_ofile[j].f_count = 1;
						sys_ofile[j].f_inode = inode;
						sys_ofile[j].f_off = 0; // 添加了对系统打开文件表地修改
						break;					// 找到就退出
					}
				return i;
			}
	}
	else /* not existed before */ // 处理新建文件的情况
	{
		inode = ialloc();						 // 分配新inode
		di_ith = iname(filename);				 // 在目录中添加文件名条目
		dir.size++;								 // 目录项数量+1

		dir.direct[di_ith].d_ino = inode->i_ino; // 关联inode与目录项

		inode->di_mode = user[user_id].u_default_mode;
		inode->di_uid = user[user_id].u_uid;
		inode->di_gid = user[user_id].u_gid;
		inode->di_size = 0;
		inode->di_number = 0;

		for (i = 0; i < SYSOPENFILE; i++) // 找空闲系统表项
			if (sys_ofile[i].f_count == 0)
			{
				break;
			}

		for (j = 0; j < NOFILE; j++)  //找空闲用户表项
		if (user[user_id].u_ofile[j] == SYSOPENFILE + 1)
		{
			break;
		}
		//// 关联系统表项与用户表项
		user[user_id].u_ofile[j] = i;
		sys_ofile[i].f_flag = mode;
		sys_ofile[i].f_count = 1; // 此处将0改为1，表示该系统打开表表项已被使用
		sys_ofile[i].f_off = 0;
		sys_ofile[i].f_inode = inode; // 绑定inode
		return j; // 返回用户文件描述符
	}
	return -1; //????
}