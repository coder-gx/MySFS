#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H
#include "Buf.h"
#include <fstream>
#include <iostream>
//#include <thread>
using namespace std;

/* 省略了进程图像在内存和盘交换区之间传递的成员变量 */
class BufferManager
{
public:
	/* static const member */
	static const int NBUF = 15;			/* 缓存控制块、缓冲区的数量 */
	static const int BUFFER_SIZE = 512;	/* 缓冲区大小。 以字节为单位 */
public:
	BufferManager();
	~BufferManager();
	//void Initialize();					/* 缓存控制块队列的初始化。将缓存控制块中b_addr指向相应缓冲区首地址。*/
	Buf* GetBlk(int blkno);	            /* 申请一块缓存，用于读写字符块blkno。*/
	void Brelse(Buf* bp);				/* 释放缓存控制块buf */
	Buf* Bread(int blkno);	            /* 读一个磁盘块，blkno为目标磁盘块逻辑块号。 */
	//Buf* Breada(int blkno, int rablkno);	/* 读一个磁盘块，带有预读方式。*/
	void Bwrite(Buf* bp);				/* 写一个磁盘块 */
	void Bdwrite(Buf* bp);				/* 延迟写磁盘块 */
	void Bawrite(Buf* bp);				/* 异步写磁盘块 */
	void ClrBuf(Buf* bp);				/* 清空缓冲区内容 */
	void Bflush();				        /* 将dev指定设备队列中延迟写的缓存全部输出到磁盘 */
private:
	void NotAvail(Buf* bp);				/* 从自由队列中摘下指定的缓存控制块buf */
private:
	Buf bFreeList;						/* 自由缓存队列控制块，定义有自由队列队头*/
	Buf m_Buf[NBUF];					/* 缓存控制块数组 */
	//Buf bDevtab;                        /* 设备控制块，定义有设备队列队头 */   //新增成员变量
	char Buffer[NBUF][BUFFER_SIZE];	    /* 缓冲区数组 */ 
};



#endif