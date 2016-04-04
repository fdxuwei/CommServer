CCFLAGS = -g -I../../../muduo -I../../ -I../../../rapidjson -DMUDUO_STD_STRING
LDFLAGS = -pthread -lcommserv -lmuduo_net  -lmuduo_base -lboost_system -lboost_thread -L../../commserv -L../../../build/debug/lib

OBJS = server.o 

all: server

%.o:%.cc
	g++ -c -o $@  $^ ${CCFLAGS}
server: $(OBJS)
	g++ -o $@ $^ ${LDFLAGS}	${CCFLAGS}