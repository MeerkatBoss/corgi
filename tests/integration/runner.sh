#!/bin/sh

set -eu

TEST_DIR="${TEST_DIR:-.tmp/tests}"
BINARY="${CORGI_BINARY:-build/bin/corgi}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

colors=no
case "$TERM" in
    xterm-color|*-256color) colors=yes;;
esac

if [ "$colors" = "yes" ]; then
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    YELLOW='\033[0;33m'
    NC='\033[0m'
else
    GREEN=""
    RED=""
    YELLOW=""
    NC=""
fi

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

export TEST_DIR
export BINARY
export SCRIPT_DIR

export GREEN
export RED
export YELLOW
export NC

setup_test_env() {
    mkdir -p "$TEST_DIR"
    rm -rf "${TEST_DIR:?}"/*
}

run_test_file() {
    local test_file="$1"
    echo "============================"
    printf "${YELLOW}Running %s...${NC}\n" "$(basename "$test_file")"

    if sh "$test_file"; then
        echo ""
        printf "${GREEN}[OK] %s passed${NC}\n\n" "$(basename "$test_file")"
        return 0
    else
        echo ""
        printf "${RED}[FAIL] %s failed${NC}\n\n" "$(basename "$test_file")"
        return 1
    fi
}

main() {
    echo "Corgi Integration Test Suite"
    echo "----------------------------"
    echo "Test directory: $TEST_DIR"
    echo "Binary: $BINARY"
    echo ""

    if [ ! -x "$BINARY" ]; then
        printf "${RED}Error: Binary not found at %s${NC}\n" "$BINARY"
        echo "Run 'make all' first"
        exit 1
    fi

    setup_test_env

    for test_file in "$SCRIPT_DIR"/test_*.sh; do
        if [ -f "$test_file" ]; then
            TOTAL_TESTS=$((TOTAL_TESTS + 1))
            if run_test_file "$test_file"; then
                PASSED_TESTS=$((PASSED_TESTS + 1))
            else
                FAILED_TESTS=$((FAILED_TESTS + 1))
            fi
        fi
    done

    echo "============================"
    echo "Tests run: $TOTAL_TESTS"
    printf "Passed: ${GREEN}%s${NC}\n" "$PASSED_TESTS"
    printf "Failed: ${RED}%s${NC}\n" "$FAILED_TESTS"

    if [ "$FAILED_TESTS" -eq 0 ]; then
        printf "${GREEN}All tests passed!${NC}\n"
        exit 0
    else
        printf "${RED}Some tests failed!${NC}\n"
        exit 1
    fi
}

main "$@"
