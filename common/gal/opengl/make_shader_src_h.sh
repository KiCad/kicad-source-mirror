#!/bin/bash

# Make a header file containing GLSL source code
echo "Generating headers containing GLSL source code.."

# Source files to be included
SHADER_SRC=( "shader.vert" "shader.frag" )
# Number of shaders
SHADERS_NUMBER=${#SHADER_SRC[@]}
OUTPUT="shader_src.h"
UPDATE=false

# Check if it is necessary to regenerate headers
for filename in "${SHADER_SRC[@]}"
do
	if [[ $filename -nt $OUTPUT ]]; then
	    UPDATE=true
	fi
done

if [[ $UPDATE == false ]]; then
    echo "Headers are up-to-date."
    exit
fi

# Prepare GLSL source to be included in C array
function processSrc {
	# 1st part: remove /* */ comments
	# 2nd part: remove // comments
	# 3rd part: remove blank lines (or containing only whitespaces)
	# 4th & 5th part: wrap every line in quotation marks
	sed '/\/\*/,/\*\//d; s/[ \t]*\/\/.*$//; /^[ \t]*$/d; s/^[ \t]*/"/; s/[ \t]*$/\\n"/' $1 >> $OUTPUT
	echo "," >> $OUTPUT
}

# Header
echo "#ifndef SHADER_SRC_H
#define SHADER_SRC_H

const unsigned int shaders_number = $SHADERS_NUMBER;
const char *shaders_src[] = {" > $OUTPUT

# Main contents
for filename in "${SHADER_SRC[@]}"
do
	processSrc $filename
done

# Footer
echo "};
#endif /* SHADER_SRC_H */" >> $OUTPUT

echo "Done."
