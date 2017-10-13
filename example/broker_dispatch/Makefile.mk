CCFLAGS = -g -I../../../muduo -I../../ -I../../../rapidjson -I../../../cppzk -DMUDUO_STD_STRING
LDFLAGS = -pthread -lcommserv -lmuduo_net  -lmuduo_base -lboost_system -lboost_thread -lcppzk -lzookeeper_mt \
-L../../commserv -L../../../build/debug/lib -L../../../cppzk

all: master worker


%.o:%.cc
	g++ -c -o $@  $^ ${CCFLAGS}
master: master.o
	g++ -o $@ $^ ${LDFLAGS}	${CCFLAGS}
worker: worker.o
	g++ -o $@ $^ ${LDFLAGS}	${CCFLAGS}

clean:
	rm -f *.o