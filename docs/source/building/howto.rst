How-To Build OSRM from Source
=============================

This How-To explans how to build OSRM from source. It assumes familiarity with `git`,
`cmake`, and a C++ compiler.  On Windows it assumes a Linux-compatible toolset like
MSYS2 is installed.

.. mermaid::

    %%{init: { 'flowchart': { 'useMaxWidth': false } } }%%

    flowchart LR
        clone[Prepare]
        choose{Choose PM}
        conan[Build with Conan PM]
        apt[Build with system PM]
        test[Run tests]
        install[Install]

        clone-->choose
        choose-->conan-->test;
        choose-->apt-->test;
        test-->install

        click clone "#clone"
        click choose "#choose"
        click conan "#conan"
        click apt "#apt"
        click test "#tests"
        click install "#install"

.. _clone:

Prepare
-------

Clone the Github repository and install Node files:

.. code:: bash

    git clone https://github.com/Project-OSRM/osrm-backend.git
    cd osrm-backend
    npm ci --ignore-scripts

.. _choose:

Choose a Package Manager
------------------------

Now you must choose which package manager to use to install dependencies:

- a C++ package manager like Conan or,
- a system package manager like `apt-get` or `brew`.

On Windows only the Conan package manager is supported. [#]_

The build method with Conan is tested on Ubuntu-24.04, Ubuntu-22.04, macOS-26, macOS-15,
macOS-14, Windows-Server-2025, and Windows-Server-2022.  Conan downloads the sources of
the dependencies and compiles them on your machine.  More things can go wrong, it takes
longer, but you always get the newest versions.

The build method with `apt-get` or `brew` is tested on Ubuntu-24.04, Ubuntu-22.04, and
macOS-15.  It uses the package manager that comes with the OS.  While using a system
package manager has its advantages, there are also some drawbacks to consider.  These
include lack of control over the installation process, and the use of outdated versions,
which may be several years old.

:octicon:`alert;2em;sd-text-warning` When you switch building methods you must close and
reopen the shell.

.. rubric:: Footnotes

.. [#] That doesn't mean there is no other way, just that we didn't have the time and
       hardware to figure it out for you.  If you know how to, please submit a patch.

.. _conan:

Build with Conan
----------------

First install Conan. You have to do this only once after a git clone.

.. code:: bash

    scripts/install_conan.sh

Then activate Conan. You have to activate Conan only once for every shell you open.

.. code:: bash

    source scripts/activate_venv

Then build it:

.. code:: bash

    conan build -pr home --build=missing

The OSRM binaries are now in `build/Release`.

To build the node package (optional):

.. code:: bash

    conan build -pr home --build=missing -o node_package=True

The node binaries are now in `build/nodejs/lib/binding_napi_v8`.

Proceed with `testing <tests>`.

.. _apt:

Build without Conan
-------------------

This build method installs the dependencies with the system package manager and then
builds with `cmake`.  This build method is not supported on Windows.

Install dependencies:

.. tab-set::

    .. tab-item:: Linux

        .. code:: bash

            sudo apt-get install -y libbz2-dev libxml2-dev libzip-dev liblua5.2-dev libtbb-dev libboost-all-dev

    .. tab-item:: macOS

        .. code:: bash

            brew install bzip2 libzip lua tbb boost


Then build with:

.. code:: bash

    cmake -B build/Release
    cmake --build build/Release -j

The OSRM binaries are now in `build/Release`.

Optional: To build the node package, type this instead of the above 2 lines:

.. code:: bash

    cmake -B build/Release -DBUILD_NODE_PACKAGE=ON
    cmake --build build/Release -j
    scripts/ci/build_node_package.sh


The OSRM binaries are now in `build/Release` and the node binaries are in
`build/nodejs/lib/binding_napi_v8`.


.. _tests:

Test
----

.. _unit-tests:

Unit tests
~~~~~~~~~~

To run the unit tests:

.. code:: bash

    cmake --build build/Release -j --target tests
    source build/osrm-run-env.sh
    ctest --test-dir build/Release -C Release -L tests -j

.. _cucumber:

Cucumber tests
~~~~~~~~~~~~~~

To run the Cucumber tests:

.. code:: bash

    npm test -- --parallel 16


Install
-------

Install OSRM into custom directories:

.. code:: bash

    cmake --install build/Release --config Release --prefix ~/my/install/dir


Or, you have to `sudo` to install into system directories:

.. code:: bash

    sudo cmake --install build/Release --config Release
