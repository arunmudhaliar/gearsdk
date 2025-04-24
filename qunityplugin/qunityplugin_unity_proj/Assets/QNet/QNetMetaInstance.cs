using System;
using System.Collections;
using System.Collections.Generic;
using qsdk;
using UnityEngine;

public class QNetMetaInstance : MonoBehaviour
{
    public class ServerMeta
    {
        public IntPtr nativeServer;
        public IntPtr roomPtr;
        private ulong networkObjectIDCounter;
        public ServerMeta()
        {
            nativeServer = default;
            roomPtr = default;
            networkObjectIDCounter = 0;
        }

        public ulong GenerteNetworkObjectId()
        {
            return ++networkObjectIDCounter;
        }
    }

    public class ClientMeta
    {
    }
    
    public int room;
    public RpcMethodRegistry methodRegistry = new RpcMethodRegistry();
    public RPCNetworkHandler networkHandler = new RPCNetworkHandler();
    public ServerMeta serverMeta = new ServerMeta();
    public ClientMeta clientMeta = new ClientMeta();
}
