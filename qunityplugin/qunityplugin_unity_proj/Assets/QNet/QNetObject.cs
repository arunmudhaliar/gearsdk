using System;
using System.Collections;
using System.Collections.Generic;
using ServerPlugin;
using UnityEngine;

namespace qsdk {
public class QNetObject : MonoBehaviour
{
    // variables
    private bool _isServer = RPCNetworkHandler.Instance.IsServer;
    private bool _isOwner = false;
    private ulong _networkObjectID = 0;

    public string _prefabId = "default";
    public uint _prefabHash => ComputeHash(_prefabId);

    public bool IsSpawned { get; private set; }
    
    // properties
    public ulong NetworkObjectId
    {
        get
        {
            return _networkObjectID;
        }
        private set
        {
            _networkObjectID = value;
        }
    }
    
    public bool IsServer
    {
        get
        {
            return _isServer;
        }
        private set
        {
            _isServer = value;
        }
    }

    public bool IsOwner
    {
        get
        {
            return _isOwner;
        }
        private set
        {
            _isOwner = value;
        }
    }
    
    // functions
    private static uint ComputeHash(string id)
    {
        return (uint)id.GetHashCode();
    }

    public void Spawn(QNetMetaInstance meta)
    {
        if (!_isServer)
        {
            qnetbehaviour.LogWarn("Attempting to spawn a network object on a non-server instance.");
            return;
        }

        if (IsSpawned)
        {
            qnetbehaviour.LogWarn("Object is already spawned.");
            return;
        }
        
        // Assign a unique NetworkObjectId to room
        _networkObjectID = meta.serverMeta.GenerteNetworkObjectId();

        // Register this object with the spawn manager
        // NetworkManager.Singleton.SpawnManager.RegisterSpawnedObject(this);

        // Send spawn message to all clients
        // NetworkManager.Singleton.MessagingSystem.SendSpawnMessageToClients(this);

        IsSpawned = true;
        IsOwner = true; // by default server is owner for all spawned objects

        // NativeMethods.room_broadcast();

        // Server also calls its own OnNetworkSpawn
        foreach (var behaviour in GetComponents<qnetbehaviour>())
        {
            behaviour.SetNetworkObjectId(_networkObjectID);
            behaviour.Invoke_RegisterRPCMethods();
            bool previousOwnership = behaviour.CachedOwnership;
            behaviour.SetOwnership(_isOwner);
            behaviour.OnNetworkOwnershipUpdate(previousOwnership);
            behaviour.OnNetworkSpawn(meta);
        }
    }
}
}