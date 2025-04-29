using UnityEngine;
using System.Collections.Generic;

namespace qsdk
{
    // todo: add comment.
    // and doxygen
    //
    public class NetworkSpawnManager
    {
        private readonly Dictionary<string, Dictionary<ulong, QNetObject>> _registeredObjectMap = new Dictionary<string, Dictionary<ulong, QNetObject>>();
        private readonly Dictionary<ulong, QNetObject> _registeredObjects = new Dictionary<ulong, QNetObject>();
        private ulong _networkSpawnID = 0;
        public ulong NextNetworkSpawnID => _networkSpawnID+1;

        public bool RegisterSpawnObject(QNetMetaInstance metaInstance, QNetObject obj)
        {
            if (obj == null || metaInstance == null)
            {
                Debug.LogWarning("NetSpawnManager: Tried to register with null parameters.");
                return false;
            }

            var networkClientID = metaInstance.clientMeta.networkClientID;
            if (!_registeredObjectMap.TryGetValue(networkClientID, out var netObjects))
            {
                netObjects = new Dictionary<ulong, QNetObject>();
                _registeredObjectMap[networkClientID] = netObjects;
                // Debug.Log($"networkClientID {networkClientID} map created.");
            }

            if (!netObjects.TryAdd(obj.NetworkObjectId, obj))
            {
                Debug.LogWarning($"NetSpawnManager: NetworkObjectID {obj.NetworkObjectId} is already registered for {networkClientID}.");
                return false;
            }
            
            _registeredObjects[obj.NetworkObjectId] = obj;
            _networkSpawnID++;
            return true;
        }

        public bool UnregisterSpawnObject(QNetMetaInstance metaInstance, QNetObject obj)
        {
            if (obj == null || metaInstance == null)
            {
                Debug.LogWarning("NetSpawnManager: Tried to unregister with null parameters.");
                return false;
            }

            var networkClientID = metaInstance.clientMeta.networkClientID;
            if (!_registeredObjectMap.TryGetValue(networkClientID, out var netObjects))
            {
                Debug.LogWarning($"NetSpawnManager: networkClientID {networkClientID} not found in map.");
                return false;
            }

            if (!netObjects.Remove(obj.NetworkObjectId))
            {
                if (_registeredObjects.Remove(obj.NetworkObjectId))
                {
                    Debug.LogWarning($"NetSpawnManager: Mismatch found. {obj.NetworkObjectId} was not found in map but on list.");    
                }
                Debug.LogWarning($"NetSpawnManager: NetworkObjectID {obj.NetworkObjectId} was not found.");
                return false;
            }

            _registeredObjects.Remove(obj.NetworkObjectId);
            return true;
        }

        public QNetObject GetObject(ulong networkObjectId)
        {
            _registeredObjects.TryGetValue(networkObjectId, out var instance);
            return instance;
        }
    }
}
