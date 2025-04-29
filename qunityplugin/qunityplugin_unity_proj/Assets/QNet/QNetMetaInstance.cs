using System;
using System.Collections;
using System.Collections.Generic;
using qsdk;
using UnityEngine;
using UnityEngine.Serialization;

public class QNetMetaInstance : MonoBehaviour
{
    [System.Serializable]
    public class ServerMeta
    {
        public IntPtr nativeServer;
        public IntPtr roomPtr;
        public ServerMeta()
        {
            nativeServer = default;
            roomPtr = default;
        }
    }

    [System.Serializable]
    public class ClientMeta
    {
        public string networkClientID;
        public ulong hash;
    }
    
    public int room;
    public RpcMethodRegistry methodRegistry = null;
    public RPCNetworkHandler networkHandler = null;
    public NetworkSpawnManager networkSpawnManager = null;
    public ServerMeta serverMeta = new ServerMeta();
    public ClientMeta clientMeta = new ClientMeta();
}
