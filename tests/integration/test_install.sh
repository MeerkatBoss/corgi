#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

INSTALL_DIR="$TEST_DIR/install-test"
MAKE="${MAKE:-make}"

test_group "Install binary"
    rm -rf "$INSTALL_DIR"

    "$MAKE" install DESTDIR="$INSTALL_DIR" prefix=/usr > /dev/null 2>&1

    assert_file_exists "Binary installed" \
        "$INSTALL_DIR/usr/bin/corgi"

    assert_success "Binary is executable" \
        test -x "$INSTALL_DIR/usr/bin/corgi"

    mode=$(stat -c '%a' "$INSTALL_DIR/usr/bin/corgi" 2>/dev/null \
           || stat -f '%OLp' "$INSTALL_DIR/usr/bin/corgi" 2>/dev/null)
    assert_success "Binary has mode 755" \
        test "$mode" = "755"

    assert_success "Installed binary runs" \
        "$INSTALL_DIR/usr/bin/corgi" --help
finish_test || exit 1

test_group "Uninstall binary"
    "$MAKE" uninstall DESTDIR="$INSTALL_DIR" prefix=/usr > /dev/null 2>&1

    assert_file_not_exists "Binary removed" \
        "$INSTALL_DIR/usr/bin/corgi"
finish_test || exit 1

exit 0
