.. _qstatslogger_hpp:

qstatslogger.hpp and qstatslogger.cpp
=====================================

The `qstatslogger` class is responsible for logging statistical data to a text file. It extends the `qtextfilelogger` class and includes methods for initializing the logger, logging various types of statistics, and managing client and server session information.

**Header File: `qstatslogger.hpp`**

The header file defines the class and its interface. Key methods include:

- **`init(const qstring& install_os, const qstring& device_name, const qstring& device_model, const int total_ram)`**: Initializes the logger with system and device information.
- **`log_stats(const qstring& buffer)`**: Logs the provided buffer content.
- **`client_open(const qstring& duid, const qstring& epic, const qstring& myth, const qstring& legend, const qstring& story)`**: Logs client session opening information.
- **`server_count(const qstring& count, long count_val, const qstring& session, const qstring& pid, const qstring& version, const qstring& epic, const qstring& myth, const qstring& legend, const qstring& story, const qstring& message)`**: Logs server-side count information.

**Source File: `qstatslogger.cpp`**

The source file implements the methods defined in the header. Key functionalities include:

- **Constructor and Destructor**
  - **`qstatslogger::qstatslogger()`**: Initializes the logger.
  - **`qstatslogger::~qstatslogger()`**: Cleans up resources.

- **Initialization**
  - **`void qstatslogger::init(const qstring& install_os_, const qstring& device_name_, const qstring& device_model_, const int total_ram_)`**: Sets up the logger with system and device details.

- **Logging Statistics**
  - **`size_t qstatslogger::log_stats(const qstring& buffer)`**: Logs data to the file, checking if the file is initialized.

- **Client and Server Logging**
  - **`size_t qstatslogger::client_open(const qstring& duid, const qstring& epic, const qstring& myth, const qstring& legend, const qstring& story)`**: Logs information when a client session opens.
  - **`size_t qstatslogger::server_count(const qstring& count, long count_val, const qstring& session, const qstring& pid, const qstring& version, const qstring& epic, const qstring& myth, const qstring& legend, const qstring& story, const qstring& message)`**: Logs server-side statistics.

**SQL Table Creation**

For reference, here are SQL table creation statements related to the `qstatslogger` class:

.. code-block:: sql

    CREATE TABLE stats_count (
        count TEXT,
        session TEXT,
        pid TEXT,
        version TEXT,
        epic TEXT,
        myth TEXT,
        legend TEXT,
        story TEXT,
        install_os TEXT,
        server_tstamp TIMESTAMP,
        client_tstamp TIMESTAMP,
        time BIGINT,
        message TEXT,
        device_name TEXT,
        device_model TEXT,
        total_ram INT4
    );

    CREATE TABLE stats_open (
        version TEXT,
        duid TEXT,
        epic TEXT,
        myth TEXT,
        legend TEXT,
        story TEXT,
        install_os TEXT,
        client_tstamp TIMESTAMP,
        time BIGINT,
        device_name TEXT,
        device_model TEXT,
        total_ram INT4
    );

**Example Usage**

Here is an example of how to use the `qstatslogger` class:

.. code-block:: cpp

    #include "qstatslogger.hpp"

    int main() {
        qstatslogger logger;
        logger.init("Windows 10", "MyPC", "ModelX", 8192); // Initialize with OS, device name, model, and RAM

        logger.set_client_session("session123");
        logger.set_client_version("1.0.0");
        logger.set_client_pid("pid123");
        
        logger.client_open("duid456", "epic_event", "mythical", "legendary", "storyline");

        logger.server_count("event_count", 5, "session123", "pid123", "1.0.0", "epic_event", "mythical", "legendary", "storyline", "Message content");

        return 0;
    }
