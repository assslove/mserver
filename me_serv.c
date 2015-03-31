/*
 * =====================================================================================
 *
 *       Filename:  me_serv.c
 *
 *    Description:  me_serv
 *					muti process one parent multi child
 *					provide some interface for man
 *
 *        Version:  1.0
 *        Created:  04/06/2014 11:38:21 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	houbin , houbin-12@163.com
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h> /* basic socket definition */
#include <netinet/in.h> /* sockaddr_in and other internet def */
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>   
#include <string.h>   
#include <sys/epoll.h>
#include <sys/mman.h>
#include <arpa/inet.h>

#include <libnanc/log.h>
#include <libnanc/conf.h>

#include "net_util.h"
#include "mem_queue.h"
#include "global.h"
#include "util.h"
#include "master.h"
#include "work.h"

int main(int argc, char* argv[]) 
{	
	int ret;
	//load conf
	if ((ret = load_conf("etc/me.conf")) == -1) {
		fprintf(stderr, "load me conf faild\n");
		return 0;
	}
	//初始化配置信息
	if (init_setting() == -1) {
		BOOT(0, "init meserv conf failed \n");
		return 0;
	}

	//log init
	if (log_init(setting.log_dir, setting.log_level, setting.log_size, setting.log_maxfiles, "0") == -1) {
		fprintf(stderr, "init log failed");
		return 0;
	}

	//chg limit
	init_rlimit();
	//save args
	save_args(argc, argv);
	//chg serv name
	chg_proc_title(setting.srv_name);
	//daemon mode
	daemon(1, 1);
		
	//master_init
	ret = master_init();
	if (ret == -1) {
		ERROR(0, "err master init [%s]", strerror(errno));
		return 0;
	}

	//handle signal
	if (handle_signal()) {
		return 0;
	}

	int i = 0;
	for (; i < setting.worknum; i++) { //创建子进程用于处理父进程的逻辑
		int pid = fork();
		if (pid < 0) {
			ERROR(0, "create work fail[%d][%s]", i, strerror(errno));
			goto fail;	
		} else if (pid == 0) { //child
			int ret = work_init(i, 0); //初始化子进程
			if (ret == -1) {
				ERROR(0, "err work init [%s]", strerror(errno));
				exit(0);
			}
			work_dispatch(i);
			work_fini(i);
			exit(0);
		} else { //parent
			chl_pids[i] = pid;
		}
	}
	//只有一个监听 用于负责读取与发送数据
	ret = master_listen();
	if (ret == -1) {
		ERROR(0, "%s", strerror(errno));
		goto fail;
	}

	BOOT(0, "%s have started", setting.srv_name);
	//master loop
	master_dispatch();
fail:
	//master fini
	master_fini();

	return 0;
}

