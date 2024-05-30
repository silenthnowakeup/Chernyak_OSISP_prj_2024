CPPFLAGS += -std=c++11 -Wall
LDFLAGS += -pthread -lcurl -lasound -ljsoncpp -lvosk -lpv_porcupine

OBJS = main.o assistant.o

silence: $(OBJS)
	g++ $(LDFLAGS) -o silence $(OBJS)

clean:
	rm -rf *.o silence

#g++ -pthread -o silence main.o assistant.o -lasound -lcurl -ljsoncpp -lvosk -lpv_porcupine