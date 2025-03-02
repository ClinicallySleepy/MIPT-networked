CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17

SRC_CLIENT := client.cpp
SRC_SERVER := server.cpp
SRC_SOCKET := socket_tools.cpp

OBJ_CLIENT := client.o
OBJ_SERVER := server.o
OBJ_SOCKET := socket_tools.o

BIN_CLIENT := client
BIN_SERVER := server

all: $(BIN_CLIENT) $(BIN_SERVER)

$(OBJ_SOCKET): $(SRC_SOCKET) socket_tools.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_CLIENT): $(OBJ_CLIENT) $(OBJ_SOCKET)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_SERVER): $(OBJ_SERVER) $(OBJ_SOCKET)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_CLIENT) $(OBJ_SERVER) $(OBJ_SOCKET) $(BIN_CLIENT) $(BIN_SERVER)

.PHONY: all clean
