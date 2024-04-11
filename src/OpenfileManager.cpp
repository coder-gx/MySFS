
#include "../include/Kernel.h"


InodeTable::InodeTable()
{

}

InodeTable::~InodeTable()
{

}

/* 先直接查找该 DiskINode 是否有对应的内存 INode，若有，则返回该 INode；若没有，则申请分配一个内存 INode。*/
Inode* InodeTable::IGet(int inumber)
{
	Inode* pInode;
	Kernel* k = Kernel::getInstance();
	BufferManager* bufMgr = Kernel::getInstance()->getBufMgr();

	while (true)
	{
		/* 检查编号为inumber的外存Inode是否有内存拷贝 */
		int index = this->IsLoaded(inumber);
		if (index >= 0)	/* 找到内存拷贝 */
		{
			pInode = &(this->m_Inode[index]);
			pInode->i_count++;
			return pInode;
		}
		else	/* 没有Inode的内存拷贝，则分配一个空闲内存Inode */
		{
			//cout<<555<<endl;
			pInode = this->GetFreeInode();
			/* 若内存Inode表已满，分配空闲Inode失败 */
			if (pInode == NULL)
				return NULL;
			else    /* 分配空闲Inode成功，将外存Inode读入新分配的内存Inode */
			{
				/* 设置新的外存Inode编号，增加引用计数 */
				pInode->i_number = inumber;
				pInode->i_count++;
				/* 将该外存Inode读入缓冲区 */
				Buf* pBuf = bufMgr->Bread(FileSystem::INODE_ZONE_START_SECTOR + (inumber-1) / FileSystem::INODE_NUMBER_PER_SECTOR);
				/* 将缓冲区中的外存Inode信息拷贝到新分配的内存Inode中 */
				pInode->ICopy(pBuf, inumber);
				/* 释放缓存 */
				bufMgr->Brelse(pBuf);
				return pInode;
			}
		}
	}
	return NULL;
}

/* 先直接释放对应的 DiskINode，然后将 INode 更新拷贝至 DiskINode，最后减少该INode 的引用计数。*/
void InodeTable::IPut(Inode* pNode)
{
	FileSystem* fileSys = Kernel::getInstance()->getFileSys();
	/* 当前进程为引用该内存Inode的唯一进程，且准备释放该内存Inode */
	if (pNode->i_count == 1)
	{
		/* 该文件已经没有目录路径指向它 */
		if (pNode->i_nlink <= 0)
		{
			/* 释放该文件占据的数据盘块 */
			pNode->ITrunc();
			pNode->i_mode = 0;
			/* 释放对应的外存Inode */
			fileSys->IFree(pNode->i_number);
		}

		/* 更新外存Inode信息 */
		pNode->IUpdate();
		/* 清除内存Inode的所有标志位 */
		pNode->i_flag = 0;
		/* 这是内存inode空闲的标志之一，另一个是i_count == 0 */
		pNode->i_number = -1;
	}
	/* 减少内存Inode的引用计数 */
	pNode->i_count--;
}

void InodeTable::UpdateInodeTable()
{
	for (int i = 0; i < InodeTable::NINODE; i++)
	{
		/* count不等于0，count == 0意味着该内存Inode未被任何打开文件引用，无需同步 */
		if (this->m_Inode[i].i_count != 0)
			this->m_Inode[i].IUpdate();
	}
}

int InodeTable::IsLoaded(int inumber)
{
	/* 寻找指定外存Inode的内存拷贝 */
	for (int i = 0; i < InodeTable::NINODE; i++)
	{
		if (this->m_Inode[i].i_number == inumber && this->m_Inode[i].i_count != 0)
			return i;
	}
	return -1;
}

Inode* InodeTable::GetFreeInode()
{
	/* 如果该内存Inode引用计数为零，则该Inode表示空闲 */
	for (int i = 0; i < InodeTable::NINODE; i++)
	{
		if (this->m_Inode[i].i_count == 0)
			return &(this->m_Inode[i]);
	}
	return NULL;      /* 寻找失败 */
}



File* OpenFileTable::FAlloc()
{
	int fd;
	Kernel* k = Kernel::getInstance();

	/* 在内核打开文件描述符表中获取一个空闲项 */
	fd = k->getOpenFiles()->AllocFreeSlot();

	if (fd < 0)  /* 如果寻找空闲项失败 */
	{
		return NULL;
	}

	for (int i = 0; i < OpenFileTable::NFILE; i++)
	{
		/* f_count==0表示该项空闲 */
		if (this->m_File[i].f_count == 0)
		{
			/* 建立描述符和File结构的勾连关系 */
			k->getOpenFiles()->SetF(fd, &this->m_File[i]);
			/* 增加对file结构的引用计数 */
			this->m_File[i].f_count++;
			/* 清空文件读、写位置 */
			this->m_File[i].f_offset = 0;
			return (&this->m_File[i]);
		}
	}
	return NULL;
}

void OpenFileTable::CloseF(File* pFile)
{
	/* 引用计数f_count将减为0，则释放File结构 */
	if (pFile->f_count <= 1)
	{
		Kernel::getInstance()->getInodeTable()->IPut(pFile->f_inode);
	}

	/* 引用当前File的进程数减1 */
	pFile->f_count--;
}

