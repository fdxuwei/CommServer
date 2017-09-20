CCFLAGS = -g -I../../muduo -I../../rapidjson -DMUDUO_STD_STRING

OBJS = CommServer.o MsgHandler.o PacketBuffer.o ProtoObjs.o SafeRand.o ServerPool.o TcpClients.o
	

OUT = libcommserv.a

all: ${OUT}

CommProto.pb.cc: CommProto.proto
	protoc  --proto_path= --cpp_out=./ $^

%.o:%.cc
	g++ -c -o $@  $^ ${CCFLAGS}
${OUT}: $(OBJS)
	ar rv $@ $?

clean:
	rm -f *.o