
all : test_serv.so data.so 
	mv test_serv.so data.so ../
test_serv.so : test_serv.cpp
	g++ -o test_serv.so test_serv.cpp -g -Wall -fPIC -shared `pkg-config --cflags --libs glib-2.0`
data.so : data.cpp
	g++ -g -Wall -fPIC -shared -o data.so data.cpp
