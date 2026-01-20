CC = clang
AR = ar
SDK = iphoneos
CFLAGS = -O2 -Wall -I.. -I../Emulator -I../Filesystem -I../Kernel -target arm64-apple-ios16.0 -isysroot $(shell xcrun --sdk $(SDK) --show-sdk-path)
TARGET = libemulatorwrapper.a
SRCS = EmulatorWrapper.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
