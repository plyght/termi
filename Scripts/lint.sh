#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT" || exit 1

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

ERRORS=0
WARNINGS=0
MODE="${1:-check}"

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
    WARNINGS=$((WARNINGS + 1))
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
    ERRORS=$((ERRORS + 1))
}

run_swift_checks() {
    print_header "Swift Checks"

    if command -v swiftlint &>/dev/null; then
        echo "Running SwiftLint..."
        if [ "$MODE" = "fix" ]; then
            if swiftlint --fix --quiet; then
                print_success "SwiftLint auto-fixed issues"
            fi
        fi

        if swiftlint lint --quiet --reporter emoji; then
            print_success "SwiftLint passed"
        else
            print_error "SwiftLint found issues"
        fi
    else
        print_warning "SwiftLint not installed (install with: wax install swiftlint)"
    fi

    if command -v swiftformat &>/dev/null; then
        echo "Running SwiftFormat..."
        if [ "$MODE" = "fix" ]; then
            if swiftformat . --quiet; then
                print_success "SwiftFormat fixed files"
            fi
        else
            if swiftformat --lint . --quiet; then
                print_success "SwiftFormat passed"
            else
                print_error "SwiftFormat found issues (run with 'fix' to auto-format)"
            fi
        fi
    else
        print_warning "SwiftFormat not installed (install with: wax install swiftformat)"
    fi

    echo "Checking Swift compilation..."
    if swift build >/dev/null 2>&1; then
        print_success "Swift builds successfully"
    else
        print_warning "Swift build failed (may be dependency issue)"
        echo "Run 'swift build' manually to see details"
    fi
}

run_python_checks() {
    print_header "Python Checks"

    if command -v ruff &>/dev/null; then
        echo "Running ruff..."
        if [ "$MODE" = "fix" ]; then
            if ruff check --fix --exclude ish-repo .; then
                print_success "Ruff auto-fixed issues"
            fi
            if ruff format --exclude ish-repo .; then
                print_success "Ruff formatted files"
            fi
        fi

        if ruff check --exclude ish-repo .; then
            print_success "Ruff check passed"
        else
            print_error "Ruff found issues"
        fi
    else
        print_warning "Ruff not installed (install with: wax install ruff)"
    fi
}

run_c_checks() {
    print_header "C/C++ Checks"

    CLANG_FORMAT="/opt/homebrew/opt/llvm/bin/clang-format"

    if [ -x "$CLANG_FORMAT" ]; then
        echo "Running clang-format..."
        C_FILES=$(find . \( -name "*.c" -o -name "*.h" \) -not -path "./ish-repo/*" -not -path "./.build/*")

        if [ -n "$C_FILES" ]; then
            if [ "$MODE" = "fix" ]; then
                echo "$C_FILES" | xargs "$CLANG_FORMAT" -i
                print_success "Clang-format fixed C files"
            else
                if echo "$C_FILES" | xargs "$CLANG_FORMAT" --dry-run --Werror 2>&1 | grep -q "error:"; then
                    print_error "Clang-format found issues (run with 'fix' to auto-format)"
                else
                    print_success "Clang-format passed"
                fi
            fi
        else
            print_success "No C files to check"
        fi
    else
        print_warning "clang-format not found (install with: wax install llvm)"
    fi
}

run_shell_checks() {
    print_header "Shell Script Checks"

    if command -v shellcheck &>/dev/null; then
        echo "Running shellcheck..."
        SHELL_FILES=$(find . -name "*.sh" -not -path "./ish-repo/*" -not -path "./.build/*")

        if [ -n "$SHELL_FILES" ]; then
            if echo "$SHELL_FILES" | xargs shellcheck; then
                print_success "Shellcheck passed"
            else
                print_error "Shellcheck found issues"
            fi
        else
            print_success "No shell scripts to check"
        fi
    else
        print_warning "Shellcheck not installed (install with: wax install shellcheck)"
    fi

    if command -v shfmt &>/dev/null; then
        echo "Running shfmt..."
        SHELL_FILES=$(find . -name "*.sh" -not -path "./ish-repo/*" -not -path "./.build/*")

        if [ -n "$SHELL_FILES" ]; then
            if [ "$MODE" = "fix" ]; then
                echo "$SHELL_FILES" | xargs shfmt -w -i 4
                print_success "Shfmt formatted shell scripts"
            else
                if echo "$SHELL_FILES" | xargs shfmt -d -i 4 >/dev/null; then
                    print_success "Shfmt passed"
                else
                    print_error "Shfmt found issues (run with 'fix' to auto-format)"
                fi
            fi
        fi
    else
        print_warning "Shfmt not installed (install with: wax install shfmt)"
    fi
}

main() {
    if [ "$MODE" != "check" ] && [ "$MODE" != "fix" ]; then
        echo "Usage: $0 [check|fix]"
        echo "  check - Run all linters and formatters in check mode (default)"
        echo "  fix   - Run all formatters and auto-fix issues"
        exit 1
    fi

    echo -e "${BLUE}Starting quality checks in ${YELLOW}${MODE}${BLUE} mode...${NC}\n"

    run_swift_checks
    echo ""
    run_python_checks
    echo ""
    run_c_checks
    echo ""
    run_shell_checks

    echo ""
    print_header "Summary"

    if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
        print_success "All checks passed! 🎉"
        exit 0
    elif [ $ERRORS -eq 0 ]; then
        print_warning "$WARNINGS warning(s) found"
        exit 0
    else
        print_error "$ERRORS error(s) and $WARNINGS warning(s) found"
        if [ "$MODE" = "check" ]; then
            echo -e "${YELLOW}Run '$0 fix' to auto-fix formatting issues${NC}"
        fi
        exit 1
    fi
}

main
