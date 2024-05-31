CPPFLAGS += -std=c++17 -Wall
LDFLAGS += -lcurl -lasound -ljsoncpp -lvosk -lpv_porcupine -lyaml-cpp
LDFLAG += -pthread
BIN_DIR=./bin
OBJS = main.o assistant.o

silence: src/main.cpp src/assistant.cpp src/assistant.h
	mkdir -p $(BIN_DIR)
	g++ $(LDFLAG) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR)


#g++ -pthread -o silence main.o assistant.o -lasound -lcurl -ljsoncpp -lvosk -lpv_porcupine