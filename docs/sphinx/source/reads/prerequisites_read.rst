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