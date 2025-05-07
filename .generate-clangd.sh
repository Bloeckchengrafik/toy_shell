#!/usr/bin/env bash

# Generates a .clangd config file with system include paths from clang output

echo "Generating .clangd file..."

# Get system include paths from clang
INCLUDE_PATHS=$(clang -E -x c++ - -v < /dev/null 2>&1 \
    | awk '/#include <...> search starts here:/{flag=1; next} /End of search list./{flag=0} flag { print }')

# Build YAML array with -isystem
YAML_FLAGS=$(printf '    - "-isystem"\n    - "%s"\n' $INCLUDE_PATHS)

# Write the .clangd file
cat > .clangd <<EOF
CompileFlags:
  Add:
$YAML_FLAGS
EOF

echo ".clangd file created with the following system include paths:"
echo "$INCLUDE_PATHS"
