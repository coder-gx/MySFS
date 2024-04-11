
#include "../include/Kernel.h"
#include <fstream>
#include <cstring>

Kernel Kernel::instance;

Kernel::Kernel()
{
	Kernel::DISK_IMG = "../myDisk.img";
	
	this->BufMgr = new BufferManager;
	this->fileSys = new FileSystem;
	this->fileMgr = new FileManager;
	this->k_InodeTable = new InodeTable;
	this->s_openFiles = new OpenFileTable;
	this->k_openFiles = new OpenFiles;
	this->spb = new SuperBlock;
}

Kernel::~Kernel()
{
}



void Kernel::clear()
{


	delete this->BufMgr;
	delete this->fileSys;
	delete this->fileMgr;
	delete this->k_InodeTable;
	delete this->s_openFiles;
	delete this->k_openFiles;
	delete this->spb;
}

void Kernel::initialize()
{
	
	this->fileSys->LoadSuperBlock();//载入superblock
	this->fileMgr->rootDirInode = this->k_InodeTable->IGet(FileSystem::ROOTINO); //获得根目录内存INode
	this->cdir = this->fileMgr->rootDirInode;      //给指向当前目录(根目录)的指针赋值
	strcpy(this->curdir,"/");


	//给root的目录写回来.和..
	/* 写当前目录*/
    this->dent.name[0]='.';
	/* 将dbuf剩余的部分填充为'\0' */
	for(int i=1;i<DirectoryEntry::DIRSIZ;i++)
    {
			this->dent.name[i]= '\0';
	}	
	
	this->dent.inode=FileSystem::ROOTINO;	
	this->k_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	this->k_IOParam.m_Base = (char*)&this->dent; 
	this->k_IOParam.m_Offset=0;
	/* 将目录项写入目录文件 */
	this->cdir->WriteI();

/*写父目录*/
	this->dent.name[1]='.';	
	this->k_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	this->k_IOParam.m_Base = (char*)&this->dent; 
	this->cdir->WriteI();

	//this->getInodeTable()->IPut(this->cdir);//新的inode进行覆盖旧的diskinode

	this->getBufMgr()->Bflush();//将延迟写写回，确保正确

}
   

//函数调用的init，初始化一些标志位，因为没有本项目删除了不必要的user，这里直接用kernel记录error
void Kernel::callInit()
{
	this->fileMgr->rootDirInode = this->k_InodeTable->IGet(FileSystem::ROOTINO); //获得根目录内存INode
	this->callReturn = -1;
	this->error = NO_ERROR;           //清标志位
}

/*以下借鉴unix v6++*/
Kernel* Kernel::getInstance()
{
	return &instance;
}

BufferManager* Kernel::getBufMgr()
{
	return this->BufMgr;
}

FileSystem* Kernel::getFileSys()
{
	return this->fileSys;
}

FileManager* Kernel::getFileMgr()
{
	return this->fileMgr;
}

InodeTable* Kernel::getInodeTable()
{
	return this->k_InodeTable;
}

OpenFiles* Kernel::getOpenFiles()
{
	return this->k_openFiles;
}

OpenFileTable* Kernel::getOpenFileTable()
{
	return this->s_openFiles;
}

SuperBlock* Kernel::getSuperBlock()
{
	return this->spb;
}

void Kernel::format()
{
	/* 格式化磁盘 */
	fstream f(Kernel::DISK_IMG, ios::out | ios::binary);
	for (int i = 0; i <= this->getFileSys()->DATA_ZONE_END_SECTOR; i++)
	{
		for (int j = 0; j < this->getBufMgr()->BUFFER_SIZE; j++)
		{
			f.write((char*)"\x0", 1);//写入0号字符，全清0
		}
			
	}
	//cout<<22<<endl;
	f.close();

	/* 格式化SuperBlock */
	Buf* pBuf;
	SuperBlock& spb = (*this->spb);
	spb.s_isize = FileSystem::INODE_ZONE_SIZE;
	spb.s_fsize = FileSystem::DATA_ZONE_SIZE;
	spb.s_ninode = 100;
	spb.s_nfree = 0;
	for (int i = 1; i <= 100; i++)  //栈式存储
	{
		spb.s_inode[100 - i] = i ;
	}

	for (int i = FileSystem::DATA_ZONE_END_SECTOR; i >= FileSystem::DATA_ZONE_START_SECTOR; i--) //清磁盘数据区
	{
		this->fileSys->Free(i);
	}
    //两个128的空间来copy到缓冲区,再从缓冲区写到磁盘
	for (int i = 0; i < 2; i++)
	{
		int* p = (int*)&spb + i * 128;
		pBuf = this->BufMgr->GetBlk(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);
		std::copy(p,p+128, (int*)pBuf->b_addr);
		this->BufMgr->Bwrite(pBuf);
	}

	/* 格式化Inode区 */
	for (int i = 0; i <FileSystem::INODE_ZONE_SIZE; i++)
	{
		pBuf = this->BufMgr->GetBlk(FileSystem::INODE_ZONE_START_SECTOR + i);
		DiskInode DiskInode[FileSystem::INODE_NUMBER_PER_SECTOR];
		for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
		{
			//单个DiskINode格式化
			DiskInode[j].d_mode = DiskInode[j].d_nlink = DiskInode[j].d_size = 0;
			for (int k = 0; k < 10; k++)
			{
				DiskInode[j].d_addr[k] = 0;
			}
		}
		/* 为根目录增加目录标志 */
		if (i == 0)
		{
			DiskInode[0].d_mode |= Inode::IFDIR;
			//DiskInode->d_addr[0]=FileSystem::DATA_ZONE_START_SECTOR;
		}
		std::copy((int*)&DiskInode,(int*)&DiskInode+128, (int*)pBuf->b_addr);
		//写回磁盘
		this->BufMgr->Bwrite(pBuf);
	}
	//与unix v6++不同，这里在构造函数里直接对各个模块进行初始化,在后面的clear中清除
	this->clear();//先清除原有的，这些都在kernel中建立过了
	this->BufMgr = new BufferManager;
	this->fileSys = new FileSystem;
	this->fileMgr = new FileManager;
	this->k_InodeTable = new InodeTable;
	this->s_openFiles = new OpenFileTable;
	this->k_openFiles = new OpenFiles;
	this->spb = new SuperBlock;
}

int Kernel::open(char* pathname, int mode)
{
	this->callInit();
	this->mode = mode;
	this->pathname = this->dirp = pathname;
	this->fileMgr->Open();
	return this->callReturn;
}

int Kernel::create(char* pathname, int mode)
{
	this->callInit();
	this->isDir = false;
	this->mode = mode;
	this->pathname = this->dirp = pathname;
	this->fileMgr->Creat();
	this->fileSys->Update();
	return this->callReturn;
}

void Kernel::mkdir(char* pathname)
{
	this->callInit();
	this->isDir = true;
	this->pathname = this->dirp = pathname;
	this->fileMgr->Creat();
	this->fileSys->Update();
	if (this->callReturn != -1)
		this->close(this->callReturn);
}

int Kernel::close(int fd)
{
	this->callInit();
	this->fd = fd;
	this->fileMgr->Close();
	return this->callReturn;
}

void Kernel::cd(char* pathname)
{
	this->callInit();
	this->pathname = this->dirp = pathname;
	this->fileMgr->ChDir();
}

int Kernel::fread(int readFd, char* buf, int nbytes)
{
	this->callInit();
	this->fd = readFd;
	this->buf = buf;
	this->nbytes = nbytes;
	this->fileMgr->Read();
	return this->callReturn;
}

int Kernel::fwrite(int writeFd, char* buf, int nbytes)
{
	this->callInit();
	this->fd = writeFd;
	this->buf = buf;
	this->nbytes = nbytes;
	this->getFileMgr()->Write();
	return this->callReturn;
}

void Kernel::ls()
{
	this->k_IOParam.m_Offset = 0;
	this->k_IOParam.m_Count = this->cdir->i_size / (DirectoryEntry::DIRSIZ + 4);
	//cout<<this->k_IOParam.m_Count <<endl;
	Buf* pBuf = NULL;
	while (true)
	{
		/* 对目录项已经搜索完毕 */
		if (this->k_IOParam.m_Count == 0)
		{
			if (pBuf != NULL)
			{
				this->getBufMgr()->Brelse(pBuf);
			}
			break;
		}

		/* 已读完目录文件的当前盘块，需要读入下一目录项数据盘块 */
		if (this->k_IOParam.m_Offset % Inode::BLOCK_SIZE == 0)
		{
			if (pBuf != NULL)
			{
				this->getBufMgr()->Brelse(pBuf);
			}
			int phyBlkno = this->cdir->Bmap(this->k_IOParam.m_Offset / Inode::BLOCK_SIZE);
			//cout<<"phy"<<phyBlkno<<endl;
			pBuf = this->getBufMgr()->Bread(phyBlkno);
		}

		/* 没有读完当前目录项盘块，则读取下一目录项至dent */
		int* src = (int*)(pBuf->b_addr + (this->k_IOParam.m_Offset % Inode::BLOCK_SIZE));
		std::copy(src,src+sizeof(DirectoryEntry) / sizeof(int), (int*)&this->dent );
		this->k_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
		this->k_IOParam.m_Count--;
		if (this->dent.inode == 0)
		{
			continue;
		}
		cout << this->dent.name << ' ';
	}
	cout << endl;
}

void Kernel::fseek(int seekFd, int offset, int ptrname)
{
	this->callInit();
	this->fd = seekFd;
	this->offset = offset;
	this->mode = ptrname;
	this->fileMgr->Seek();
}

void Kernel::fdelete(char* pathname)
{
	this->callInit();
	this->dirp = pathname;
	this->fileMgr->Delete();
}

void Kernel::cp(char* from, char* to,int mode)
{
	//mode==0 外部->磁盘 mode==1 磁盘->外部  mode==2 磁盘->磁盘

	if(mode==0){
	fstream f(from, ios::in | ios::binary);
	if (f)
	{
		f.seekg(0, f.end);  /* 第一个参数是偏移量，第二个参数是基地址 */
		int length = f.tellg();  /* 返回当前定位指针的位置，也代表着输入流的大小 */
		f.seekg(0, f.beg);
		char* tmpBuffer = new char[length];
		f.read(tmpBuffer, length);  //将内容读到中间变量中
		int tmpFd = this->create(to, Inode::IRWXU);//采用create模式，没有的文件直接创建
		if (this->error != NO_ERROR)
		{
			f.close();
		delete tmpBuffer;
		return;

		}
		this->fwrite(tmpFd, tmpBuffer, length);
		if (this->error != NO_ERROR)
		{
			f.close();
		delete tmpBuffer;
		return;
		}
		this->close(tmpFd);
	
		
	}
	else
	{
		this->error = NOOUTENT;
		return;
	}
	}
    else if(mode==1){
	fstream f(to, ios::out | ios::binary);
	if (f)
	{   
		//cout<<from<<endl;
		int tmpFd = this->open(from, File::FREAD);//只读形式打开
        if (this->error != NO_ERROR)
		{f.close();
		return;

		}
        File* pFile= this->getOpenFiles()->GetF(tmpFd);
		if (this->error != NO_ERROR)
			{
				f.close();
		return;
			}
		int length=pFile->f_inode->i_size;

        char* tmpBuffer = new char[length];
		this->fread(tmpFd, tmpBuffer, length);
		if (this->error != NO_ERROR)
			{
				f.close();
		delete tmpBuffer;
		return;
			}
		this->close(tmpFd);
		f.write(tmpBuffer, length);  
	}
	else
	{
		this->error = NOOUTENT;
		return;
	}
	}
	else if(mode==2){

        int fromFd = this->open(from, File::FREAD);
        if (this->error != NO_ERROR)
			{
		              return;
			}
        File* pFile= this->getOpenFiles()->GetF(fromFd);
		if (this->error != NO_ERROR)
			{
				return ;
			}
		int length=pFile->f_inode->i_size;
       
        char* tmpBuffer = new char[length];
		this->fread(fromFd, tmpBuffer, length);
		if (this->error != NO_ERROR)
			{
				 delete tmpBuffer;
				 return ;
			}
		this->close(fromFd);

        int toFd = this->create(to,File::FWRITE);
        if (this->error != NO_ERROR)
			{
				 delete tmpBuffer;
				 return ;
			}
		this->fwrite(toFd, tmpBuffer, length);
		if (this->error != NO_ERROR)
			{
				 delete tmpBuffer;
				 return ;
			}
		this->close(toFd);
 
	
	
       
	}

}

void Kernel::frename(char* Ori, char* Cur)
{
	this->callInit();
	char* curDir = curdir;
	string ori = Ori;
	string cur = Cur;
	if (ori.find('/') != string::npos) {
		string nextDir = ori.substr(0, ori.find_last_of('/'));
		if(nextDir=="")
		{
			nextDir = "/";
		}
		char nd[128];
		strcpy(nd, nextDir.c_str());
		cd(nd);
		ori = ori.substr(ori.find_last_of('/') + 1);
		if (cur.find('/') != string::npos)
			cur = cur.substr(cur.find_last_of('/') + 1);
	}
	this->fileMgr->Rename(ori,cur);
	cd(curDir);
}

void Kernel::dfs_tree(string path, int depth)
{
	vector<string> dirs; /* 目录项 */
	string nextDir;

	this->k_IOParam.m_Offset =0 ;
	this->k_IOParam.m_Count = this->cdir->i_size / (DirectoryEntry::DIRSIZ + 4);
	Buf* pBuf = NULL;
	while (true)
	{
		/* 对目录项已经搜索完毕 */
		if (this->k_IOParam.m_Count == 0)
		{
			if (pBuf != NULL)
			{
				this->getBufMgr()->Brelse(pBuf);
			}
			break;
		}

		/* 已读完目录文件的当前盘块，需要读入下一目录项数据盘块 */
		if (this->k_IOParam.m_Offset % Inode::BLOCK_SIZE == 0)
		{
			if (pBuf != NULL)
			{
				this->getBufMgr()->Brelse(pBuf);
			}
			int phyBlkno = this->cdir->Bmap(this->k_IOParam.m_Offset / Inode::BLOCK_SIZE);
			pBuf = this->getBufMgr()->Bread(phyBlkno);
		}

		/* 没有读完当前目录项盘块，则读取下一目录项至dent */
		int* src = (int*)(pBuf->b_addr + (this->k_IOParam.m_Offset % Inode::BLOCK_SIZE));
		std::copy(src,src+sizeof(DirectoryEntry) / sizeof(int), (int*)&this->dent);
		this->k_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
		this->k_IOParam.m_Count--;
		if (this->dent.inode == 0)
		{
			continue;
		}
		dirs.emplace_back(this->dent.name);
	}
	for (int i = 0; i < dirs.size(); i++) {
		if(dirs[i][0]=='.')//避开初始的两个目录项
		{
           continue;
		}
		nextDir = (path == "/" ? "" : path) + '/' + dirs[i];
		for (int j = 0; j < depth + 1; j++)
			cout << "|   ";
		cout << "|---" << dirs[i] << endl;
		char nd[128];
		strcpy(nd, nextDir.c_str());
		cd(nd);
		if(error != Kernel::NO_ERROR)  /* 访问的是数据文件，不是目录文件 */
		{
			error = NO_ERROR;
			continue;
		}
		dfs_tree(nextDir, depth + 1);
	}
	char nd[128];
	strcpy(nd, path.c_str());
	cd(nd);
	return;
}

void Kernel::ftree(string path)
{
	string curDirPath = curdir;
	
	char nd[128];
	strcpy(nd, path.c_str());
	cd(nd);
	
	cout << "|---" << (path == "/" ? "/" : path.substr(path.find_last_of('/') + 1)) << endl;
	dfs_tree(path, 0);
	strcpy(nd, curDirPath.c_str());
	cd(nd);
}
