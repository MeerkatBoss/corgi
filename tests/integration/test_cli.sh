#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

setup() {
    rm -rf "$SOURCE_DIR" "$TARGET_DIR"
    mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
}

setup_file() {
    setup
    create_test_file "$SOURCE_DIR/file.txt"
}

# == Normal execution tests ==

test_group "Basic usage"
    setup

    assert_success "Just works" \
        "$BINARY" --source "$SOURCE_DIR" \
                  --target "$TARGET_DIR"
finish_test || exit 1

test_group "Create missing directory"
    setup_file
    rm -r "$TARGET_DIR"

    assert_success "Still works" \
        "$BINARY" --source "$SOURCE_DIR" \
                  --target "$TARGET_DIR"
    assert_directory_exists "Target directory created" "$TARGET_DIR"
finish_test || exit 1

test_group "Valid tags"
    setup_file

    assert_success "Accepts valid tags" \
        "$BINARY" --source "$SOURCE_DIR" \
                  --target "$TARGET_DIR" \
                  --tag "vacation" --tag "summer-trip" --dry-run
finish_test || exit 1

test_group "Dry-run mode"
    setup_file

    assert_success "Accepts --dry-run" \
        "$BINARY" --source "$SOURCE_DIR" \
                  --target "$TARGET_DIR" \
                  --tag "dryrun" --dry-run

    assert_file_count "No files created" "$TARGET_DIR" 0
    assert_file_count "Source file still exists" "$SOURCE_DIR" 1
finish_test || exit 1

test_group "Dry-run mode without target"
    setup_file
    rm -r "$TARGET_DIR"

    assert_success "Accepts --dry-run" \
        "$BINARY" --source "$SOURCE_DIR" \
                  --target "$TARGET_DIR" \
                  --tag "dryrun" --dry-run

    assert_directory_not_exists "Target directory not created" "$TARGET_DIR"
finish_test || exit 1

test_group "Verbose mode"
    setup
    rm -r "$TARGET_DIR"
    create_test_file "$SOURCE_DIR/file1.txt"
    create_test_file "$SOURCE_DIR/file2.txt"

    output=$("$BINARY" --source "$SOURCE_DIR" \
                      --target "$TARGET_DIR" \
                      --tag "verbose" --verbose 2>&1)

    assert_contains "File count reported" "$output" "Found 2 files"
    assert_contains "Target creation reported" \
        "$output" "Target directory '$TARGET_DIR' does not exist, creating it"
    assert_contains "Success reported" \
        "$output" "Successfully processed 2 files"
    assert_contains "Prepare reported" "$output" "Preparing 2 operations"
    assert_contains "Commit reported" "$output" "Committing 2 operations"
    assert_contains "Copy reported" "$output" "copy: $SOURCE_DIR/file1.txt ->"
    assert_contains "NOP reported" "$output" "Nothing to commit"
finish_test || exit 1

test_group "Help command"
    output=$("$BINARY" --help 2>&1 || true)
    assert_contains "Help shows usage" "$output" "Usage:"
    assert_contains "Help shows --source" "$output" "--source"
    assert_contains "Help shows show --target" "$output" "--target"
finish_test || exit 1

# == Edge cases ==

test_group "Long options with = separator"
    setup_file

    assert_success "Accepts --source=DIR --target=DIR" \
        "$BINARY" --source="$SOURCE_DIR" --target="$TARGET_DIR" --dry-run

    output=$("$BINARY" --source="$SOURCE_DIR" --target="$TARGET_DIR" \
                       --tag=hello --verbose --dry-run 2>&1)
    assert_contains "Source path parsed" \
        "$output" "Found 1 files in '$SOURCE_DIR'"
    assert_contains "Tag parsed" "$output" "hello"
finish_test || exit 1

test_group "Long options with space separator"
    setup_file

    assert_success "Accepts --source DIR --target DIR" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" --dry-run

    output=$("$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                       --tag hello --verbose --dry-run 2>&1)
    assert_contains "Source path parsed" \
        "$output" "Found 1 files in '$SOURCE_DIR'"
    assert_contains "Tag parsed" "$output" "hello"
finish_test || exit 1

test_group "Long options with mixed separators"
    setup_file

    assert_success "Accepts --source=DIR --target DIR" \
        "$BINARY" --source="$SOURCE_DIR" --target "$TARGET_DIR" --dry-run

    assert_success "Accepts --source DIR --target=DIR" \
        "$BINARY" --source "$SOURCE_DIR" --target="$TARGET_DIR" --dry-run

    output=$("$BINARY" --source="$SOURCE_DIR" --target "$TARGET_DIR" \
                       --tag=hello --tag world --verbose --dry-run 2>&1)
    assert_contains "Tag with = parsed" "$output" "hello"
    assert_contains "Tag with space parsed" "$output" "world"
finish_test || exit 1

test_group "Short options with space separator"
    setup_file

    assert_success "Accepts -s DIR -d DIR" \
        "$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" --dry-run

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" \
                       -t hello -v --dry-run 2>&1)
    assert_contains "Source path parsed" \
        "$output" "Found 1 files in '$SOURCE_DIR'"
    assert_contains "Tag parsed" "$output" "hello"
finish_test || exit 1

test_group "Short options with attached value"
    setup_file

    assert_success "Accepts -sDIR -dDIR" \
        "$BINARY" -s"$SOURCE_DIR" -d"$TARGET_DIR" --dry-run

    output=$("$BINARY" -s"$SOURCE_DIR" -d"$TARGET_DIR" \
                       -thello -v --dry-run 2>&1)
    assert_contains "Source path parsed" \
        "$output" "Found 1 files in '$SOURCE_DIR'"
    assert_contains "Tag parsed" "$output" "hello"
finish_test || exit 1

test_group "Short options with mixed value styles"
    setup_file

    assert_success "Space for -s, attached for -d" \
        "$BINARY" -s "$SOURCE_DIR" -d"$TARGET_DIR" --dry-run

    assert_success "Attached for -s, space for -d" \
        "$BINARY" -s"$SOURCE_DIR" -d "$TARGET_DIR" --dry-run

    output=$("$BINARY" -s "$SOURCE_DIR" -d"$TARGET_DIR" \
                       -t hello -tworld -v --dry-run 2>&1)
    assert_contains "Tag with space parsed" "$output" "hello"
    assert_contains "Tag attached parsed" "$output" "world"
finish_test || exit 1

test_group "Combined short boolean flags"
    setup_file

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" \
                       -vf --dry-run 2>&1)
    assert_contains "Verbose active with -vf" "$output" "Found 1 files"

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" \
                       -fv --dry-run 2>&1)
    assert_contains "Verbose active with -fv" "$output" "Found 1 files"
finish_test || exit 1

test_group "Combined short flags with value option"
    setup_file

    output=$("$BINARY" -vfs "$SOURCE_DIR" -d "$TARGET_DIR" --dry-run 2>&1)
    assert_contains "Verbose active with -vfs DIR" "$output" "Found 1 files"

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" \
                       -vft hello --dry-run 2>&1)
    assert_contains "Verbose active with -vft TAG" "$output" "Found 1 files"
    assert_contains "Tag parsed from -vft TAG" "$output" "hello"

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" \
                       -vfthello --dry-run 2>&1)
    assert_contains "Verbose active with -vfthello" "$output" "Found 1 files"
    assert_contains "Tag parsed from -vfthello" "$output" "hello"
finish_test || exit 1

test_group "Mixed long and short options"
    setup_file

    output=$("$BINARY" --source "$SOURCE_DIR" -d "$TARGET_DIR" \
                    -t hello --verbose --dry-run 2>&1)
    assert_contains "Long --source with short -d" "$output" "Found 1 files"
    assert_contains "Short -t tag parsed" "$output" "hello"

    output=$("$BINARY" -s "$SOURCE_DIR" --target="$TARGET_DIR" \
                    --tag=world -v --dry-run 2>&1)
    assert_contains "Short -s with long --target=" "$output" "Found 1 files"
    assert_contains "Long --tag= parsed" "$output" "world"
finish_test || exit 1

test_group "Option ordering independence"
    setup_file

    output1=$("$BINARY" -t hello --dry-run -v \
                        -s "$SOURCE_DIR" -d "$TARGET_DIR" 2>&1)
    assert_contains "Tags before dirs accepted" "$output1" "Found 1 files"
    assert_contains "Tag correctly parsed" "$output1" "hello"

    output2=$("$BINARY" -s "$SOURCE_DIR" -v \
                        -d "$TARGET_DIR" -f -t hello --dry-run 2>&1)
    assert_contains "Interspersed flags accepted" "$output2" "Found 1 files"
    assert_contains "Tag in mixed order" "$output2" "hello"

    assert_success "Same output" test "'$output1'" = "'$output2'"
finish_test || exit 1

test_group "Force flag with -f and --force"
    setup_file

    "$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" -t test 2>/dev/null

    assert_failure "Collision without force" \
        "$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" -t test

    assert_success "-f resolves collision" \
        "$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" -t test -f

    assert_success "--force resolves collision" \
        "$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" -t test --force
finish_test || exit 1

test_group "Multiple tags via mixed syntax"
    setup_file

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" \
                       -t alpha -tbeta --tag gamma --tag=delta \
                       -v --dry-run 2>&1)
    assert_contains "Tag alpha" "$output" "alpha"
    assert_contains "Tag beta" "$output" "beta"
    assert_contains "Tag gamma" "$output" "gamma"
    assert_contains "Tag delta" "$output" "delta"
finish_test || exit 1

test_group "Option terminator --"
    setup

    assert_success "Trailing -- accepted" \
        "$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" --dry-run --

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" -- --dry-run 2>&1 \
            || true)
    assert_contains "-- stops option parsing" "$output" "Unexpected argument"

    output=$("$BINARY" -s "$SOURCE_DIR" -- -d "$TARGET_DIR" 2>&1 || true)
    assert_contains "-- prevents -d parsing" \
        "$output" "--source and --target are required"

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" --dry-run \
                       -- extra 2>&1 || true)
    assert_contains "Positional after -- rejected" \
        "$output" "Unexpected argument"
    assert_contains "Reports the argument" "$output" "extra"
finish_test || exit 1

test_group "Help options"
    output1=$("$BINARY" --help 2>&1 || true)
    assert_contains "--help shows usage" "$output1" "Usage:"
    assert_contains "--help shows --source" "$output1" "--source"
    assert_contains "--help shows --target" "$output1" "--target"

    output2=$("$BINARY" -h 2>&1 || true)
    assert_contains "-h shows usage" "$output2" "Usage:"

    assert_success "Same output" test "'$output1'" = "'$output2'"
finish_test || exit 1

test_group "Long option abbreviation"
    setup

    assert_success "--so for --source" \
        "$BINARY" --so "$SOURCE_DIR" --target "$TARGET_DIR" --dry-run

    assert_success "--tar for --target" \
        "$BINARY" --source "$SOURCE_DIR" --tar "$TARGET_DIR" --dry-run

    assert_success "--verb for --verbose" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" --verb --dry-run

    assert_success "--fo for --force" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" --fo --dry-run

    assert_success "--dr for --dry-run" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" --dr

    assert_success "--he for --help" \
        "$BINARY" --he

    assert_failure "Reject ambiguous --ta" \
        "$BINARY" --source "$SOURCE_DIR" --ta "$TARGET_DIR" --dry-run
finish_test || exit 1

test_group "Repeated options"
    setup

    SECOND_SOURCE="$TEST_DIR/source2"
    rm -rf "$SECOND_SOURCE"
    mkdir -p "$SECOND_SOURCE"

    assert_failure "Repeated options not allowed" \
        "$BINARY" -s "$SOURCE_DIR" -s "$SECOND_SOURCE" \
                  -d "$TARGET_DIR" -v --dry-run 2>&1
finish_test || exit 1


# == Errors in usage ==

test_group "Missing required options"
    assert_failure "Fails with no arguments" "$BINARY"

    output=$("$BINARY" 2>&1 || true)
    assert_contains "Reports missing options" \
        "$output" "--source and --target are required"
    assert_contains "Shows help on error" "$output" "Usage:"

    output=$("$BINARY" -s "$SOURCE_DIR" 2>&1 || true)
    assert_contains "Missing target reported" \
        "$output" "--source and --target are required"

    output=$("$BINARY" -d "$TARGET_DIR" 2>&1 || true)
    assert_contains "Missing source reported" \
        "$output" "--source and --target are required"

    output=$("$BINARY" -v -f --dry-run 2>&1 || true)
    assert_contains "Missing target and source reported" \
        "$output" "--source and --target are required"
finish_test || exit 1

test_group "Missing option arguments"
    output=$("$BINARY" -s "$SOURCE_DIR" -d 2>&1 || true)
    assert_contains "Missing arg for -d" "$output" "Missing argument"

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" -t 2>&1 || true)
    assert_contains "Missing arg for -t" "$output" "Missing argument"

    output=$("$BINARY" -s 2>&1 || true)
    assert_contains "Missing arg for -s" "$output" "Missing argument"

    output=$("$BINARY" --source 2>&1 || true)
    assert_contains "Missing arg for --source" "$output" "Missing argument"
    assert_contains "Reports option name" "$output" "--source"

    output=$("$BINARY" -s "$SOURCE_DIR" --target 2>&1 || true)
    assert_contains "Missing arg for --target" "$output" "Missing argument"
    assert_contains "Reports option name" "$output" "--target"

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" --tag 2>&1 || true)
    assert_contains "Missing arg for --tag" "$output" "Missing argument"
    assert_contains "Reports option name" "$output" "--tag"
finish_test || exit 1

test_group "Unknown options"
    assert_failure "Rejects -x" \
        "$BINARY" -x -s "$SOURCE_DIR" -d "$TARGET_DIR"

    output=$("$BINARY" -x -s "$SOURCE_DIR" -d "$TARGET_DIR" 2>&1 || true)
    assert_contains "Reports invalid -x" "$output" "Invalid option"
finish_test || exit 1

test_group "Positional arguments"
    setup

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" extra 2>&1 || true)
    assert_contains "Rejects positional arg" "$output" "Unexpected argument"
    assert_contains "Reports the argument" "$output" "extra"

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" \
                       --dry-run foo bar 2>&1 || true)
    assert_contains "Rejects first positional" "$output" "Unexpected argument"
    assert_contains "Reports first positional" "$output" "foo"
finish_test || exit 1

test_group "Too many tags"
    setup_file

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" --dry-run \
        -t a -t b -t c -t d -t e -t f -t g -t h \
        -t i -t j -t k -t l -t m -t n -t o -t p \
        -t q 2>&1 || true)
    assert_contains "Reports too many tags" "$output" "Too many tags"

    assert_success "Accepts 8 tags" \
        "$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" --dry-run \
        -t a -t b -t c -t d -t e -t f -t g -t h

    output=$("$BINARY" -s "$SOURCE_DIR" -d "$TARGET_DIR" --dry-run \
        -t a -t b -t c -t d -t e -t f -t g -t h \
        -t i -t j -t k -t l -t m -t n -t o -t p 2>&1 || true)
    assert_contains "16 tags: CLI accepts, file limit reached" \
        "$output" "Failed to add tags"
finish_test || exit 1

exit 0
