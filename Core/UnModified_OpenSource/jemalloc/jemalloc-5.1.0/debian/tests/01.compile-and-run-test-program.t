#!/bin/sh

test_description="compile with jemalloc"

. /usr/share/sharness/sharness.sh

test_expect_success "compile test program against libjemalloc" "
  gcc ../ex_stats_print.c -o ex_stats_print -ljemalloc
"
test_expect_success "run test program" "
 ./ex_stats_print
"

test_done
