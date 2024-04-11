
#include "../include/Buf.h"
#include "../include/BufferManager.h"
#include "../include/Kernel.h"

const char* Kernel::DISK_IMG;  //定义为了static，需要先声明一下

/* 代替BufferManager::Initialize() */

//因为只有一个进程，没有io请求队列，也没有多个设备，
//可以认为任何时刻最多有一个busy的缓存，因此可以只保留自由队列

BufferManager::BufferManager()
{
	int i;
	Buf* bp;

	this->bFreeList.av_forw = this->bFreeList.av_back = &(this->bFreeList); /* 初始化自由队列队头 */
	//this->bDevtab.b_forw = this->bDevtab.b_back = &(this->bDevtab);       /* 初始化设备队列队头 */

	for (i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);
		bp->b_addr = this->Buffer[i];

		
		/* 初始化设备队列 */  //NODEV队列改为设备队列
		//bp->b_back = &(this->bDevtab);
		//bp->b_forw = this->bDevtab.b_forw;
		//this->bDevtab.b_forw->b_back = bp;
		//this->bDevtab.b_forw = bp;

		/* 初始化自由队列 */
		bp->b_flags = Buf::B_BUSY;
		Brelse(bp);
	}
	return;
}

BufferManager::~BufferManager()
{
	Bflush();
}

Buf* BufferManager::GetBlk(int blkno)
{
	Buf* bp;
	//Buf* dp = &this->bDevtab;
	Buf *dp=&this->bFreeList;
loop:
	/*直接在队列中寻找*/
	for (bp = dp->av_forw; bp != dp; bp = bp->av_forw)
	{
		/* 不是要找的缓存，则继续 */
		if (bp->b_blkno != blkno)
			continue;

		/* 从自由队列中抽取出来 */
		this->NotAvail(bp);
		return bp;
	}

	// /* 设备队列没找到，从自由队列取 */
	// /* 如果自由队列为空 */
	// if (this->bFreeList.av_forw == &this->bFreeList)
	// {
	// 	this->bFreeList.b_flags |= Buf::B_WANTED;
	// 	goto loop;
	// }
        

	    /* 没找到就取自由队列第一个空闲块 */

	   bp = this->bFreeList.av_forw;
	   this->NotAvail(bp);

	/* 如果该字符块是延迟写，将其异步写到磁盘上 */
	if (bp->b_flags & Buf::B_DELWRI)
	{
		bp->b_flags |= Buf::B_ASYNC;
		this->Bwrite(bp);
		//goto loop;  这里不再等待，直接采用写回之后的该磁盘
	}

	/* 注意: 这里清除了所有其他位，只设了B_BUSY */
	bp->b_flags = Buf::B_BUSY;
	bp->b_blkno = blkno;
	return bp;
}

void BufferManager::Brelse(Buf* bp)
{
	/* 注意以下操作并没有清除B_DELWRI、B_WRITE、B_READ、B_DONE标志
	 * B_DELWRI表示虽然将该控制块释放到自由队列里面，但是有可能还没有些到磁盘上。
	 * B_DONE则是指该缓存的内容正确地反映了存储在或应存储在磁盘上的信息
	 */
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);
	/* 插到自由队列队尾 */
	(this->bFreeList.av_back)->av_forw = bp;
	bp->av_back = this->bFreeList.av_back;
	bp->av_forw = &(this->bFreeList);
	this->bFreeList.av_back = bp;
	return;
}

Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	bp = this->GetBlk(blkno);
	/* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
	if (bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* 没有找到相应缓存，没有io队列，直接读 */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;
	fstream f(Kernel::DISK_IMG, ios::in | ios::binary);
	f.seekg(blkno * BufferManager::BUFFER_SIZE);
	f.read(bp->b_addr, bp->b_wcount);
	f.close();
	return bp;
}

// void writing(writeArg* arg)
// {
// 	std::fstream f(Kernel::DISK_IMG, ios::in | ios::out | ios::binary);
// 	f.seekp(arg->bp->b_blkno * BufferManager::BUFFER_SIZE);
// 	f.write((char*)arg->bp->b_addr, arg->bp->b_wcount);
// 	f.close();
// 	arg->b->Brelse(arg->bp);
// }

void BufferManager::Bwrite(Buf* bp)
{
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE;


	std::fstream f(Kernel::DISK_IMG, ios::in | ios::out | ios::binary);
	f.seekp(bp->b_blkno * BufferManager::BUFFER_SIZE);
	f.write((char*)bp->b_addr, bp->b_wcount);
	f.close();
	this->Brelse(bp);
	
	
	return;
}

void BufferManager::Bdwrite(Buf* bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}

void BufferManager::Bawrite(Buf* bp)
{
	/* 标记为异步写 */
	bp->b_flags |= Buf::B_ASYNC;
	this->Bwrite(bp);
	return;
}

void BufferManager::ClrBuf(Buf* bp)
{
	int* pInt = (int*)bp->b_addr;

	/* 将缓冲区中数据清零 */
	for (int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}

void BufferManager::Bflush()
{
	Buf* bp;
	/* 注意：这里之所以要在搜索到一个块之后重新开始搜索，
	 * 因为在bwite()进入到驱动程序中时有开中断的操作，所以
	 * 等到bwrite执行完成后，CPU已处于开中断状态，所以很
	 * 有可能在这期间产生磁盘中断，使得bfreelist队列出现变化，
	 * 如果这里继续往下搜索，而不是重新开始搜索那么很可能在
	 * 操作bfreelist队列的时候出现错误。
	 */
loop:
	for (bp = this->bFreeList.av_forw; bp != &(this->bFreeList); bp = bp->av_forw)
	{
		/* 找出自由队列中所有延迟写的块 */
		if (bp->b_flags & Buf::B_DELWRI)
		{
			this->NotAvail(bp);
			this->Bwrite(bp);
			goto loop;
		}
	}
	return;
}

void BufferManager::NotAvail(Buf* bp)
{

	/* 从自由队列中取出 */
	bp->av_back->av_forw = bp->av_forw;
	bp->av_forw->av_back = bp->av_back;
	/* 设置B_BUSY标志 */
	bp->b_flags |= Buf::B_BUSY;
	return;
}
