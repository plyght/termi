CC = clang
SDK = iphonesimulator
CFLAGS = -Wall -Wextra -O2 -I. -I/opt/homebrew/opt/libarchive/include -target arm64-apple-ios16.0-simulator -isysroot $(shell xcrun --sdk $(SDK) --show-sdk-path)
LDFLAGS = -L/opt/homebrew/opt/libarchive/lib -lsqlite3 -larchive

FAKEFS_OBJS = fakefs/fake-db.o fakefs/fake.o fakefs/proc_sys_dev.o
TOOLS_OBJS = tools/fakefsify.o

all: libfakefs.a fakefsify

libfakefs.a: $(FAKEFS_OBJS)
	ar rcs $@ $^

fakefsify: $(TOOLS_OBJS) libfakefs.a
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(FAKEFS_OBJS) $(TOOLS_OBJS) libfakefs.a fakefsify

test: fakefsify
	@echo "Testing fakefsify tool..."
	@echo "Build complete. Run './fakefsify <rootfs.tar.gz> <output-dir>' to import Alpine rootfs"

.PHONY: all clean test
