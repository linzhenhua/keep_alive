##############################
# Makefile
##############################

#target
TARGET=main           #目标可执行文件

#compile and lib parameter
CXX=gcc       					#编译器
CXXFLAGS=-g -Wall -std=gnu99   #编译器参数
LIBS=-lm              #动态链接库名称
#LDFLAGS=-L/usr/lib/mysql/       #动态库路径
#INCLUDE=-I/usr/include/mysql/   #头文件路径

#SRC=$(wildcard *.c)            #wildcard一般和*搭配使用，wildcard表示*通配符展开时，仍然有效
#OBJ=$(SRC:%.c=%.o)             #%.cc=%.o 表示所有.o文件替换为.cc文件，这样就可以得到在当前目录可生成的.o文件列表

#link
#$(TARGET):$(OBJ)
	#$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

#compile
#%.o: %.cc
	#$(CXX) $(CXXFLAGS) $(INCLUDE) -c $^

#link
$(TARGET):ytk_daemon_log.o cJSON.o keep_alive.o main.o
	$(CXX) -o $@ $^ $(LIBS)

#compile
ytk_daemon_log.o:ytk_daemon_log.c ytk_daemon_log.h
	$(CXX) $(CXXFLAGS) -c $<
cJSON.o:cJSON.c cJSON.h
	$(CXX) $(CXXFLAGS) -c $<
keep_alive.o:keep_alive.c keep_alive.h
	$(CXX) $(CXXFLAGS) -c $<
main.o:main.c keep_alive.h
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o
	rm -f *~
	rm -f $(TARGET) 
