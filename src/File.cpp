#include"../include/File.h"
#include"../include/Kernel.h"


File::File()
{
	this->f_count = 0;
	this->f_flag = 0;
	this->f_offset = 0;
	this->f_inode = NULL;
}

File::~File()
{

}


OpenFiles::OpenFiles()
{
	for (int i = 0; i < OpenFiles::NOFILES; i++)
		SetF(i, NULL);
}

int OpenFiles::AllocFreeSlot()
{
	int i;
	for (i = 0; i < OpenFiles::NOFILES; i++)
	{
		/* 进程打开文件描述符表中找到空闲项，则返回之 */
		if (this->k_OpenFileTable[i] == NULL)
		{
			/* 系统调用返回值 */
			Kernel::getInstance()->callReturn = i;
			return i;
		}
	}
	Kernel::getInstance()->callReturn = -1;   /* Open1，需要一个标志。当打开文件结构创建失败时，可以回收系统资源*/
	return -1;
}

File* OpenFiles::GetF(int fd)
{
	File* pFile;
	Kernel* k = Kernel::getInstance();

	/* 如果打开文件描述符的值超出了范围 */
	if (fd < 0 || fd >= OpenFiles::NOFILES)
	{
		k->error = Kernel::BADF;
		return NULL;
	}

	if ((pFile = this->k_OpenFileTable[fd]) == NULL)
	{
		k->error = Kernel::BADF;
		return NULL;
	}
	return pFile;   /* 即使pFile==NULL也返回它，由调用GetF的函数来判断返回值 */
}



void OpenFiles::SetF(int fd, File* pFile)
{
	if (fd < 0 || fd >= OpenFiles::NOFILES)
		return;
	/* 进程打开文件描述符指向系统打开文件表中相应的File结构 */
	this->k_OpenFileTable[fd] = pFile;
}
