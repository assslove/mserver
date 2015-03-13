/*
 * =====================================================================================
 *
 *       Filename:  mem_queue.c
 *
 *    Description:  共享队列设计 与simple_server不同，主要在于本共享队列涉及到多进程的处理
 *    本共享队列利用POSIX信号量来解决同步问题 push时候必须获得锁，pop时间也得获得锁，每次pop需要传出信息,
 *    push时候也得把信息赋值进去。
 *
 *        Version:  1.0
 *        Created:  12/07/2014 09:14:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	xiaohou, houbin-12@163.com
 *   Organization:  XiaoHou, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

#include <libnanc/log.h>
#include <libnanc/posix_lock.h>

#include "mem_queue.h"
#include "util.h"
#include "net_util.h"

const int blk_head_len = sizeof(mem_block_t);
const int mem_head_len = sizeof(mem_head_t);
extern struct svr_setting setting;

int mq_init(mem_queue_t *q, int size, int type, const char* semname)
{
	q->len = size;	
	q->type = type;

	q->ptr = (mem_head_t *)mmap((void *)-1, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);	
	if (q->ptr == MAP_FAILED) {
		return -1;
	}

	q->ptr->head = mem_head_len;
	q->ptr->tail = mem_head_len;

	if ((q->sem = safe_semopen(semname, 1)) == SEM_FAILED) {
		return -1;
	}

#ifdef ENABLE_TRACE
	mq_display(q);
#endif
	return 0;
}

int mq_fini(mem_queue_t *q, int size, const char* semname)
{
	munmap(q->ptr, size);	
	//关闭信号量
	safe_semclose(q->sem, semname);
	return 0;
}

mem_block_t *mq_pop(mem_queue_t *q)
{
	static mem_block_t *blk = NULL;
	static mem_block_t *tmp_blk = NULL;
	mem_head_t *ptr = q->ptr;
	if (unlikely(!blk)) {
		blk = (mem_block_t *)malloc(setting.max_msg_len);
		if (!blk) {
			ERROR(0, "pop blk is null");
			return NULL;
		}
	}

	safe_semwait(q->sem);
pop_again:
	if (ptr->head > ptr->tail) { 
		if (likely(ptr->head >= ptr->tail + blk_head_len)) {  //大于一个空块的大小
			goto pop_succ;
		} else { //不会出现
			TRACE(0, "mem pop: queue is null");		
			goto pop_fail;
		}
	} else if (ptr->head < ptr->tail) {
		if (q->len < ptr->tail + blk_head_len) { //如果容纳不下一个块, 已经到尾部
			ptr->tail = mem_head_len;
			goto pop_again;
		} else { //如果可以容纳一个块
			tmp_blk = blk_tail(q);
			if (tmp_blk->type == BLK_ALIGN) { //如果是填充块 则调整位置
				ptr->tail = mem_head_len;
				goto pop_again;
			} else { //成功弹出
				goto pop_succ;
			}
		}
	} else { //相等队列空
		goto pop_fail;
	}

pop_fail:
#ifdef ENABLE_TRACE
	//mq_display(q);
#endif
	safe_sempost(q->sem);
	return NULL;

pop_succ:
#ifdef ENABLE_TRACE
	mq_display(q);
#endif
	tmp_blk = blk_tail(q);
	memcpy(blk, tmp_blk, tmp_blk->len);
	ptr->tail += blk->len;
	--ptr->blk_cnt;
	safe_sempost(q->sem);
	return blk;
}

/* @brief 利用goto 主要节省代码行数， 也可以不用goto来处理
 * @note 空队列 head == tail 
 *       满队列 head + xx >= tail push速度赶上pop速度
 */
int mq_push(mem_queue_t *q, const mem_block_t *b, const void *data, int pipefd)
{
	static mem_block_t *blk = NULL;
	safe_semwait(q->sem);
push_again:
	if (q->ptr->head >= q->ptr->tail) { 
		if (q->ptr->head + b->len >= q->len) { //如果大于最大长度
			if (q->ptr->head + blk_head_len <= q->len) { //如果容下一个块头//填充
				blk = blk_head(q);
				blk->type = BLK_ALIGN;
				blk->len = q->len - q->ptr->head; 
			}

			if (unlikely(mem_head_len == q->ptr->tail)) { //如果尾部还没有弹出 说明是满的状态
				goto push_fail;
			} else {
				q->ptr->head = mem_head_len;		//调整到头部
				goto push_again;
			}
		} else {//完全可以容纳
			goto push_succ;
		}
	} else if (q->ptr->head < q->ptr->tail) { 
		if (unlikely(q->ptr->head + b->len >= q->ptr->tail)) {//full
			goto push_fail;
		} else {
			goto push_succ;
		}
	} 

push_fail:
#ifdef ENABLE_TRACE
	//mq_display(q);
#endif
	safe_sempost(q->sem);
	return -1;

push_succ:
	blk = blk_head(q);
	memcpy(blk, b, blk_head_len);
	memcpy(blk->data, data, b->len - blk_head_len);
	q->ptr->head += b->len;
	++q->ptr->blk_cnt;

#ifdef ENABLE_TRACE
	mq_display(q);
#endif
	safe_sempost(q->sem); //释放锁
	write(pipefd, q, 1);
	return 0;
}

mem_block_t* blk_head(mem_queue_t *q)
{
	return (mem_block_t *)((char *)q->ptr + q->ptr->head);
}

mem_block_t* blk_tail(mem_queue_t *q)
{
	return (mem_block_t *)((char *)q->ptr + q->ptr->tail);
}

void mq_display(mem_queue_t *q)
{
	TRACE(q->type, "blk_len=%d, head_len=%u, blk_cnt=%d, head=%d, tail=%d, len=%u", \
			blk_head_len, mem_head_len, q->ptr->blk_cnt, q->ptr->head, q->ptr->tail, q->len);
	if (q->ptr->blk_cnt <= 0) {
		TRACE(q->type, "blk_cnt=%d", q->ptr->blk_cnt);
	}
}
