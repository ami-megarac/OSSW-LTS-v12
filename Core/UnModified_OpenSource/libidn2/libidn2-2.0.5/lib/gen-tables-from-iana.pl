#!/usr/bin/perl -w

# Copyright (C) 2011-2017 Simon Josefsson

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# I consider the output of this program to be unrestricted.  Use it as
# you will.

use strict;

my ($intable) = 0;
my ($line, $start, $end, $state);
my ($tablesize) = 0;

print "/* This file is automatically generated.  DO NOT EDIT! */\n\n";

print "#include <config.h>\n";
print "#include \"data.h\"\n";
print "\n";
print "const struct idna_table idna_table\[\] = {\n";

while(<>) {
    $intable = 1 if m,^    Codepoint    Property,;
    next if !$intable;

    next if m,^    Codepoint    Property,;
    next if m,^ +PROSGEGRAMMENI$,;
    next if m, UNASSIGNED ,;

    if (m, +([0-9A-F]+)(-([0-9A-F]+))? (PVALID|CONTEXTJ|CONTEXTO|DISALLOWED) ,) {
	$start = $1;
	$end = $3;
	$state = $4;
	printf "  {0x$start, 0x$end, $state},\n" if $end;
	printf "  {0x$start, 0x$start, $state},\n" if !$end;
   $tablesize++;
    } else {
	die "regexp failed on line: -->$_<--";
    }
}

print "};\n";
print "const size_t idna_table_size = $tablesize;\n";

exit 0;