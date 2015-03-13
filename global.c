/*
 * =====================================================================================
 *
 *       Filename:  global.c
 *
 *    Description:  全局变量定义区 
 *
 *        Version:  1.0
 *        Created:  12/07/2014 09:33:33 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	xiaohou, houbin-12@163.com
 *   Organization:  XiaoHou, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */


#include <libnanc/list.h>

#include "global.h"
#include "fds.h"
#include "net_util.h"
#include "master.h"
#include "outer.h"


/* @brief 用于修改进进程名字用
 */
int g_argc;
char **g_argv;
char *argv_start;
char *argv_end;
char *env_end;

int chl_pids[MAX_WORKS] = {0};

//work进程管理
work_mgr_t  workmgr;
//epoll相关信息
epoll_info_t epinfo;
//配置信息
svr_setting_t setting;
//fd manager 没有用处
GHashTable *fds;
//stop flag
int stop;
