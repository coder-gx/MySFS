
#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include "FileSystem.h"
#include "OpenfileManager.h"

/*
 * 文件管理类(FileManager)
 * 封装了文件系统的各种系统调用在核心态下处理过程，
 * 如对文件的Open()、Close()、Read()、Write()等等
 * 封装了对文件系统访问的具体细节。
 */
class FileManager
{
public:
	/* 目录搜索模式，用于NameI()函数 */
	enum DirectorySearchMode
	{
		OPEN = 0,		/* 以打开文件方式搜索目录 */
		CREATE = 1,		/* 以新建文件方式搜索目录 */
		DELETE = 2		/* 以删除文件方式搜索目录 */
	};
public:
	FileManager() {};
	~FileManager() {};
	void Open();			 /* 打开一个文件 */
	void Creat();             /* 新建一个文件 */
	void Open1(Inode* pInode, int mode, int trf); /* Open和Create的公共部分 */
	void Close();             /* 关闭一个文件 */
	void Seek();          /* 改变当前读写指针的位置 */
	void Read();          /* 读文件 */
	void Write();         /* 写文件 */
	void Rdwr(enum File::FileFlags mode);  /* Read和Write方法的公共部分 */
	Inode* NameI(char(*func)(), enum DirectorySearchMode mode); /* 路径搜索 将路径转化为相应的Inode*/
	static char NextChar();  /* 获取路径中的下一个字符 */
	Inode* MakNode(unsigned int mode); /* 被Creat()调用，新建一个文件时，分配资源 */
	void ChDir();             /* 改变当前工作目录 */
	void Delete();        /* 删除文件 */    
	void Rename(string ori, string cur);        /* 重命名文件 */  
	int Access(Inode *pInode, unsigned int mode); //判断权限
public:
	Inode* rootDirInode; /* 根目录内存Inode */
	
};

class DirectoryEntry
{
public:
	static const int DIRSIZ = 28;	/* 目录项中路径部分的最大字符串长度 */
public:
	DirectoryEntry() {};
	~DirectoryEntry() {};
public:
	int inode;		        /* 目录项中Inode编号部分 */
	char name[DIRSIZ];	    /* 目录项中路径名部分 */
};
#endif