qhiredis
================

This section provides documentation for the various components of the `gearsdk` project. Below, you will find detailed information about various header files used in the project.

header files
------------

The following header files are documented here:

1. `qhiredis.hpp`
2. `qhiredis_async.hpp`

qhiredis
--------

The `qhiredis.hpp` file contains the definition and implementation of the `qhiredis` class, which provides common functionality for the server side of the project.

.. doxygenfile:: qhiredis.hpp
   :project: gearsdk

qhiredis_async
--------------

The `qhiredis_async.hpp` file provides the `qhiredis_async` class, which manages the connection and interaction with a Redis database.

.. doxygenfile:: qhiredis_async.hpp
   :project: gearsdk
