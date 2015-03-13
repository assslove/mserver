#ifndef OUT_H_
#define OUT_H_

//#include "fds.h"

/* @brief 提供对业务逻辑的接口
*/
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
	typedef struct ServInterface {
		void*   data_handle;  //数据段打开so
		void*   handle;		  //代码段打开so 

		/* @brief 处理定时器
		 * @note 现每隔2秒循环一次
		 */
		void	(*handle_timer)(); 

		/* @brief 处理客户端消息
		 */
		int		(*proc_cli_msg)(void* msg, int len, int fd);

		/* @brief 处理服务端消息
		 */
		void	(*proc_serv_msg)(int fd, void* msg, int len);

		/* @brief 客户端fd断掉之后的处理
		 */
		void	(*on_cli_closed)(int fd);

		/* @brief 服务端fd断掉的处理
		 */
		void	(*on_serv_closed)(int fd);

		/* @brief 服务器初始化
		 */
		int 	(*serv_init)(int ismaster);

		/* @brief 服务器结束之后处理
		 */
		int 	(*serv_fini)(int ismaster);

		/* @brief 获取消息长度
		 */
		int		(*get_msg_len)(int fd, const void *data, int len, int ismaster);
	} serv_if_t;

	extern serv_if_t so;

	int  reg_data_so(const char* name);
	int  reg_so(const char* name, int flag);
	void unreg_data_so();
	void unreg_so();

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OUTER_H_
