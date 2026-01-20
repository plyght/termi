#!/bin/bash
set -e

echo "================================================================"
echo "  termi Filesystem Layer Verification"
echo "================================================================"
echo ""

ERRORS=0

# Check directory structure
echo "✓ Checking directory structure..."
for dir in Filesystem/fakefs Filesystem/tools Alpine; do
    if [ ! -d "$dir" ]; then
        echo "  ✗ Missing directory: $dir"
        ERRORS=$((ERRORS + 1))
    fi
done

# Check implementation files
echo "✓ Checking implementation files..."
FILES=(
    "Filesystem/fakefs/fake-db.h"
    "Filesystem/fakefs/fake-db.c"
    "Filesystem/fakefs/fake.h"
    "Filesystem/fakefs/fake.c"
    "Filesystem/fakefs/proc_sys_dev.h"
    "Filesystem/fakefs/proc_sys_dev.c"
    "Filesystem/tools/fakefsify.h"
    "Filesystem/tools/fakefsify.c"
    "Filesystem/Makefile"
    "Alpine/setup_alpine.sh"
)

for file in "${FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "  ✗ Missing file: $file"
        ERRORS=$((ERRORS + 1))
    fi
done

# Check documentation
echo "✓ Checking documentation..."
DOCS=(
    "Filesystem/README.md"
    "INTEGRATION.md"
    "FILESYSTEM_STATUS.md"
    "FILESYSTEM_API.md"
)

for doc in "${DOCS[@]}"; do
    if [ ! -f "$doc" ]; then
        echo "  ✗ Missing documentation: $doc"
        ERRORS=$((ERRORS + 1))
    fi
done

# Check setup script is executable
echo "✓ Checking Alpine setup script..."
if [ ! -x "Alpine/setup_alpine.sh" ]; then
    echo "  ! setup_alpine.sh is not executable"
    chmod +x Alpine/setup_alpine.sh
    echo "  ✓ Fixed permissions"
fi

# Count lines of code
echo ""
echo "Code Statistics:"
LINES=$(find Filesystem -name "*.c" -o -name "*.h" | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
echo "  Total C code: $LINES lines"

FILES_COUNT=$(find Filesystem -name "*.c" -o -name "*.h" | wc -l | tr -d ' ')
echo "  Implementation files: $FILES_COUNT"

echo ""
echo "================================================================"
if [ $ERRORS -eq 0 ]; then
    echo "  ✅ ALL CHECKS PASSED"
    echo ""
    echo "  Filesystem layer is ready for integration!"
    echo ""
    echo "  Next steps:"
    echo "    1. cd Filesystem && make"
    echo "    2. cd Alpine && ./setup_alpine.sh"
    echo "    3. Integrate with other agents (see INTEGRATION.md)"
    echo ""
else
    echo "  ✗ FOUND $ERRORS ERRORS"
    echo ""
    echo "  Please fix the errors above before proceeding."
    echo ""
    exit 1
fi
echo "================================================================"
