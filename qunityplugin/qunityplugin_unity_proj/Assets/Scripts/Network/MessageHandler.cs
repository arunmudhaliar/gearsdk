using System;
using System.Collections.Generic;
using System.Text.Json;
using UnityEngine;

public class MessageHandler
{
    // Define delegate type for message handlers
    public delegate void MessageCallback(ulong len, byte[] buf, string msg);

    // Map of message handlers
    private Dictionary<ulong, MessageCallback> _handlers = new Dictionary<ulong, MessageCallback>();

    // Register a handler
    public void RegisterHandler(ulong t_crc, MessageCallback callback)
    {
        Debug.Log($"Registering handler for : {t_crc}");
        _handlers[t_crc] = callback;
    }

    // Call a handler based on messageId
    public bool HandleMessage(ulong t_crc, ulong len, byte[] buf, string msg)
    {
        if (_handlers.TryGetValue(t_crc, out var callback))
        {
            callback(len, buf, msg);
            return true;
        }
        else
        {
            Debug.Log($"No handler found for messageId: {t_crc}");
        }

        return false;
    }
}