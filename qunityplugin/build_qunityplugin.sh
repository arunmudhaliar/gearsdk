#!/bin/bash

# Build the release version using your Makefile
make release

# Determine the operating system
UNAME=$(uname)
if [ "$UNAME" == "Linux" ]; then
    # Linux-specific actions if needed
    echo "Running on Linux"
    chmod 755 libqunityplugin.so
elif [ "$UNAME" == "Darwin" ]; then
    # macOS-specific actions
    echo "Running on macOS"
    
    # Update library paths using install_name_tool for libev and libquiche
    install_name_tool -change /Users/amudaliar/Desktop/ev_mac_out/lib/libev.4.dylib @rpath/libev.4.dylib libqunityplugin.dylib
    install_name_tool -change /Users/amudaliar/Documents/PROJECTS/PERSONAL/quiche/target/release/deps/libquiche.dylib @rpath/libquiche.dylib libqunityplugin.dylib

    # Verify the updated library paths with otool
    otool -L libqunityplugin.dylib
else
    echo "Unsupported operating system: $UNAME"
    exit 1
fi
