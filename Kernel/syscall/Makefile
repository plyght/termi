CC = clang
SDK = iphoneos
CFLAGS = -Wall -Wextra -Werror -Wno-cast-function-type-mismatch -std=c11 -O2 -I../include -target arm64-apple-ios16.0 -isysroot $(shell xcrun --sdk $(SDK) --show-sdk-path)
ARCH =

SOURCES = calls.c process.c fs.c memory.c network.c user_access.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = libsyscall.a

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rcs $@ $(OBJECTS)

%.o: %.c calls.h ../include/types.h
	$(CC) $(CFLAGS) $(ARCH) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	@echo "Syscall translation layer built successfully"
	@nm $(TARGET) | grep "T _sys_" | wc -l | xargs echo "Implemented syscalls:"
