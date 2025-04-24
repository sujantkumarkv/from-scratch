#!/bin/sh
#
# Use this script to run your program LOCALLY.
#
# Note: Changing this script WILL NOT affect how CodeCrafters runs your program.
#
# Learn more: https://codecrafters.io/program-interface

set -e # Exit early if any commands fail

# The following block of code is responsible for compiling and running the program locally.
# It is adapted from the .codecrafters/compile.sh and .codecrafters/run.sh scripts.
# 
# - If you want to change how the program compiles locally, modify this section.
# - To change how it compiles remotely, you should edit the .codecrafters/compile.sh file.

(
  # Change the current directory to the directory where this script is located.
  # This ensures that all subsequent commands are executed within the context of the repository directory.
  cd "$(dirname "$0")"

  # Use the gcc compiler to compile all C source files located in the 'app' directory.
  # The '-lcurl' and '-lz' options link the compiled program with the libcurl and zlib libraries, respectively.
  # The '-o' option specifies the output file, which in this case is '/tmp/http-c'.
  # This means the compiled program will be saved in the '/tmp' directory with the name 'http-c'.
  gcc -lcurl -lz -o /tmp/http-c app/*.c
)

# The following command is responsible for executing the compiled program.
# 
# - If you want to change how the program runs locally, modify this section.
# - To change how it runs remotely, you should edit the .codecrafters/run.sh file.

# The 'exec' command replaces the current shell process with the specified program.
# Here, it runs the compiled program located at '/tmp/http-c'.
# The "$@" passes any additional arguments provided to this script to the program.
exec /tmp/http-c "$@"
