#ifndef _MSG_QUEUE_H_
#define _MSG_QUEUE_H_

#include "concurrentqueue.h"


#define MSG_MAX_LEN 65535

/*
消息包队列(先进先出)
支持多线程
lixiaoming 2020-02-04
*/
class MsgQueue
{

public:

	struct Item
	{
        int		type;
		uint16_t	len;
		void*		data = nullptr;
		size_t      offset;
		~Item()
		{
			if (data)
				free(data);
		}
	};

public:

	~MsgQueue()
	{
		exit = true;
	}

    void Exit()
    {
        exit=true;
    }

	//写入1条消息
    bool Push(int type, uint16_t len, const void* data, size_t dataoffset)
	{

		auto item = new Item;
		if (!item)
		{
			printf("alloc failed\n");
			return false;
		}
		item->offset = dataoffset/(1024*1024);
		item->type = type;
		item->len = len;
		item->data = malloc(len);
		if (!item->data) 
			goto FAILED;

		memcpy(item->data, data, len);
		while (!mItemQueue.enqueue(item) && (!exit));
		return true;

	FAILED:
		if (item) 
			delete item;

		return false;

	}

	//读取一定数量的消息
	//返回值:消息数量
	size_t Pop(Item** ppItem, size_t nItemMaxNum)
	{
		auto nItemNum = mItemQueue.try_dequeue_bulk(ppItem, nItemMaxNum);
		return nItemNum;
	}

	//释放消息内存
	void Free(Item** ppItem, size_t nItemNum)
	{
		for (size_t i = 0; i < nItemNum; ++i)
			delete ppItem[i];
	}

protected:
	bool exit=false;
	struct MyTraits : public moodycamel::ConcurrentQueueDefaultTraits
	{
		static const size_t BLOCK_SIZE = 256;//每次分配一块内存，内存可以存放BLOCK_SIZE个元素
		static const size_t MAX_SUBQUEUE_SIZE = 512*1024;//总共存多少元素,实际数量为(MAX_SUBQUEUE_SIZE+BLOCK_SIZE-1)/BLOCK_SIZE*BLOCK_SIZE;即整数个BLOCK_SIZE
	};
	moodycamel::ConcurrentQueue<Item*, MyTraits> mItemQueue;
};

#endif
