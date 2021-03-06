#ifndef MASTER_H_
#define MASTER_H_

struct mem_block;

/* @brief master init
 * @note 
 */
int master_init();

/* @brief  负责主进程读取网络数据 或者将发送管理数据发送给具体的子进程
 */
int master_listen();

/* @brief 主进程通知子进程的管道
 * @note 接收消息按照一定规则来通知
 */
int master_recv_pipe_create(int i); 

/* @brief 
 */
int master_dispatch();

/* @brief 
 */
int master_fini();


/*  @brief 处理cli
 */
int handle_cli(int fd);


/* @brief 处理管道的读取
 * @note 直接就可以
 */
int handle_pipe(int fd);

/* @brief 处理往客户端发送消息
 */
void handle_mq_send();

/* @brief 发送消息块
 */
int do_blk_send(struct mem_block *blk);



/* @brief 执行fd打开
 */
int do_fd_open(int fd);

/* @brief 初始化配置信息
 */
int init_setting();

/* @brief  将消息转化为blk
 */
void raw2blk(int fd, struct mem_block *blk);

/* @brief 处理信号
 * @return 0-success -1-error
 */
int handle_signal();

/* @brief 对应描述符被挂断 实现子进程重启
 * @note 由于父进程与子进程共享资源，所以不能在收到信号的时候处理
 *  信号收到时只处理僵尸进程
 */
void handle_hup(int fd);
#endif
