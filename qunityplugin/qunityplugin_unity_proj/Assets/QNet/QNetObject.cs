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
    private ulong _networkObjectID = 0;

    public string _prefabId = "default";
    public uint _prefabHash => ComputeHash(_prefabId);
    private string _networkClientID;
    public QNetMetaInstance _qnetMeta;
    public bool IsSpawned { get; private set; }

    
    // properties
    public uint PrefabHash => _prefabHash;
    
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
            if (string.IsNullOrEmpty(_networkClientID))
            {
                Debug.LogError($"IsOwner is not valid, since _cachedNetworkClientID is {_networkClientID}");
                return false;
            }
            if (_isServer)
            {
                return true;
            }
            return _qnetMeta.clientMeta.networkClientID == _networkClientID;
        }
    }
    
    // functions
    private static uint ComputeHash(string id)
    {
        return (uint)id.GetHashCode();
    }

    public bool Spawn(QNetMetaInstance meta)
    {
        _qnetMeta = meta;
        if (!_isServer)
        {
            qnetbehaviour.LogWarn("Attempting to spawn a network object on a non-server instance.");
            return false;
        }

        if (IsSpawned)
        {
            qnetbehaviour.LogWarn("Object is already spawned.");
            return false;
        }
        
        // Assign a unique NetworkObjectId to room
        _networkObjectID = meta.networkSpawnManager.NextNetworkSpawnID;
        string previousNetworkClientID = _networkClientID;
        _networkClientID = meta.clientMeta.networkClientID;
        
        // Register this object with the spawn manager
        if (!meta.networkSpawnManager.RegisterSpawnObject(meta, this))
        {
            qnetbehaviour.LogWarn($"RegisterSpawnObject failed for client {_networkClientID}, prefab:{_prefabId}, Name {name}, networkObjectID{_networkObjectID}.");
            return false;
        }
        
        IsSpawned = true;
        
        // Send spawn message to all clients
        meta.networkHandler.SendSpawnMessageToClient(_qnetMeta, this, true);
        
        // assign vars
        var qnetbehaviours = GetComponentsInChildren<qnetbehaviour>(true);
        foreach (var behaviour in qnetbehaviours)
        {
            behaviour.SetNetworkObjectId(_networkObjectID);
            behaviour.SetOwnership(_networkClientID);
            behaviour.CachedMetaInstance = meta;
        }
        
        // Server also calls its own OnNetworkSpawn
        foreach (var behaviour in GetComponents<qnetbehaviour>())
        {
            behaviour.Invoke_RegisterRPCMethods();
            behaviour.OnNetworkOwnershipUpdate(previousNetworkClientID);
            behaviour.OnNetworkSpawn(meta);
        }
        return true;
    }

    public bool Despawn()
    {
        if (!_isServer)
        {
            qnetbehaviour.LogWarn("Attempting to despawn a network object on a non-server instance.");
            return false;
        }

        if (!IsSpawned)
        {
            qnetbehaviour.LogWarn("Object not spawned.");
            return false;
        }

        if (!_qnetMeta.networkSpawnManager.UnregisterSpawnObject(_qnetMeta, this))
        {
            qnetbehaviour.LogWarn($"UnregisterSpawnObject failed for client {_networkClientID}, prefab:{_prefabId}, Name {name}, networkObjectID{_networkObjectID}.");
            return false;
        }
        
        IsSpawned = false;
        
        // Send spawn message to all clients
        _qnetMeta.networkHandler.SendSpawnMessageToClient(_qnetMeta, this, false);
        
        // Server also calls its own OnNetworkSpawn
        foreach (var behaviour in GetComponents<qnetbehaviour>())
        {
            behaviour.OnNetworkDeSpawn();
        }
        return true;
    }
}
}