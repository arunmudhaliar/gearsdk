networkcommon
=============

**Brief about each files**

``essentials.hpp``: Contains essential utility functions used across the project.
   .. note::

      For detailed information on essential utilities and classes used in the project, refer to the `essentials.hpp`_ documentation.

      .. _essentials.hpp: ../reads/essentials_read.html

``qbuffer.hpp*``: A data buffer for handling buffer on heap memory.
   .. note::

      For detailed information on qbuffer utilities and classes used in the project, refer to the `qbuffer.hpp`_ documentation.

      .. _qbuffer.hpp: ../reads/qbuffer_read.html

``qstatslogger.hpp*``: For logging statistical data.
   .. note::

      This documentation provides a detailed explanation of the `qstatslogger.hpp`_ class, including its header and source files, SQL table creation statements, and an example usage.

      .. _qstatslogger.hpp: ../reads/qstatslogger_read.html

``qtextfilelogger.hpp``: For logging text data to files.

``message.hpp``: Defines general message structures or classes used in the network communication.

``roommessage.hpp``: Contains definitions for handling messages related to game rooms.

   .. note::
      **To get a glimpse of important functions**

      **message_room_base**

      - **Inheritance**: Inherits from `message_base`.
      - **Purpose**: Serves as a base class for all room-related messages.

      **Key Members**

      - **signature**: A unique identifier for the message type.
      - **cached_type_string_crc**: Cached CRC value for the message type string.

      **Key Methods**

      - **serialize**: Serializes the message to a JSON object.
      - **deserialize**: Deserializes the message from a JSON object.
      - **deserialize_header**: Static method to deserialize the header of a message.

      **msg_room_match_request**

      - **Inheritance**: Inherits from `message_room_base`.
      - **Purpose**: Represents a request to match a room.

      **Key Members**

      - **room_config**: Configuration for the room.
      - **prev_cid_hash_val**: Used for reconnection.
      - **room_id**: Used for reconnection.
      - **pid**: Player ID.

      **Key Methods**

      - **serialize**: Serializes the message to a JSON object.
      - **deserialize**: Deserializes the message from a JSON object.

.. doxygenfile:: essentials.hpp
   :project: gearsdk