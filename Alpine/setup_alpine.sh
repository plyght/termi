#!/bin/bash
set -e

ALPINE_VERSION="3.19.0"
ALPINE_ARCH="aarch64"
ALPINE_URL="https://dl-cdn.alpinelinux.org/alpine/v3.19/releases/${ALPINE_ARCH}/alpine-minirootfs-${ALPINE_VERSION}-${ALPINE_ARCH}.tar.gz"

echo "Setting up Alpine Linux ARM64 for termi..."

if [ ! -f "../Filesystem/fakefsify" ]; then
    echo "Error: fakefsify tool not built. Run 'make' in Filesystem/ first."
    exit 1
fi

echo "Downloading Alpine Linux ${ALPINE_VERSION} ARM64..."
if [ ! -f "alpine-minirootfs-${ALPINE_VERSION}-${ALPINE_ARCH}.tar.gz" ]; then
    curl -LO "$ALPINE_URL"
else
    echo "Alpine rootfs tarball already downloaded."
fi

echo "Importing Alpine rootfs to fake filesystem..."
rm -rf rootfs
../Filesystem/fakefsify "alpine-minirootfs-${ALPINE_VERSION}-${ALPINE_ARCH}.tar.gz" rootfs

echo "Setting up Alpine repositories..."
cat > rootfs/data/etc/apk/repositories << 'EOF'
https://dl-cdn.alpinelinux.org/alpine/v3.19/main
https://dl-cdn.alpinelinux.org/alpine/v3.19/community
EOF

echo "Creating necessary directories..."
mkdir -p rootfs/data/proc
mkdir -p rootfs/data/sys
mkdir -p rootfs/data/dev
mkdir -p rootfs/data/dev/pts
mkdir -p rootfs/data/tmp
chmod 1777 rootfs/data/tmp

echo "Setting up /etc/passwd and /etc/group..."
cat > rootfs/data/etc/passwd << 'EOF'
root:x:0:0:root:/root:/bin/sh
EOF

cat > rootfs/data/etc/group << 'EOF'
root:x:0:
EOF

echo "Alpine Linux ARM64 rootfs ready!"
echo ""
echo "Directory structure:"
echo "  rootfs/data/      - Actual filesystem contents"
echo "  rootfs/meta.db    - SQLite metadata database"
echo ""
echo "Next steps:"
echo "  1. Integrate this filesystem with the ARM64 emulator"
echo "  2. Mount virtual filesystems (/proc, /sys, /dev)"
echo "  3. Initialize syscall layer to use fakefs operations"
echo ""
echo "To test Alpine packages:"
echo "  # apk update"
echo "  # apk add bash coreutils"
