#!/usr/bin/env bash

#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Fails when the changes between two commits add a wxASSERT that guards a null pointer or file handle.
# wxASSERT compiles away in release builds.  Use wxCHECK, wxCHECK_RET or wxCHECK2 and bail out instead.

usage='usage: check_wxassert_null.sh <base-rev> [<head-rev>]'

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "$usage" >&2
    exit 2
fi

base="$1"
head="${2:-HEAD}"

cd "$(git rev-parse --show-toplevel)" || exit 2

set -o pipefail

for rev in "$base" "$head"; do
    if ! git rev-parse --quiet --verify "${rev}^{commit}" > /dev/null; then
        echo "check_wxassert_null.sh: unknown commit '${rev}'" >&2
        exit 2
    fi
done

git diff -U0 --no-color --no-ext-diff --diff-filter=AMR "$base" "$head" -- \
        '*.c' '*.cpp' '*.cxx' '*.h' '*.hpp' '*.mm' ':(exclude)thirdparty/**' |
    awk '
        # Only parse ---/+++ inside a file header so added lines starting with "++" are not mistaken for one
        /^diff --git / { in_header = 1; next }
        in_header && /^\+\+\+ / { file = substr( $0, 7 ); next }
        /^@@ / {
            in_header = 0
            match( $0, /\+[0-9]+/ )
            line = substr( $0, RSTART + 1, RLENGTH - 1 ) - 1
            next
        }
        !in_header && /^\+/ {
            line++

            if( $0 ~ /wxASSERT(_MSG)?[ \t]*\([ \t]*[^,;]*!=[ \t]*(nullptr|NULL)([^[:alnum:]_]|$)/ ||
                $0 ~ /wxASSERT(_MSG)?[ \t]*\([^;]*(IsOk|IsOpened)[ \t]*\(\)/ )
            {
                print file ":" line ": " substr( $0, 2 )
                found = 1
            }

            next
        }
        END {
            if( found )
            {
                print ""
                print "wxASSERT must not guard null pointers or file handles.  Use wxCHECK* and take the least bad action instead."
                exit 1
            }
        }'
