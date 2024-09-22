.. _prerequisites_read:

Pre-requisites
==============

.. note::
    **Pre-requisites to setup dev environment using VSCode in mac-os**:

    **Xcode Command Line Tool**

    .. code-block:: shell
        
        xcode-select --install

    **Homebrew**

    .. code-block:: shell

        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    **Install cmake**

    .. code-block:: shell

        brew install cmake

    **Install llvm**

    .. code-block:: shell

        brew install llvm

    Make sure to add export variables to `~/.zshrc` (Path may need to be set based on the installation locations):

    .. code-block:: shell

        export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
        export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"

    **Install lldb-mi**

    `https://github.com/lldb-tools/lldb-mi <https://github.com/lldb-tools/lldb-mi>`_
    
    Download the lldb-mi from the above git link and follow the shell commands.

    .. code-block:: shell

        cmake -DLLVM_DIR=/opt/homebrew/Cellar/llvm/[version]/lib/cmake/llvm .
        cmake --build .
        sudo cmake --install .  # (sudo may be necessary for installation)
        # This will install `lldb-mi` to `/usr/local/bin/lldb-mi`.

    **Google Test**

    For unit, integration and functional testing, we use Google Test. To install Google Test, follow the below steps:

    .. code-block:: shell

        brew install googletest
        brew install pkg-config # Required for make to find gtest


    **Build libuv**

    For building libuv from source, follow the below steps:

    .. code-block:: shell

        # Clone the libuv repository
        git clone https://github.com/libuv/libuv.git
        cd libuv

        # Set up the environment variables for the toolchain
        export NDK_PATH=<ndk path>
        export TOOLCHAIN=$NDK_PATH/build/cmake/android.toolchain.cmake

        # Build the library for different architectures
        cmake .. -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-24
        cmake --build .

        # Clean the build directory and follow the below steps for different architectures followed by 'cmake --build .' command'
        cmake .. -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24
        cmake .. -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DANDROID_ABI=x86 -DANDROID_PLATFORM=android-24
        cmake .. -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DANDROID_ABI=x86_64 -DANDROID_PLATFORM=android-24

.. note::
    **Pre-requisites to setup dev environment using VSCode in linux**:
        For building libuv from source, follow the below steps:

    .. code-block:: shell

        # May need administrative privillage, if so use 'sudo'
        # Ex: sudo apt-get update && apt-get install make
        apt-get update && apt-get install make
        apt-get install build-essential -y
        apt-get install libssl-dev -y
        apt-get install libsasl2-dev -y
        apt install libsnappy-dev -y
        apt-get install libzstd-dev -y
        apt install iproute2 -y
        apt-get install libcurl4-openssl-dev -y

    **Install lldb-mi**

    `https://github.com/lldb-tools/lldb-mi <https://github.com/lldb-tools/lldb-mi>`_
    
    Download the lldb-mi from the above git link and follow the instructions to build. VScode requires 'lldb-mi' for debugging.