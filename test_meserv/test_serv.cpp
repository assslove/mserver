/*
 * =====================================================================================
 *
 *       Filename:  test_serv.cpp
 *
 *    Description:  对mserver 的测试 echo 服务器
 *
 *        Version:  1.0
 *        Created:  12/23/2014 05:18:02 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	houbin , houbin-12@163.com
 *   Organization:  Houbin, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */


#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <libnanc/log.h>
#include <libnanc/conf.h>
#include <libnanc/proto_head.h>
#include <meserv/net_util.h>
#include <meserv/fds.h>
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus 
#define OUTER_FUNC extern "C"
#else
#define OUTER_FUNC 
#endif 


OUTER_FUNC void handle_timer()
{
}

OUTER_FUNC int proc_cli_msg(void *msg, int len, int fd)
{
	proto_pkg_t *pkg = reinterpret_cast<proto_pkg_t *>(msg);

	DEBUG(pkg->id, "online fd=%u, len=%u,id=%u,seq=%u,cmd=%u,ret=%u, msg=%s", fd, pkg->len, pkg->id, pkg->seq, pkg->cmd, pkg->ret, (char *)pkg->data);

	uint32_t  cli[1024];
	memcpy(cli, msg, pkg->len);

	return send_to_cli(fd, cli, pkg->len);
}

OUTER_FUNC int proc_serv_msg(int fd, void *msg, int len)
{
	return 0;
}

OUTER_FUNC int on_cli_closed(int fd) 
{
	INFO(0, "cli fd=%u closed", fd);
	return 0;
}

OUTER_FUNC int on_serv_closed(int fd)
{
	return 0;
}

OUTER_FUNC int serv_init(int ismaster) 
{
	INFO(0, "%s init", ismaster ? "master" : "work");

	return 0;
}

OUTER_FUNC int serv_fini(int ismaster) 
{
	INFO(0, "%s fini", ismaster ? "master" : "work");
	return 0;
}

OUTER_FUNC int	get_msg_len(int fd, const void *data, int len, int ismaster)
{
	return *(int *)((uint8_t*)data);
}


