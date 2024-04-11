#include "../include/FileManager.h"
#include "../include/Kernel.h"
#include <cstring>


/*
 * 功能：打开文件
 * 效果：建立打开文件结构，内存i节点开锁 、i_count 为正数（i_count ++）
 * */
void FileManager::Open()
{
	Inode* pInode;
	Kernel* k = Kernel::getInstance();

	pInode = this->NameI(NextChar, FileManager::OPEN);
	/* 没有找到相应的Inode */
	if (pInode == NULL)
		return;
	this->Open1(pInode, k->mode, 0);
}

/*
 * 功能：创建一个新的文件
 * 效果：建立打开文件结构，内存i节点开锁 、i_count 为正数（应该是 1）
 * */
void FileManager::Creat()
{
	Inode* pInode;
	Kernel* k = Kernel::getInstance();

	unsigned int newACCMode = k->mode & (Inode::IRWXU | Inode::IRWXG | Inode::IRWXO);

    //cout<<11<<endl;
	pInode = this->NameI(NextChar, FileManager::CREATE);
	
    //cout<<pInode->i_number<<endl;

	/* 没有找到相应的Inode，或NameI出错 */
	if (pInode == NULL)
	{
		//cout<<33<<endl;
		//cout<<k->error<<endl;
		if (k->error != Kernel::NO_ERROR)
			return;
		/* 创建Inode */
		pInode = this->MakNode(newACCMode & (~Inode::ISVTX));
		if (pInode == NULL)
		{
			return;
		}
		//cout<<44<<endl;
		/*
		 * 如果所希望的名字不存在，使用参数trf = 2来调用open1()。
		 * 不需要进行权限检查，因为刚刚建立的文件的权限和传入参数mode
		 * 所表示的权限内容是一样的。
		 */
		this->Open1(pInode, File::FWRITE, 2);
	}
	else
	{
		/* 如果NameI()搜索到已经存在要创建的文件，则清空该文件（用算法ITrunc()）。UID没有改变
		 * 原来UNIX的设计是这样：文件看上去就像新建的文件一样。然而，新文件所有者和许可权方式没变。
		 * 也就是说creat指定的RWX比特无效。
		 * 邓蓉认为这是不合理的，应该改变。
		 * 现在的实现：creat指定的RWX比特有效 */
		this->Open1(pInode, File::FWRITE, 1);
		pInode->i_mode |= newACCMode;
	}
}

/*
* trf == 0由open调用
* trf == 1由creat调用，creat文件的时候搜索到同文件名的文件
* trf == 2由creat调用，creat文件的时候未搜索到同文件名的文件，这是文件创建时更一般的情况
* mode参数：打开文件模式，表示文件操作是 读、写还是读写
*/
void FileManager::Open1(Inode* pInode, int mode, int trf)
{
	Kernel* k = Kernel::getInstance();

	/*
	 * 对所希望的文件已存在的情况下，即trf == 0或trf == 1进行权限检查
	 * 如果所希望的名字不存在，即trf == 2，不需要进行权限检查，因为刚建立
	 * 的文件的权限和传入的参数mode的所表示的权限内容是一样的。
	 */
	if (trf != 2 )
	{
		 if (mode & File::FREAD)
		{
			/* 检查读权限 */
			if(this->Access(pInode, Inode::IREAD)==1){
               k->error = Kernel::NOACCESS;
			}
		}
		if (mode & File::FWRITE)
		{
			/* 检查写权限 */
			if(this->Access(pInode, Inode::IWRITE)){
				 k->error = Kernel::NOACCESS;
			}
			/* open去写目录文件是不允许的 */
		if ((pInode->i_mode & Inode::IFMT) == Inode::IFDIR)
		{
			k->error = Kernel::ISDIR;
		}

		}
		
	}

	if (k->error != Kernel::NO_ERROR) {
		k->getInodeTable()->IPut(pInode);
		return;
	}

	/* 在creat文件的时候搜索到同文件名的文件，释放该文件所占据的所有盘块 */
	if (trf == 1)
	{
		pInode->ITrunc();
	}

	/* 分配打开文件控制块File结构 */
	File* pFile = k->getOpenFileTable()->FAlloc();
	if (pFile == NULL)
	{
		k->getInodeTable()->IPut(pInode);
		return;
	}
	/* 设置打开文件方式，建立File结构和内存Inode的勾连关系 */


	//unix v6++源码里新建的文件默认是只写模式不可读，这里对于Creat下的文件增加读权限
	if(trf!=0){
        pFile->f_flag = (File::FREAD | File::FWRITE);
	}
	else{
	    pFile->f_flag = mode & (File::FREAD | File::FWRITE);
	}
	pFile->f_inode = pInode;

	/* 不能创建目录文件！！！ */
	if (trf != 0 && k->isDir)
	{
		pInode->i_mode |= Inode::IFDIR;
	}
	return;
}

void FileManager::Close()
{
	Kernel* k = Kernel::getInstance();

	/* 获取打开文件控制块File结构 */
	File* pFile = k->getOpenFiles()->GetF(k->fd);
	if (pFile == NULL)
		return;
	/* 释放打开文件描述符fd，递减File结构引用计数 */
	k->getOpenFiles()->SetF(k->fd, NULL);
	/* 减少在系统打开文件表中File的引用计数 */
	k->getOpenFileTable()->CloseF(pFile);
}

void FileManager::Seek()
{
	File* pFile;
	Kernel* k = Kernel::getInstance();
	int fd = k->fd;

	pFile = k->getOpenFiles()->GetF(fd);
	if (NULL == pFile)
	{
		return;     /* 若FILE不存在，GetF有设出错码 */
	}

	int offset = k->offset;

	switch (k->mode)
	{
		/* 读写位置设置为offset */
	case 0:
		pFile->f_offset = offset;
		break;
		/* 读写位置加offset(可正可负) */
	case 1:
		pFile->f_offset += offset;
		break;
		/* 读写位置调整为文件长度加offset */
	case 2:
		pFile->f_offset = pFile->f_inode->i_size + offset;
		break;
	}
}

void FileManager::Read()
{
	/* 直接调用Rdwr()函数即可 */
	this->Rdwr(File::FREAD);
}

void FileManager::Write()
{
	/* 直接调用Rdwr()函数即可 */
	this->Rdwr(File::FWRITE);
}

void FileManager::Rdwr(enum File::FileFlags mode)
{
	//cout<<22<<endl;
	File* pFile;
	Kernel* k = Kernel::getInstance();

	/* 根据Read()/Write()的系统调用参数fd获取打开文件控制块结构 */
	pFile = k->getOpenFiles()->GetF(k->fd);
	if (pFile == NULL)
	{
		/* 不存在该打开文件，GetF已经设置过出错码，所以这里不需要再设置了 */
		return;
	}
	 if ((pFile->f_flag & mode) == 0)
	{
		//cout<<pFile->f_flag<<endl;
		k->error = Kernel::NOACCESS;
		return;
	}
    
	k->k_IOParam.m_Base = (char*)k->buf;         /* 目标缓冲区首址 */
	k->k_IOParam.m_Count = k->nbytes;            /* 要求读/写的字节数 */
	k->k_IOParam.m_Offset = pFile->f_offset;     /* 设置文件起始读位置 */
	//cout<<33<<endl;
	//cout<<k->k_IOParam.m_Base<<endl;
	//cout<<44<<endl;

   
	
	if (File::FREAD == mode)
		pFile->f_inode->ReadI();
	else
		pFile->f_inode->WriteI();
	/* 根据读写字数，移动文件读写偏移指针 */
	pFile->f_offset += (k->nbytes - k->k_IOParam.m_Count);
	/* 返回实际读写的字节数，修改存放系统调用返回值的核心栈单元 */
	k->callReturn = k->nbytes - k->k_IOParam.m_Count;
}

/* 返回NULL表示目录搜索失败，否则是根指针，指向文件的内存打开i节点 ，上锁的内存i节点  */
Inode* FileManager::NameI(char(*func)(), enum DirectorySearchMode mode)
{
	Inode* pInode;
	Buf* pBuf;
	char curchar;
	char* pChar;
	int freeEntryOffset;	         /* 以创建文件模式搜索目录时，记录空闲目录项的偏移量 */
	Kernel* k = Kernel::getInstance();
	BufferManager* bufMgr = k->getBufMgr();

	/*
	 * 如果该路径是'/'开头的，从根目录开始搜索，
	 * 否则从进程当前工作目录开始搜索。
	 */
	pInode = k->cdir;
	if ('/' == (curchar = (*func)()))
	{
		pInode = this->rootDirInode;
	}
		
	/* 检查该Inode是否正在被使用，以及保证在整个目录搜索过程中该Inode不被释放 */
	k->getInodeTable()->IGet(pInode->i_number);

	/* 允许出现////a//b 这种路径 这种路径等价于/a/b */
	while ('/' == curchar)
	{
		curchar = (*func)();
	}

	/* 如果试图更改和删除当前目录文件则出错 */
	if ('\0' == curchar && mode != FileManager::OPEN)
		goto out;

	/* 外层循环每次处理pathname中一段路径分量 */
	while (true)
	{
		/* 如果出错则释放当前搜索到的目录文件Inode，并退出 */
		if (k->error != Kernel::NO_ERROR)
		{
			break;     /* goto out; */
		}
			
		/* 整个路径搜索完毕，返回相应Inode指针。目录搜索成功返回。 */
		if ('\0' == curchar)
		{
			return pInode;
		}
			
		/* 将Pathname中当前准备进行匹配的路径分量拷贝到Kernel的dbuf[]中 便于和目录项进行比较 */
		pChar = &(k->dbuf[0]);
		while ('/' != curchar && '\0' != curchar && k->error == Kernel::NO_ERROR)
		{
			if (pChar < &(k->dbuf[DirectoryEntry::DIRSIZ]))
			{
				*pChar = curchar;
				pChar++;
			}
			curchar = (*func)();
		}
		/* 将dbuf剩余的部分填充为'\0' */
		while (pChar < &(k->dbuf[DirectoryEntry::DIRSIZ]))
		{
			*pChar = '\0';
			pChar++;
		}

		/* 允许出现////a//b 这种路径 这种路径等价于/a/b */
		while ('/' == curchar)
		{
			curchar = (*func)();
		}
			
		if (k->error != Kernel::NO_ERROR)
		{
			return NULL;           /* */
		}

		/* 内层循环部分对于dbuf[]中的路径名分量，逐个搜寻匹配的目录项 */
		k->k_IOParam.m_Offset = 0;
		/* 设置为目录项个数 ，含空白的目录项*/
		k->k_IOParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);   /* 4是外存INode大小 */
		freeEntryOffset = 0;
		pBuf = NULL;

		while (true)
		{
			/* 对目录项已经搜索完毕 */
			if (k->k_IOParam.m_Count == 0)
			{
				if (pBuf != NULL)
				{
					bufMgr->Brelse(pBuf);
				}
				/* 如果是创建新文件 */
				if (FileManager::CREATE == mode && curchar == '\0')
				{
					

					/* 将父目录Inode指针保存起来，以后写目录项“WriteDir()函数”会用到 */
					k->pdir = pInode;

					if (freeEntryOffset)	/* 此变量存放了空闲目录项位于目录文件中的偏移量 */
					{
						/* 将空闲目录项偏移量存入u区中，写目录项WriteDir()会用到 */
						k->k_IOParam.m_Offset = freeEntryOffset - (DirectoryEntry::DIRSIZ + 4);
					}
					else   /*问题：为何if分支没有置IUPD标志？  这是因为文件的长度没有变呀*/
					{
						pInode->i_flag |= Inode::IUPD;
					}
					/* 找到可以写入的空闲目录项位置，NameI()函数返回 */
					return NULL;
				}

				/* 目录项搜索完毕而没有找到匹配项，释放相关Inode资源,并退出 */
				k->error = Kernel::NOENT;
				goto out;
			}

			/* 已读完目录文件的当前盘块，需要读入下一目录项数据盘块 */
			if (k->k_IOParam.m_Offset % Inode::BLOCK_SIZE == 0)
			{
				if (pBuf != NULL)
				{
					bufMgr->Brelse(pBuf);
				}
				/* 计算要读的物理盘块号 */
				int phyBlkno = pInode->Bmap(k->k_IOParam.m_Offset / Inode::BLOCK_SIZE);
				pBuf = bufMgr->Bread(phyBlkno);
			}

			/* 没有读完当前目录项盘块，则读取下一目录项至dent */
			int* src = (int*)(pBuf->b_addr + (k->k_IOParam.m_Offset % Inode::BLOCK_SIZE));
			std::copy(src, src+sizeof(DirectoryEntry) / sizeof(int),(int*)&k->dent);

			k->k_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
			k->k_IOParam.m_Count--;

			/* 如果是空闲目录项，记录该项位于目录文件中偏移量 */
			if (k->dent.inode == 0)
			{
				if (freeEntryOffset == 0)
				{
					freeEntryOffset = k->k_IOParam.m_Offset;
				}
				/* 跳过空闲目录项，继续比较下一目录项 */
				continue;
			}

			int i;
			for (i = 0; i < DirectoryEntry::DIRSIZ; i++)
			{
				if (k->dbuf[i] != k->dent.name[i])
				{
					break;	/* 匹配至某一字符不符，跳出for循环 */
				}
					
			}

			if (i < DirectoryEntry::DIRSIZ)
			{
				/* 不是要搜索的目录项，继续匹配下一目录项 */
				continue;
			}
			else
			{
				/* 目录项匹配成功，回到外层While(true)循环 */
				break;
			}
		}

		/*
		 * 从内层目录项匹配循环跳至此处，说明pathname中
		 * 当前路径分量匹配成功了，还需匹配pathname中下一路径
		 * 分量，直至遇到'\0'结束。
		 */
		if (pBuf != NULL)
			bufMgr->Brelse(pBuf);

		/* 如果是删除操作，则返回父目录Inode，而要删除文件的Inode号在dent.inode中 */
		if (FileManager::DELETE == mode && '\0' == curchar)
			return pInode;

		/*
		 * 匹配目录项成功，则释放当前目录Inode，根据匹配成功的
		 * 目录项m_ino字段获取相应下一级目录或文件的Inode。
		 */
		k->getInodeTable()->IPut(pInode);
		pInode = k->getInodeTable()->IGet(k->dent.inode);
		/* 回到外层While(true)循环，继续匹配Pathname中下一路径分量 */

		if (pInode == NULL)   /* 获取失败 */
			return NULL;
	}
out:
	k->getInodeTable()->IPut(pInode);
	return NULL;
}

char FileManager::NextChar()
{
	Kernel* k = Kernel::getInstance();
	/* k->dirp指向pathname中的字符 */
	return *k->dirp++;
}

Inode* FileManager::MakNode(unsigned int mode)
{
	Inode* pInode;
	Kernel* k = Kernel::getInstance();

	/* 分配一个空闲DiskInode，里面内容已全部清空 */
	pInode = k->getFileSys()->IAlloc();

	//cout<<k->pdir->i_number<<endl;
	//cout<<pInode->i_number<<endl;

	if (pInode == NULL)
	{
		return NULL;
	}

	pInode->i_flag |= (Inode::IACC | Inode::IUPD);
	pInode->i_mode = mode | Inode::IALLOC;
	pInode->i_nlink = 1;

	/* 将目录项写入u.u_dent，随后写入目录文件 (this->WriteDir(pInode)) */
	/* 设置目录项中Inode编号部分 */
	k->dent.inode = pInode->i_number;
    

	/* 设置目录项中pathname分量部分 */
	for (int i = 0; i < DirectoryEntry::DIRSIZ; i++)
	{
		k->dent.name[i] = k->dbuf[i];
	}
		
	k->k_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	k->k_IOParam.m_Base = (char*)&k->dent; 
	     

	/* 将目录项写入父目录文件 */
	k->pdir->WriteI();
	k->getInodeTable()->IPut(k->pdir);
    
	//如果是文件夹，需要初始化目录项
if(k->isDir){

	/* 写当前目录*/
    k->dent.name[0]='.';
	/* 将dbuf剩余的部分填充为'\0' */
	for(int i=1;i<DirectoryEntry::DIRSIZ;i++)
	{
		k->dent.name[i]= '\0';
	}	
	k->k_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	k->k_IOParam.m_Base = (char*)&k->dent; 
	k->k_IOParam.m_Offset=0;
	/* 将目录项写入目录文件 */
	pInode->WriteI();

/*写父目录*/
	k->dent.name[1]='.';	
	k->dent.inode=k->pdir->i_number;
	k->k_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	k->k_IOParam.m_Base = (char*)&k->dent; 
	pInode->WriteI();

	//k->getInodeTable()->IPut(pInode);

	}

	return pInode;
}

void FileManager::ChDir()
{
	Inode* pInode;
	Kernel* k = Kernel::getInstance();

	pInode = this->NameI(FileManager::NextChar, FileManager::OPEN);
	if (pInode == NULL)
		return;
	/* 搜索到的文件不是目录文件 */
	if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
	{
		k->error = Kernel::NOTDIR;
		k->getInodeTable()->IPut(pInode);
		return;
	}
	k->getInodeTable()->IPut(k->cdir);
	k->cdir = pInode;

	/* 设置当前工作目录字符串curdir this->SetCurDir */
	/* 路径不是从根目录'/'开始，则在现有u.u_curdir后面加上当前路径分量 */
    
	int j=0;
	int cur=strlen(k->curdir)-1;
	
	while(1){
		if(k->pathname[j]=='.'){
			if(k->pathname[j+1]=='.'){
				char ch;
				while((ch=k->curdir[cur-1])!='/'){
                       cur--;
				}
				cur--;
				j+=3;
				
			}
			else
				j+=2;//跳过/
		}else{
			break;
		}
	}
	//cout<<k->pathname<<' '<<k->curdir<<endl;
	if (k->pathname[0] != '/')
	{
		
		//strcpy(k->curdir+cur,k->pathname+j);//dst<-src
		//防止根目录分隔符重复
		if(k->curdir[cur]=='/'){
		   cur--;
		}
		else{
			k->curdir[cur+1]='/';
		}
		for(int i=0;i<=strlen(k->pathname)-j;i++){
			k->curdir[cur+i+2]=k->pathname[j+i];
		}
		
	}
	else    /* 如果是从根目录'/'开始，则取代原有工作目录 */
	{
		strcpy(k->curdir,k->pathname);
	}
	if(k->curdir[cur+2+strlen(k->pathname)-j]!='\0'){
			k->curdir[cur+2+strlen(k->pathname)-j]='\0';
	}
}

void FileManager::Delete()
{
	Inode* pInode;  //当前目录的INode指针
	Inode* pDeleteInode;  //当前文件的INode指针
	Kernel* k = Kernel::getInstance();

	pDeleteInode = this->NameI(FileManager::NextChar, FileManager::DELETE);
	//没找到需要删除的文件
	if (pDeleteInode == NULL)
		return;
	pInode = k->getInodeTable()->IGet(k->dent.inode);

	if(pInode==NULL){
		k->error=Kernel::E_BUSY;
		return ;
	}
	//本身就是root，不需要再考虑root对目录文件的权限问题
    
	k->k_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
	k->k_IOParam.m_Base = (char*)&k->dent;
	k->k_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	k->dent.inode = 0;
	pDeleteInode->WriteI();        //删除的文件写回磁盘
	pInode->i_nlink--;
	pInode->i_flag |= Inode::IUPD; //nlink--
	k->getInodeTable()->IPut(pDeleteInode);
	k->getInodeTable()->IPut(pInode);
}

void FileManager::Rename(string ori, string cur)
{
	Inode* pInode;  //当前目录的INode指针
	Kernel* k = Kernel::getInstance();
	Buf* pBuf = NULL;
	BufferManager* bufMgr = k->getBufMgr();
	pInode = k->cdir;
	k->k_IOParam.m_Offset = 0;
	k->k_IOParam.m_Count = pInode->i_size / sizeof(DirectoryEntry);
	while (k->k_IOParam.m_Count) {
		if (0 == k->k_IOParam.m_Offset % Inode::BLOCK_SIZE) {  //要换盘快了
			if (pBuf) //缓存有信息
				bufMgr->Brelse(pBuf); //释放
			int phyBlkno = pInode->Bmap(k->k_IOParam.m_Offset / Inode::BLOCK_SIZE); //新的盘块号
			pBuf = bufMgr->Bread(phyBlkno); //缓存读新的盘块
		}

		DirectoryEntry tmp; 
		memcpy(&tmp, pBuf->b_addr + (k->k_IOParam.m_Offset % Inode::BLOCK_SIZE), sizeof(k->dent));

		if (strcmp(tmp.name, ori.c_str()) == 0) {
			strcpy(tmp.name, cur.c_str());
			memcpy(pBuf->b_addr + (k->k_IOParam.m_Offset % Inode::BLOCK_SIZE), &tmp, sizeof(k->dent));
		}
		k->k_IOParam.m_Offset += sizeof(DirectoryEntry);
		k->k_IOParam.m_Count--;
	}
	char* ans = pBuf->b_addr + ((k->k_IOParam.m_Offset -32 ) % Inode::BLOCK_SIZE)+4 ;
	if (pBuf)
	{
		bufMgr->Bwrite(pBuf);
		//bufMgr->Brelse(pBuf);
	}
		
}


/*
 * 返回值是0，表示拥有打开文件的权限；1表示没有所需的访问权限。文件未能打开的原因记录在u.u_error变量中。
 */
int FileManager::Access(Inode *pInode, unsigned int mode)
{
	if ((pInode->i_mode & mode) != 0)
	{
		return 0;
	}
	return 1;
}