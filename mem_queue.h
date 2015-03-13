#ifndef MEM_QUEUE_H_
#define MEM_QUEUE_H_

#include <stdint.h>
#include <semaphore.h>

/* @brief 内存块类型
 */
enum MEM_BLOCK_TYPE {
	BLK_DATA	= 0,
	BLK_ALIGN	= 1, 
	BLK_CLOSE	= 2,
	BLK_OPEN	= 3,
};

/* @brief 共享内存类型
 */
enum MEM_TYPE {
	MEM_TYPE_RECV = 0,  //接收队列
	MEM_TYPE_SEND = 1	//发送队列
};

/* @brief 共享内存首部,用于记录队列信息
 */
typedef struct mem_head {
	volatile int head;		//头部	
	volatile int tail;		//尾部
	volatile int blk_cnt;	//总共块数  正确
} __attribute__((packed)) mem_head_t;

/* @brief 内存队列
 */
typedef struct mem_queue {
	mem_head_t *ptr;
	int len;				//总长
	int8_t type;			//类型
	sem_t *sem;				//信号量
} __attribute__((packed)) mem_queue_t;

/* @brief 内存块信息
 */
typedef struct mem_block {
	//int id;					//不需要，由于只有两个共享队列，不用管到底放在那个
	int fd;					//fd
	int len;				//长度
	uint8_t type;			//类型
	char data[];			//数据
} __attribute__((packed)) mem_block_t;


extern const int blk_head_len;
extern const int mem_head_len;

/* @brief b 放在q的尾部
 *
 * @param q 共享内存队列
 * @param b 队列块
 * @param data 队列中数据
 * @param pipefd 通知的fd
 */
int mq_push(mem_queue_t *q, const mem_block_t *b, const void *data, int pipefd);

/* @brief q从尾部出来
 * @return mem_block_t* 返回弹出的块
 */
mem_block_t* mq_pop(mem_queue_t *q);

/* @biref 创建共享队列
 *
 * @param q 队列指针
 * @param size 队列大小
 * @param type 队列类型
 * @param semname 信号名字
 */
int mq_init(mem_queue_t *q, int size, int type, const char* semname);

/* @brief 释放共享队列
 *
 * @param size 队列大小
 * @param semname 信号名字
 */
int mq_fini(mem_queue_t *q, int size, const char* semname);

/* @brief 返回尾块
 */
inline mem_block_t* blk_tail(mem_queue_t *q);

/* @brief 返回头部块
 */
inline mem_block_t* blk_head(mem_queue_t *q);

/* @brief 打印队列调试信息
 */
void mq_display(mem_queue_t *q);

#endif
