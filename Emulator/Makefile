CC = clang
SDK = iphoneos
CFLAGS = -Wall -Wextra -O2 -g -I. -I../Kernel/include -target arm64-apple-ios16.0 -isysroot $(shell xcrun --sdk $(SDK) --show-sdk-path)
LDFLAGS =

SRCS = cpu/arm64_cpu.c \
       cpu/decoder.c \
       cpu/interpreter.c \
       mmu/mmu.c \
       loader/elf_loader.c \
       emulator.c

OBJS = $(SRCS:.c=.o)

SYSCALL_OBJS = ../Kernel/syscall/calls.o \
               ../Kernel/syscall/fs.o \
               ../Kernel/syscall/memory.o \
               ../Kernel/syscall/process.o \
               ../Kernel/syscall/network.o \
               ../Kernel/syscall/user_access.o

TARGET = libemulator.a

all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f cpu/*.o mmu/*.o loader/*.o

test: $(TARGET)
	$(CC) $(CFLAGS) test_emulator.c $(TARGET) $(SYSCALL_OBJS) $(LDFLAGS) -o test_emulator

.PHONY: all clean test
