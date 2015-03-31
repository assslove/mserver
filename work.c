/*
 * =====================================================================================
 *
 *       Filename:  work.c
 *
 *    Description:  子进程处理函数放在此处
 *
 *        Version:  1.0
 *        Created:  12/11/2014 02:00:12 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	houbin , houbin-12@163.com
 *   Organization:  Houbin, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */

#include <errno.h>
#include <sys/epoll.h>
#include <malloc.h>
#include <sys/types.h>
#include <libnanc/log.h>
#include <fcntl.h>

#include "net_util.h"
#include "util.h"
#include "global.h"
#include "work.h"
#include "outer.h"
#include "fds.h"
#include "mem_queue.h"

int work_init(int i, int isreboot)
{
	log_fini();
	char buf[10] = {'\0'};
	sprintf(buf, "%d", i + 1);

	int idx = 0;
	for (idx = 0; idx <= epinfo.maxfd; ++idx) { //关闭监听的fd
		if (epinfo.fds[idx].fd > 0) {
			close(epinfo.fds[idx].fd);
//			INFO(0, "%s close fd=%d", __func__, epinfo.fds[idx].fd);
		}
	}

	//log init
	if (log_init(setting.log_dir, setting.log_level, setting.log_size, setting.log_maxfiles, buf) == -1) {
		fprintf(stderr, "init log failed\n");
		return 0;
	}

	work_t *work = &workmgr.works[i];
	//chg title
	chg_proc_title("%s-WORK-%d", setting.srv_name, work->id);
	
	free(epinfo.evs);
	free(epinfo.fds);
	close(epinfo.epfd);
	
	//子进程epinfo主要用于接收父进程的管道通知
	setting.nr_max_event = 20;
	setting.nr_max_fd = 20;
	epinfo.epfd = epoll_create(setting.nr_max_event);
	if (epinfo.epfd == -1) {
		ERROR(0, "create epfd error: %s", strerror(errno));
		return -1;
	}

	epinfo.evs = (struct epoll_event *)calloc(setting.nr_max_event / 10, sizeof(struct epoll_event));		
	if (epinfo.evs == NULL) {
		ERROR(0, "create epoll events error: %s", strerror(errno));
		return -1;
	}

	epinfo.fds = (fd_wrap_t *)calloc(setting.nr_max_fd, sizeof(fd_wrap_t));		
	if (epinfo.fds == NULL) {
		ERROR(0, "create epoll fds error: %s", strerror(errno));
		return -1;
	}

	if (add_fdinfo_to_epinfo(workmgr.works[i].recv_pipefd[0], i, fd_type_pipe, 0, 0) == -1) {  //用于接收主进程的读取
		return -1;
	} 

	int k = 0;
	for (; k < workmgr.nr_work; k++) {
		if (k == i) {
			close(workmgr.works[k].recv_pipefd[1]); //关闭接收队列的写端
			close(workmgr.works[k].send_pipefd[0]); //关闭发送队列的读端
		} else { //其它都关闭
			if (!isreboot) {
				close(workmgr.works[k].recv_pipefd[0]);
				close(workmgr.works[k].send_pipefd[1]);
			}
			close(workmgr.works[k].recv_pipefd[1]);
			//close(workmgr.works[k].send_pipefd[0]);
		}
	}

	stop = 0;
	//清除chl_pids;
	memset(chl_pids, 0, sizeof(chl_pids));

	//初始化子进程
	if (so.serv_init && so.serv_init(0)) {
		ERROR(0, "child serv init failed");
		return -1;
	}
	//init list_head
	INIT_LIST_HEAD(&epinfo.readlist);				
	INIT_LIST_HEAD(&epinfo.closelist);				

	work_index = i;
	INFO(0, "child serv[id=%d] have started", workmgr.works[i].id);

	return 0;
}

int work_dispatch(int i)
{
	//handle closelist
	handle_closelist(0);
	//handle readlist
	handle_readlist(0);

	stop = 0;
	int k = 0;
	int fd = 0;
	while (!stop) {
		int nr = epoll_wait(epinfo.epfd, epinfo.evs, setting.nr_max_event, 2000);
		if (nr == -1 && errno != EINTR) {
			ERROR(0, "epoll wait [id=%d,err=%s]", i, strerror(errno));
			return 0;
		}
		for (k = 0; k < nr; ++k) {
			fd = epinfo.evs[k].data.fd;
			//判断异常状态
			//if (fd > epinfo.maxfd || epinfo.fds[fd].fd != fd) {
			//ERROR(0, "child wait failed fd=%d", fd);
			//continue;
			//}

			if (epinfo.evs[k].events & EPOLLIN) {
				switch (epinfo.fds[fd].type) {
					case fd_type_pipe:
						do_proc_pipe(fd);
						break;
					case fd_type_svr:
						if (do_proc_svr(fd) == -1) {
							do_fd_close(fd, 0);
						}
						break;
					case fd_type_mcast:
						do_proc_mcast(fd);
						break;
					default:
						break;
				}
			} else if (epinfo.evs[k].events & EPOLLOUT) {
				if (epinfo.fds[fd].buff.slen > 0) {
					if (do_fd_write(fd) == -1) {
						do_fd_close(fd, 0);
					}
				}

				if (epinfo.fds[fd].buff.slen == 0) { //发送完毕
					mod_fd_to_epinfo(epinfo.epfd, fd, EPOLLIN);
				}
			} else if (epinfo.evs[k].events & EPOLLHUP) {

			} else if (epinfo.evs[k].events & EPOLLRDHUP) {

			} else {
				INFO(0, "wait event [fd=%d,events=%d]", fd, epinfo.evs[k].events);
			}
		}

		//handle memqueue read
		if (nr) { //有事件触发读取收队列事件
			handle_mq_recv(i);	
		}
		//handle timer callback
		if (so.handle_timer) {
			so.handle_timer();
		}
	}

	return 0;	
}

int work_fini(int i)
{
	if (so.serv_fini && so.serv_fini(0)) {
		ERROR(0, "child serv fini failed");
	}

	free(epinfo.evs);
	free(epinfo.fds);
	close(epinfo.epfd);

	close(workmgr.works[i].send_pipefd[1]);
	close(workmgr.works[i].recv_pipefd[0]);
	
	log_fini();
	DEBUG(0, "work serv [id=%d] have stopped!", workmgr.works[i].id);

	return 0;	
}

int handle_mq_recv(int i)
{
	static mem_queue_t *recvq = &epinfo.msgq.rq;
	//static mem_queue_t *sendq = &epinfo.msgq.sq;
	mem_block_t *tmpblk;

	while ((tmpblk = mq_pop(recvq)) != NULL) {
		switch (tmpblk->type) {
			case BLK_DATA: //对客户端消息处理
				do_blk_msg(tmpblk);
				break;
			//case BLK_OPEN:
				//if (do_blk_open(tmpblk) == -1) { //处理块打开，如果打不到，则关闭
					//tmpblk->type = BLK_CLOSE;	
					//tmpblk->len = sizeof(mem_block_t);
					//mq_push(sendq, tmpblk, NULL);
				//}
				//break;
			case BLK_CLOSE:
				do_blk_close(tmpblk); //处理关闭
				break;
		}
	}
	return 0;
}

int do_blk_msg(mem_block_t *blk)
{
	if (blk->len <= blk_head_len) {
		ERROR(0, "err blk len[total_len=%u,blk_len=%u]", blk->len, blk_head_len);
		return 0;
	}

	//fdsess_t *fdsess = get_fd(blk->fd);
	//if (fdsess) {
	//if (so.proc_cli_msg(blk->data, blk->len - blk_head_len, fdsess)) { //处理客户端消息
	//close_cli(blk->fd); //断开连接
	//}
	//}
	return so.proc_cli_msg(blk->data, blk->len - blk_head_len, blk->fd);
}

int do_blk_open(mem_block_t *blk)
{
	fdsess_t *fdsess = get_fd(blk->fd);
	if (fdsess || (blk->len != blk_head_len + sizeof(fd_addr_t))) {
		ERROR(0, "fd open error[fd=%u,len=%u,%u]", blk->fd, blk->len, fdsess);
		return -1;
	} else {
		fdsess_t *sess = g_slice_alloc(sizeof(fdsess_t));
		sess->fd = blk->fd;
		//sess->id = blk->id;
		fd_addr_t *addr = (fd_addr_t *)blk->data;
		sess->ip = addr->ip;
		sess->port = addr->port;
		save_fd(sess);

		TRACE(0, "work add fd [fd=%d]", blk->fd);
	}
	return 0;
}

int do_blk_close(mem_block_t *blk)
{
	fdsess_t *sess = get_fd(blk->fd);
	if (!sess) {
		ERROR(0, "[fd=%u] have closed", blk->fd);
		return -1;
	}
	//处理接口关闭回调
	so.on_cli_closed(blk->fd);

	remove_fd(blk->fd);

	TRACE(0, "work remove fd [fd=%d]", blk->fd);

	return 0;
}

int do_proc_mcast(int fd)
{
	//static char buf[MCAST_MSG_LEN];
	return 0;
}

int do_proc_svr(int fd) 
{
	fd_buff_t *buff = &epinfo.fds[fd].buff;

	if (handle_read(fd) == -1) {
		return -1;
	}

	char *tmp_ptr = buff->rbf;
proc_again:
	if (buff->msglen == 0 && buff->rlen > 0) { //获取长度
		buff->msglen = so.get_msg_len(fd, tmp_ptr, buff->rlen, SERV_WORK);
		TRACE(0, "recv [fd=%u][rlen=%u][msglen=%u]", fd, buff->rlen, buff->msglen);
	}

	//proc
	if (buff->rlen >= buff->msglen) {
		so.proc_serv_msg(fd, tmp_ptr, buff->msglen);
		//清空
		tmp_ptr += buff->msglen;
		buff->rlen -= buff->msglen;
		buff->msglen = 0;

		if (buff->rlen > 0) {//如果没有处理完继续处理
			goto proc_again;
		}
	}

	if (buff->rbf != tmp_ptr && buff->rlen > 0) {  //还有剩余的在缓存区，不够一个消息，等读完了再去push
		memmove(buff->rbf, tmp_ptr, buff->rlen); //合并到缓冲区头
	}

	return 0;
}

int do_proc_pipe(int fd) 
{
	static char pipe_buf[PIPE_MSG_LEN];
	while (read(fd, pipe_buf, PIPE_MSG_LEN) == PIPE_MSG_LEN) {}
	return 0;
}

void close_cli(int fd)
{
	//fdsess_t *sess = get_fd(fd);
	//if (!sess) {
	//return ;
	//}

	//mem_block_t blk;
	//blk.id = sess->id;
	//blk.len = blk_head_len;
	//blk.type = BLK_CLOSE;
	//blk.fd = fd;
	//mq_push(&workmgr.works[blk.id].sq, &blk, NULL);
	//do_blk_close(&blk); //不要重复执行
}
