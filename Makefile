CXX=clang++
# CXX=g++
CXXFLAGS=-std=c++11
CPPFLAGS=-I.
LDFLAGS=-lIce -lIceUtil -pthread -lvlc
CLASSPATH=.:/usr/share/java/Ice.jar
JAVAC=javac
JAVA=java


all: slicedefcpp metaServer musicServer

client: slicedefjava Client.class

# Client
Client.class: Client.java
	$(JAVAC) -classpath $(CLASSPATH) $^ Player/*.java


# Servers
metaServer: metaServer.o server.o
	$(CXX) $^ -o $@ $(LDFLAGS)

metaServer.o: metaServer.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@ $(CPPFLAGS)

musicServer: musicServer.o server.o
	$(CXX) $^ -o $@ $(LDFLAGS)

musicServer.o: musicServer.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@ $(CPPFLAGS)

server.o: server.cpp server.h
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(CPPFLAGS)


server.cpp: slicedefcpp

server.h: slicedefcpp


.PHONY: clean mrproper slicedefs slicedefcpp slicedefjava runsrv

# Slice definitions
slicedefs: slicedefcpp slicedefjava

slicedefcpp: server.ice
	slice2cpp server.ice

slicedefjava: server.ice
	slice2java server.ice

runsrv:
	./server

runjavaclient:
	$(JAVA) -classpath $(CLASSPATH) Client


clean:
	rm -f *.o
	rm -f Client.class
	rm -rf Player

mrproper: clean
	rm -f musicServer metaServer
	rm -f server.cpp server.h
