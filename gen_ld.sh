#!/bin/sh
gcc=$1
in=$2
out=$3
shift
shift
shift
c_flags=$*

echo "/* Auto generated file - do not modify */" > $out
$gcc -E -x c $c_flags $in | grep -v '^#' >> $out