
#include "../include/FileSystem.h"
#include "../include/Kernel.h"

FileSystem::FileSystem()
{

}

FileSystem::~FileSystem()
{

}

void FileSystem::LoadSuperBlock()
{
	Kernel* k = Kernel::getInstance();
	BufferManager* bufMgr = k->getBufMgr(); 
	SuperBlock* spb = k->getSuperBlock();  /* 代替源码中的系统全局超级块SuperBlock对象 */
	Buf* pBuf;

	for (int i = 0; i < 2; i++)
	{
		int* p = (int*)spb + i * 128;
		pBuf = bufMgr->Bread(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);
		std::copy((int*)pBuf->b_addr,(int*)pBuf->b_addr+128, p);    /* 代替源码中Utility::DWordCopy*/
		bufMgr->Brelse(pBuf);
	}
}

void FileSystem::Update()
{
	SuperBlock* spb = Kernel::getInstance()->getSuperBlock();
	BufferManager* bufMgr = Kernel::getInstance()->getBufMgr();
	Buf* pBuf;

	/* 同步SuperBlock到磁盘 */
	if (spb->s_fmod == 0)
	{
		return;
	}

	/* 清SuperBlock修改标志 */
	spb->s_fmod = 0;

	/*
	* 为将要写回到磁盘上去的SuperBlock申请一块缓存，由于缓存块大小为512字节，
	* SuperBlock大小为1024字节，占据2个连续的扇区，所以需要2次写入操作。
	*/
	for (int i = 0; i < 2; i++)
	{
		/* 第一次p指向SuperBlock的第0字节，第二次p指向第512字节 */
		int* p = (int*)spb + i * 128;

		/* 将要写入到SUPER_BLOCK_SECTOR_NUMBER + i扇区中去 */
		pBuf = bufMgr->GetBlk(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);

		/* 将SuperBlock中第0 - 511字节写入缓存区 */
		copy(p,p+128, (int*)pBuf->b_addr);

		/* 将缓冲区中的数据写到磁盘上 */
		bufMgr->Bwrite(pBuf);
	}

	/* 同步修改过的内存Inode到对应外存Inode */
	InodeTable* k_InodeTable = Kernel::getInstance()->getInodeTable();
	k_InodeTable->UpdateInodeTable();

	/* 将延迟写的缓存块写到磁盘上 */
	bufMgr->Bflush();
}

 /* 采用栈式管理，若栈不为空，则直接分配；若栈顶指针为空，则遍历 DiskINode区，将找到的空闲节点压入栈中 */
Inode* FileSystem::IAlloc()
{
	SuperBlock* spb = Kernel::getInstance()->getSuperBlock();
	Buf* pBuf;
	Inode* pNode;

	BufferManager* bufMgr = Kernel::getInstance()->getBufMgr();
	Kernel* k = Kernel::getInstance();
	InodeTable* k_InodeTable = Kernel::getInstance()->getInodeTable();

	int ino;      /* 分配到的空闲外存Inode编号 */

	/* SuperBlock直接管理的空闲Inode索引表已空，必须到磁盘上搜索空闲Inode。*/
	if (spb->s_ninode <= 0)
	{
		/* 外存Inode编号从0开始，这不同于Unix V6中外存Inode从1开始编号 */
		/*这会导致父目录无法访问，我给他改回去了*/
		ino = 0;//这里仍然使用Unix V6的方式，从1开始编号

		/* 依次读入磁盘Inode区中的磁盘块，搜索其中空闲外存Inode，记入空闲Inode索引表 */
		for (int i = 0; i < spb->s_isize; i++)
		{
			pBuf = bufMgr->Bread(FileSystem::INODE_ZONE_START_SECTOR + i);

			/* 获取缓冲区首址 */
			int* p = (int*)pBuf->b_addr;

			/* 检查该缓冲区中每个外存Inode，若i_mode != 0，表示已经被占用 */
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
			{
				ino++;

				int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

				/* 该外存Inode已被占用，不能记入空闲Inode索引表 */
				if (mode != 0)
				{
					continue;
				}

				/*
				* 如果外存inode的i_mode==0，此时并不能确定
				* 该inode是空闲的，因为有可能是内存inode没有写到
				* 磁盘上,所以要继续搜索内存inode中是否有相应的项
				*/
				if (k_InodeTable->IsLoaded(ino) == -1)
				{
					/* 该外存Inode没有对应的内存拷贝，将其记入空闲Inode索引表 */
					spb->s_inode[spb->s_ninode++] = ino;

					/* 如果空闲索引表已经装满，则不继续搜索 */
					if (spb->s_ninode >= 100)
					{
						break;
					}
						
				}
			}

			/* 至此已读完当前磁盘块，释放相应的缓存 */
			bufMgr->Brelse(pBuf);

			/* 如果空闲索引表已经装满，则不继续搜索 */
			if (spb->s_ninode >= 100)
			{
				break;
			}
				
		}

		/* 如果在磁盘上没有搜索到任何可用外存Inode，返回NULL */
		if (spb->s_ninode <= 0)
		{
			return NULL;
		}
			
	}

	/*
	 * 上面部分已经保证，除非系统中没有可用外存Inode，
	 * 否则空闲Inode索引表中必定会记录可用外存Inode的编号。
	 */
	while (true)
	{
		/* 从索引表“栈顶”获取空闲外存Inode编号 */
		ino = spb->s_inode[--spb->s_ninode];

		/* 将空闲Inode读入内存 */
		pNode = k_InodeTable->IGet(ino);

		/* 未能分配到内存inode */
		if (pNode == NULL)
		{
			return NULL;
		}

		/* 如果该Inode空闲,清空Inode中的数据 */
		if (pNode->i_mode == 0)
		{
			pNode->Clean();
			/* 设置SuperBlock被修改标志 */
			spb->s_fmod = 1;
			return pNode;
		}
		else	/* 如果该Inode已被占用 */
		{
			k_InodeTable->IPut(pNode);
			continue;	/* while循环 */
		}
	}
	return NULL;
}

void FileSystem::IFree(int number)
{
	SuperBlock* spb = Kernel::getInstance()->getSuperBlock();
	/*
	 * 如果超级块直接管理的空闲外存Inode超过100个，
	 * 同样让释放的外存Inode散落在磁盘Inode区中。
	 */
	if (spb->s_ninode >= 100)
	{
		return;
	}
		
	spb->s_inode[spb->s_ninode++] = number;

	/* 设置SuperBlock被修改标志 */
	spb->s_fmod = 1;
}

/* 空闲盘块采用栈式管理，并且采用分组链式的索引方法 */
Buf* FileSystem::Alloc()
{
	int blkno;     /* 分配到的空闲磁盘块编号 */
	SuperBlock* spb = Kernel::getInstance()->getSuperBlock();
	Buf* pBuf;
	BufferManager* bufMgr = Kernel::getInstance()->getBufMgr();
	Kernel* k = Kernel::getInstance();

	/* 从索引表“栈顶”获取空闲磁盘块编号 */
	blkno = spb->s_free[--spb->s_nfree];

	/*
	 * 若获取磁盘块编号为零，则表示已分配尽所有的空闲磁盘块。
	 * 或者分配到的空闲磁盘块编号不属于数据盘块区域中(由BadBlock()检查)，
	 * 都意味着分配空闲磁盘块操作失败。
	 */
	if (blkno == 0)
	{
		spb->s_nfree = 0;
		k->error = Kernel::NOSPACE;
		return NULL;
	}

	/*
	 * 栈已空，新分配到空闲磁盘块中记录了下一组空闲磁盘块的编号,
	 * 将下一组空闲磁盘块的编号读入SuperBlock的空闲磁盘块索引表s_free[100]中。
	 */
	if (spb->s_nfree <= 0)
	{
		/* 读入该空闲磁盘块 */
		pBuf = bufMgr->Bread(blkno);

		/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		int* p = (int*)pBuf->b_addr;

		/* 首先读出空闲盘块数s_nfree */
		spb->s_nfree = *p++;

		/* 读取缓存中后续位置的数据，写入到SuperBlock空闲盘块索引表s_free[100]中 */
		std::copy(p,p+100, spb->s_free);

		/* 缓存使用完毕，释放以便被其它进程使用 */
		bufMgr->Brelse(pBuf);
	}

	/* 普通情况下成功分配到一空闲磁盘块(可能是记录了下一组空闲磁盘块的编号的空闲磁盘) */
	pBuf = bufMgr->GetBlk(blkno);	        /* 为该磁盘块申请缓存 */
	bufMgr->ClrBuf(pBuf);	                /* 清空缓存中的数据 */
	spb->s_fmod = 1;	                    /* 设置SuperBlock被修改标志 */

	return pBuf;
}

void FileSystem::Free(int blkno)
{
	SuperBlock* spb = Kernel::getInstance()->getSuperBlock();
	Buf* pBuf;
	BufferManager* bufMgr = Kernel::getInstance()->getBufMgr();

	/*
	 * 尽早设置SuperBlock被修改标志，以防止在释放
	 * 磁盘块Free()执行过程中，对SuperBlock内存副本
	 * 的修改仅进行了一半，就更新到磁盘SuperBlock去
	 */
	spb->s_fmod = 1;

	/*
	 * 如果先前系统中已经没有空闲盘块，
	 * 现在释放的是系统中第1块空闲盘块
	 */
	if (spb->s_nfree <= 0)
	{
		spb->s_nfree = 1;
		spb->s_free[0] = 0;   /* 使用0标记空闲盘块链结束标志 */
	}

	/* SuperBlock中直接管理空闲磁盘块号的栈已满 */
	if (spb->s_nfree >= 100)
	{
		/*
		 * 使用当前Free()函数正要释放的磁盘块，存放前一组100个空闲
		 * 磁盘块的索引表
		 */
		pBuf = bufMgr->GetBlk(blkno);

		/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		int* p = (int*)pBuf->b_addr;

		/* 首先写入空闲盘块数，除了第一组为99块，后续每组都是100块 */
		*p++ = spb->s_nfree;
		/* 将SuperBlock的空闲盘块索引表s_free[100]写入缓存中后续位置 */
		std::copy(spb->s_free,spb->s_free+100, p);

		spb->s_nfree = 0;
		/* 将存放空闲盘块索引表的“当前释放盘块”写入磁盘，即实现了空闲盘块记录空闲盘块号的目标 */
		bufMgr->Bwrite(pBuf);
	}
	spb->s_free[spb->s_nfree++] = blkno;	/* SuperBlock中记录下当前释放盘块号 */
	spb->s_fmod = 1;
}
