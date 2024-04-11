#ifndef KERNEL_H
#define KERNEL_H
#include "BufferManager.h"
#include "FileManager.h"
#include "FileSystem.h"
#include "OpenfileManager.h"  /* */
#include <iostream>
#include<vector>
using namespace std;



/* 二级文件系统的核心类(内核)，只初始化一个实例*/
class Kernel
{
public:
	/* 参考User.h中的u_error's Error Code */
	enum ERROR {
		NO_ERROR = 0,            /* 没有出错 */
		ISDIR = 1,               /* 操纵非数据文件 */
		NOTDIR = 2,              /* cd命令操纵数据文件 */
		NOENT = 3,               /* 文件不存在 */
		BADF = 4,                /* 文件标识fd错误 */
		NOOUTENT = 5,            /* 外部文件不存在 */
		NOSPACE = 6,              /* 磁盘空间不足 */
		NOACCESS  =7 ,          /*无权限访问该文件*/
        E_BUSY = 8                /*资源忙，无法释放*/
	};
	Kernel();
	~Kernel();
	static Kernel* getInstance();  /* 获取唯一的内核类实例 */

	BufferManager* getBufMgr();        /* 获取内核的高速缓存管理实例 */
	FileSystem* getFileSys();          /* 获取内核的文件系统实例 */
	FileManager* getFileMgr();         /* 获取内核的文件管理实例 */

	InodeTable* getInodeTable();       /* 获取内核的内存Inode表 */
	OpenFiles* getOpenFiles();         /* 获取内核的打开文件描述符表 */
	OpenFileTable* getOpenFileTable(); /* 获取系统全局的打开文件描述符表 */
	SuperBlock* getSuperBlock();       /* 获取全局的SuperBlock内存副本*/
public:
	/* 系统调用相关成员 */
	char* dirp;			   	      /* 指向路径名的指针,用于nameI函数 */

	/* 文件系统相关成员 */
	Inode* cdir;		          /* 指向当前目录的Inode指针 */
	Inode* pdir;                  /* 指向当前目录父目录的Inode指针 */
	DirectoryEntry dent;		  /* 当前目录的目录项 */
	char dbuf[DirectoryEntry::DIRSIZ];	/* 当前路径分量 */
	char curdir[128];            /* 当前完整工作目录 */
	ERROR error;                  /* 存放错误码 */

	/* 文件I/O操作 */
	IOParameter k_IOParam;        /* 记录当前读、写文件的偏移量，用户目标区域和剩余字节数参数 */

	/* 当前系统调用参数 */
	char* buf;                    /* 指向读写的缓冲区 */   //u_arg[0]
	int fd;                       /* 记录文件标识 */     //u_arg[0]
	char* pathname;               /* 目标路径名 */     //u_arg[0]
	int nbytes;                   /* 记录读写的字节数 */   //u_arg[1]
	int offset;                   /* 记录Seek的读写指针位移 */   //u_arg[1]
	int mode;                     /* 记录操纵文件的方式或seek的模式 */   //u_arg[2]
	int callReturn;               /* 记录调用函数的返回值 */   //u_ar0[User::EAX]

	static const char* DISK_IMG;
	
	bool isDir;                   /* 当前操作是否针对目录文件 */
private:
	static Kernel instance;      /* 唯一的内核类实例 */

	BufferManager* BufMgr;       /* 内核的高速缓存管理实例 */
	FileSystem* fileSys;         /* 内核的文件系统实例 */
	FileManager* fileMgr;        /* 内核的文件管理实例 */
	
	
	InodeTable* k_InodeTable;    /* 内核的内存Inode表 */
	OpenFileTable* s_openFiles;  /* 系统全局打开文件描述符表 */
	OpenFiles* k_openFiles;      /* 内核的打开文件描述符表 */
	SuperBlock* spb;              /* 全局的SuperBlock内存副本 */

public:
	void initialize();                                  /* 内核初始化 */
	void callInit();                                    /* 每个函数调用的初始化工作 */
	void format();                                      /* 格式化磁盘 */
	int open(char* pathname, int mode);                 /* 打开文件 */
	int create(char* pathname, int mode);               /* 新建文件 */
	void mkdir(char* pathname);                         /* 新建目录 */
	void cd(char* pathname);                            /* 改变当前工作目录 */
	void ls();                                          /* 显示当前目录下的所有文件 */
	int fread(int readFd, char* buf, int nbytes);       /* 读一个文件到目标区 */
	int fwrite(int writeFd, char* buf, int nbytes);     /* 根据目标区的字符写一个文件 */
	void fseek(int seekFd, int offset, int ptrname);    /* 改变读写指针的位置 */
	void fdelete(char* pathname);                       /* 删除文件 */
	void cp(char* from, char* to,int mode);                  /* 将文件拷贝到磁盘某目录下 */
	void frename(char* ori, char* cur);                 /* 将文件重命名 */
	void dfs_tree(string path, int depth);
	void ftree(string path);                             /* 显示目录树 */
	int close(int fd);                                  /* 关闭文件 */
	void clear();                                       /* 系统关闭时收尾工作 */
};
#endif