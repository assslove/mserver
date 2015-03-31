#ifndef WORK_H_
#define WORK_H_


/* @brief
 */
int work_init(int i, int isreboot);

/* @brief 
 */
int work_dispatch(int i);

/* @brief
 */
int work_fini(int i);

/* @brief 读取共享队列读入
 */
int handle_mq_recv();

/* @brief 处理块读取
 */
int do_blk_msg(mem_block_t *blk);

/* @brief 处理块关闭
 */
int do_blk_close(mem_block_t *blk);

/* @brief 处理块打开
 */
int do_blk_open(mem_block_t *blk);

/* @brief 处理服务器端的数据处理
 */
int do_proc_svr(int fd);

/* @brief 处理管道
 */
int do_proc_pipe(int fd);

/* @brief 处理组播
 */
int do_proc_mcast(int fd);

/* @brief 关闭fd
 */
void close_cli(int fd);

#endif
