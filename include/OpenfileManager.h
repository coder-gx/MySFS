
#ifndef OPEN_FILE_MANAGER_H
#define OPEN_FILE_MANAGER_H
#include "INode.h"
#include "FileSystem.h"
#include "File.h"

/*
 * 内存Inode表(class InodeTable)
 * 负责内存Inode的分配和释放。
 */
class InodeTable
{
public:
	static const int NINODE = 100;	/* 内存Inode的数量 */
public:
	InodeTable();
	~InodeTable();
	Inode* IGet(int inumber); /* 根据外存Inode编号获取对应Inode */
	void IPut(Inode* pNode);  /* 减少该内存Inode的引用计数，如果此Inode已经没有目录项指向它，且无进程引用该Inode，则释放此文件占用的磁盘块 */
	void UpdateInodeTable();  /* 将所有被修改过的内存Inode更新到对应外存Inode中 */
	int IsLoaded(int inumber); /* 检查编号为inumber的外存inode是否有内存拷贝。如果有则返回该内存Inode在内存Inode表中的索引 */
	Inode* GetFreeInode();     /* 在内存Inode表中寻找一个空闲的内存Inode */
public:
	Inode m_Inode[NINODE];		/* 内存Inode数组，每个打开文件都会占用一个内存Inode */
};

/* 一个类型为 File 的数组，数组每一项都是一个 File 文件控制块 */
class OpenFileTable
{
public:
	static const int NFILE = 100;	/* 打开文件控制块File结构的数量 */
public:
	OpenFileTable() {};
	~OpenFileTable() {};
	File* FAlloc();            /* 在系统打开文件表中分配一个空闲的File结构 */
	void CloseF(File* pFile); /* 对打开文件控制块File结构的引用计数f_count减1. 若引用计数f_count为0，则释放File结构*/
public:
	File m_File[NFILE];			/* 系统打开文件表 */
};

#endif