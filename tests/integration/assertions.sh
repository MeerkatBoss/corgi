#!/bin/sh

TEST_NAME=""
ASSERTION_COUNT=0
FAILED_ASSERTIONS=0

_fail() {
    ASSERTION_COUNT=$((ASSERTION_COUNT + 1))
    FAILED_ASSERTIONS=$((FAILED_ASSERTIONS + 1))
    printf "    ${RED}[FAIL]${NC} %s\n" "$*"
}

_pass() {
    ASSERTION_COUNT=$((ASSERTION_COUNT + 1))
    printf "    ${GREEN}[OK]${NC} %s\n" "$*"
}

test_group() {
    TEST_NAME="$1"
    ASSERTION_COUNT=0
    FAILED_ASSERTIONS=0
    echo ""
    echo "  Testing: $TEST_NAME"
}

assert_success() {
    local description="$1"
    shift

    if "$@" > /dev/null 2>&1; then
        _pass $description
        return 0
    else
        _fail $description
        echo "      Command failed: $*"
        return 1
    fi
}

assert_failure() {
    local description="$1"
    shift

    if ! "$@" > /dev/null 2>&1; then
        _pass $description
        return 0
    else
        _fail $description
        echo "      Command should have failed: $*"
        return 1
    fi
}

assert_file_exists() {
    local description="$1"
    local file="$2"

    if [ -f "$file" ]; then
        _pass $description
        return 0
    else
        _fail $description
        echo "      File not found: $file"
        return 1
    fi
}

assert_directory_exists() {
    local description="$1"
    local directory="$2"

    if [ -d "$directory" ]; then
        _pass $description
        return 0
    else
        _fail $description
        echo "      Directory not found: $directory"
        return 1
    fi
}

assert_file_not_exists() {
    local description="$1"
    local file="$2"

    if [ ! -f "$file" ]; then
        _pass $description
        return 0
    else
        _fail $description
        echo "      File should not exist: $file"
        return 1
    fi
}

assert_directory_not_exists() {
    local description="$1"
    local directory="$2"

    if [ ! -d "$directory" ]; then
        _pass $description
        return 0
    else
        _fail $description
        echo "      Directory should not exist: $directory"
        return 1
    fi
}

assert_contains() {
    local description="$1"
    local haystack="$2"
    local needle="$3"

    if echo -- "$haystack" | grep -F -o -- "$needle" > /dev/null; then
        _pass $description
        return 0
    else
        _fail $description
        echo "      Expected to find: $needle"
        echo "      In: $haystack"
        return 1
    fi
}

assert_contains_count() {
    local description="$1"
    local haystack="$2"
    local needle="$3"
    local count="$4"

    local actual_count=$(echo "$haystack" | grep -F -o -- "$needle" | wc -l)
    if [ "$actual_count" -eq "$count" ]; then
        _pass $description
        return 0
    else
        _fail $description
        echo "      Expected to find $count times: $needle"
        echo "      In: $haystack"
        echo "      Found $actual_count times instead"
    fi

}


assert_filename_matches() {
    local description="$1"
    local filename="$2"
    local pattern="$3"

    if echo "$filename" | grep -qE "$pattern"; then
        _pass $description
        return 0
    else
        _fail $description
        echo "      Filename: $filename"
        echo "      Expected pattern: $pattern"
        return 1
    fi
}

assert_files_identical() {
    local description="$1"
    local file1="$2"
    local file2="$3"

    if cmp -s "$file1" "$file2"; then
        _pass $description
        return 0
    else
        _fail $description
        echo "      Files differ: $file1 vs $file2"
        return 1
    fi
}

assert_file_count() {
    local description="$1"
    local directory="$2"
    local expected_count="$3"

    local actual_count
    actual_count=$(find "$directory" -type f 2>/dev/null | wc -l)

    if [ "$actual_count" -eq "$expected_count" ]; then
        _pass $description
        return 0
    else
        _fail $description
        echo "      Expected: $expected_count files"
        echo "      Found: $actual_count files"
        return 1
    fi
}

create_test_file() {
    local filepath="$1"
    local content="${2:-test content}"

    mkdir -p "$(dirname "$filepath")"
    echo "$content" > "$filepath"
}

finish_test() {
    if [ "$FAILED_ASSERTIONS" -eq 0 ]; then
        printf "  ${GREEN}All assertions passed (%s/%s)${NC}\n" \
            "$ASSERTION_COUNT" "$ASSERTION_COUNT"
        return 0
    else
        printf "  ${RED}Failed: %s/%s assertions${NC}\n" \
            "$FAILED_ASSERTIONS" "$ASSERTION_COUNT"
        return 1
    fi
}
