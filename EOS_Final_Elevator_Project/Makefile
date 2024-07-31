CC = gcc-9
#CC = /usr/bin/aarch64-linux-gnu-gcc

SRC = $(wildcard src/)
OBJ = obj
INCLUDE = include
OBJS = $(OBJ)/bitmap.o $(OBJ)/elevator_tool.o $(OBJ)/device_output.o $(OBJ)/kernel_tool.o $(OBJ)/sockop.o $(OBJ)/ascii.o

all: elevator_system txt_input server client

elevator_system: ${SRC}elevator_system.c $(OBJS)
	$(CC) -I$(INCLUDE) -o elevator_system $^ -lpthread  -lrt

txt_input: ${SRC}txt_input.c
	$(CC) -I$(INCLUDE) -o txt_input ${SRC}txt_input.c -lrt -lpthread

server: ${SRC}server.c ${OBJS}
	$(CC) -I$(INCLUDE) -o server $^ -lpthread  -lrt

client: ${SRC}client.c ${OBJS}
	$(CC) -I$(INCLUDE) -o client $^ -lpthread  -lrt

$(OBJ)/bitmap.o: ${SRC}bitmap.c | $(OBJ)
	$(CC) -I$(INCLUDE) -c $^ -o $@

$(OBJ)/elevator_tool.o: ${SRC}elevator_tool.c | $(OBJ)
	$(CC) -I$(INCLUDE) -c $^ -o $@

$(OBJ)/device_output.o: ${SRC}device_output.c | $(OBJ)
	$(CC) -I$(INCLUDE) -c $^ -o $@

$(OBJ)/kernel_tool.o: ${SRC}kernel_tool.c | $(OBJ)
	$(CC) -I$(INCLUDE) -c $^ -o $@

$(OBJ)/sockop.o: ${SRC}sockop.c | $(OBJ)
	$(CC) -I$(INCLUDE) -c $^ -o $@

$(OBJ)/ascii.o: ${SRC}ascii.c | $(OBJ)
	$(CC) -I$(INCLUDE) -c $^ -o $@

$(OBJ):
	mkdir -p $(OBJ)

clean: 
	rm -rf $(OBJ)/*.o elevator_system txt_input server client
