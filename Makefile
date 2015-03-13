# 生成多进程处理的网络服务器，可用于执行数据库操作等功能
#

INSTALL_DIR = /usr/local/include/meserv
CFLAGS =  -g -Wall -Wunused-function -rdynamic -lpthread -ldl -L/usr/local/lib -lnanc -DENABLE_TRACE
#CFLAGS =  -g -Wall -Wunused-function -rdynamic -lpthread -ldl -L/usr/local/lib -lnanc
LIBS = `pkg-config --cflags --libs glib-2.0`
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))

all : meserv

meserv : $(OBJS)
	gcc $(OBJS) -o meserv $(LIBS) $(CFLAGS) 

%.o : %.c
	gcc -c -o $@ $< $(LIBS) $(CFLAGS) 

clean:
	-rm -f $(OBJS) meserv

install:
	sudo rm -rf $(INSTALL_DIR)
	sudo mkdir $(INSTALL_DIR)
	sudo cp -r *.h $(INSTALL_DIR) 
