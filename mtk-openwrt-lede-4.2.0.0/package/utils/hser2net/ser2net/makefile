XX = g++
CFLAGS += -O2 -Wno-implicit-function-declaration -Wno-int-conversion -I./ -I./include/ 

LDFLAGS += -luci
TARGET = ./hser2net

%.o: %.c *.h ser2net.h
	$(CC) $(CFLAGS) -c $< -o $@

%.o:%.cpp *.h ser2net.h
	$(XX) $(CFLAGS) -c $< -o $@

SOURCES = $(wildcard *.c *.cpp)
OBJS = client.o at_command.o config_command.o key.o des.o flash_mtd.o sha.o udp_server.o
OBJS += common/net.o common/hlk_memory.o common/uci_ctrl.o common/parse_wifi6.o

hser2net:$(TARGET)

$(TARGET) : $(OBJS) ser2net.h
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -lpthread  -o $(TARGET)

clean:
	rm -rf *.o $(TARGET)
