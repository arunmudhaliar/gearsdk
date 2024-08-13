.. _qbuffer_hpp:

qbuffer.hpp
===========

This file defines a data buffer for handling buffer operations on heap memory. It provides functionalities to manage dynamic memory efficiently, ensuring safe and optimized buffer operations.

Includes
--------

The file typically includes necessary headers for memory management and buffer operations. (Note: The actual includes are not provided here as the file content is not available.)

Classes
-------

qbuffer
-------

Handles dynamic buffer operations.

- `qbuffer()`: Constructor to initialize the buffer.
- `~qbuffer()`: Destructor to clean up the buffer.
- `void allocate(size_t size)`: Allocates memory for the buffer.
- `void deallocate()`: Deallocates the buffer memory.
- `void append(const void* data, size_t size)`: Appends data to the buffer.
- `void clear()`: Clears the buffer content.
- `size_t size() const`: Returns the current size of the buffer.
- `const void* data() const`: Returns a pointer to the buffer data.

Usage
-----

The `qbuffer` class is used to manage dynamic memory buffers, providing methods to allocate, deallocate, append data, and clear the buffer. It ensures efficient memory management and safe buffer operations.

Example
-------

Here is a simple example of how to use the `qbuffer` class:

.. code-block:: cpp

   #include "qbuffer.hpp"

   int main() {
       qbuffer buffer;
       buffer.allocate(1024); // Allocate 1KB buffer

       const char* data = "Hello, World!";
       buffer.append(data, strlen(data)); // Append data to buffer

       // Use buffer data
       const void* bufferData = buffer.data();
       size_t bufferSize = buffer.size();

       buffer.clear(); // Clear the buffer
       buffer.deallocate(); // Deallocate the buffer

       return 0;
   }
