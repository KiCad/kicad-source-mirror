#!/usr/bin/env bash

#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2019 Kicad Developers, see AUTHORS.txt for contributors.
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
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# Simple program to check and fix formatting in KiCad source files,
# while ignoring violations in "uncontrolled" files, such as generated files
# or 3rd party sources.

usage='usage: check_coding.sh [<options>] [--]
     --help                     Print usage plus more detailed help.

     --diff                     Only show proposed changes, do not format files

     --cached                   Re-format changes currently staged for commit (default)
     --amend                    Re-format changes made in the previous commit
     --commit <commit-rev>      Re-format changes made since commit-rev
     --ci                       Run in CI mode to return non-zero when there are formatting errors
'

help="$usage"'
Example to format cached files:
    check_coding.sh

To show what would be done:
    check_coding.sh --diff
'

die() {
    echo "$@" 1>&2; exit 1
}

# Parse command-line arguments.
ci=false
diff=false
mode='cached'

while test "$#" != 0; do
    case "$1" in
    --diff) diff=true ;;
    --amend) mode='amend' ;;
    --cached) mode='cached' ;;
    --commit) mode='commit'
        format_ref_commit="$2"
        shift
        ;;
    --ci) ci=true ;;
    --) shift ; break ;;
    -*) die "$usage" ;;
    *) break ;;
    esac
    shift
done

test "$#" = 0 || die "$usage"

# This is the main KiCad formatting attribute
# used by .gitattributes to mark KiCad source files
format_attribute="format.clang-format-kicad"

format_commit='HEAD'

# Select the git file list command and the commit to check
case "${mode}" in
    '')       echo "$usage"; exit 0 ;;
    commit)
        # Files changed since listed commit
        git_list_files="git diff-tree  --diff-filter=ACM --name-only HEAD ${format_ref_commit} -r --no-commit-id"
        format_commit=${format_ref_commit}
        ;;
    amend)
        # Files changed by the last commit
        git_list_files='git diff-tree  --diff-filter=ACM --name-only HEAD -r --no-commit-id'
        format_commit='HEAD^'
        ;;
    cached)
        # Currently staged files
        git_list_files='git diff-index --diff-filter=ACM --name-only HEAD --cached'
        ;;
    *) die "Invalid mode: $mode" ;;
esac


if [ "${diff}" = true ]; then
    # Only show the proposed changes
    format_command="git clang-format --diff ${format_commit}"
else
    # Actually make the changes
    format_command="git clang-format ${format_commit}"
fi


if [ "${ci}" = true ]; then
    # In CI mode we want to set the return value based on modifications (1 = modifications
    # needed, 0 = no modifications). We must capture the output to do this (since git clang-format
    # will always return 0). By capturing the output, we break the terminal coloring, so we hide
    # this inside a special CI mode.

    format_results="$( \
    ${git_list_files} |

        # Filter sources with the formatting attribute set
        git check-attr ${format_attribute} --stdin |

        # output only the file names
        grep ": set$" |
        cut -d: -f1 |

        # Apply the formatting command
        xargs ${format_command}
    )"

    echo "$format_results"

    # Read the results to see if modifications have been requested
    if [[ $format_results == "no modified files to format" ]] \
        || [[ $format_results == "clang-format did not modify any files" ]];
    then
        true
    else
        false
    fi
else
    ${git_list_files} |

        # Filter sources with the formatting attribute set
        git check-attr ${format_attribute} --stdin |

        # output only the file names
        grep ": set$" |
        cut -d: -f1 |

        # Apply the formatting command
        xargs ${format_command}
fi
