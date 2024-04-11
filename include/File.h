#ifndef FILE_H
#define FILE_H
#include "INode.h"
/*
 * 打开文件控制块File类。
 * 该结构记录了进程打开文件
 * 的读、写请求类型，文件读写位置等等。
 */
/* 记录文件以同一或者不同的路径名打开，用不同的操作要求打开*/
/* 在源码File.h中 */
class File
{
public:
	enum FileFlags
	{
		FREAD = 0x1,			/* 读请求类型 */
		FWRITE = 0x2,			/* 写请求类型 */
		FPIPE = 0x4				/* 管道类型 */
	};
public:
	File();
	~File();
	unsigned int f_flag;		/* 对打开文件的读、写操作要求 */
	int		f_count;			/* 当前引用该文件控制块的进程数量 */
	Inode* f_inode;			    /* 指向打开文件的内存Inode指针 */
	int		f_offset;			/* 文件读写位置指针 */
};


/* 管理每个进程的打开文件，其本质上也是一个 File 类的数组 */
class OpenFiles
{
public:
	static const int NOFILES = 15;	/* 允许打开的最大文件数 */
public:
	OpenFiles();
	~OpenFiles() {};
	int AllocFreeSlot();            /* 在内核打开文件描述符表中分配一个空闲表项 */
	File* GetF(int fd);             /* 根据文件描述符参数fd找到对应的打开文件控制块File结构 */
	void SetF(int fd, File* pFile); /* 为已分配到的空闲描述符fd和已分配的打开文件表中空闲File对象建立勾连关系*/
public:
	File* k_OpenFileTable[NOFILES];		/* File对象的指针数组，指向系统打开文件表中的File对象 */
};


/*
 * 文件I/O的参数类
 * 对文件读、写时需用到的读、写偏移量、
 * 字节数以及目标区域首地址参数。
 */
struct IOParameter
{
	char* m_Base;	/* 当前读、写用户目标区域的首地址 */  
	int m_Offset;	/* 当前读、写文件的字节偏移量 */
	int m_Count;	/* 当前还剩余的读、写字节数量 */
};


#endif

