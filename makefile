all: server client
server: main.cc service.cc
	g++ main.cc service.cc -o $@ -std=c++20 -ljsoncpp -lpthread -lmysqlpp -ggdb
client: client.cc
	g++ $^ -o $@ -std=c++20 -ljsoncpp -lpthread -lmysqlpp 
.PHONY:cleanserver cleanclient clean
clean:cleanserver cleanclient
cleanserver:
	rm server  -rf
cleanclient:
	rm client -rf