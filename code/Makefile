CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = COOLServer
OBJS = log/*.cpp pool/*.cpp timer/*.cpp \
       http/*.cpp server/*.cpp \
       buffer/*.cpp config/*.cpp main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)




