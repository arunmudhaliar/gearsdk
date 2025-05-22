using System;
using System.Collections.Generic;
using System.Text;
using qsdk;
using ServerPlugin;
using UnityEngine;

public abstract class Room
{
    protected IntPtr _nativeServerPtr = IntPtr.Zero;
    protected IntPtr _roomPtr = IntPtr.Zero;
    protected int _roomID = -1;
    protected GameObject _rootNode = null;
    protected RpcMethodRegistry _methodRegistry = new RpcMethodRegistry();
    protected RPCNetworkHandler _networkHandler = new RPCNetworkHandler();
    protected NetworkSpawnManager _networkSpawnManager = new NetworkSpawnManager();
    protected Dictionary<string, (ulong hash, QNetMetaInstance metaInstance)> _metaMap =
        new Dictionary<string, (ulong hash, QNetMetaInstance metaInstance)>();
    private Room(){}

    public int RoomID
    {
        get { return _roomID; }
    }

    public RpcMethodRegistry MethodRegistry => _methodRegistry;
    public RPCNetworkHandler NetworkHandler => _networkHandler;
    public NetworkSpawnManager SpawnManager => _networkSpawnManager;
    
    public Room(int room, IntPtr nativeServerPtr, IntPtr roomPtr, GameObject rootNode)
    {
        _roomID = room;
        _nativeServerPtr = nativeServerPtr;
        _roomPtr = roomPtr;
        _rootNode = rootNode;
        NetworkHandler.RegisterSendCallback(RemoteSendData);
        NetworkHandler.RegisterBroadCastCallback(RemoteBroadCastData);
        NetworkHandler.RegisterSpawnMessageCallback(RemoteSpawnMessageData);
    }

    protected abstract void RemoteSendData(QNetMetaInstance qnetMeta, byte[] data);
    protected abstract void RemoteBroadCastData(QNetMetaInstance qnetMeta, byte[] data);
    protected abstract void RemoteSpawnMessageData(QNetMetaInstance qnetMeta, QNetObject netObject, byte[] data);

    public QNetMetaInstance GetQNetMetaInstance(string pid)
    {
        if (_metaMap.TryGetValue(pid, out var playerInfo))
        {
            return playerInfo.metaInstance;
        }
        return null;
    }
    public QNetMetaInstance AddOrUpdatePlayerMeta(string pid, ulong hashValue)
    {
        if (_metaMap.TryGetValue(pid, out var playerInfo))
        {
            playerInfo.metaInstance.clientMeta.hash = hashValue;
            _metaMap[pid] = (hash: hashValue, metaInstance: playerInfo.metaInstance);
            qnetbehaviour.LogInfo($"Room.AddOrUpdatePlayer updated pid {pid} with hash {hashValue} !!!");
            return playerInfo.metaInstance;
        }
        else
        {
            // add QNetMetaInstance component
            // GameObject methodRegistryInstance = new GameObject("QNetMetaInstance");
            QNetMetaInstance metaInstanceValue = _rootNode.AddComponent<QNetMetaInstance>();
            metaInstanceValue.serverMeta.nativeServer = _nativeServerPtr;
            metaInstanceValue.serverMeta.roomPtr = _roomPtr;
            metaInstanceValue.room = _roomID;
            metaInstanceValue.methodRegistry = MethodRegistry;
            metaInstanceValue.networkHandler = NetworkHandler;
            metaInstanceValue.networkSpawnManager = SpawnManager;
            metaInstanceValue.clientMeta.networkClientID = pid;
            metaInstanceValue.clientMeta.hash = hashValue;
            // methodRegistryInstance.transform.SetParent(newRoot.transform);
            
            _metaMap[pid] = (hash: hashValue, metaInstance: metaInstanceValue);
            qnetbehaviour.LogInfo($"Room.AddOrUpdatePlayer added pid {pid} with hash {hashValue} !!!");
            return metaInstanceValue;
        }
    }

    public void ReleaseAllMetaInstances()
    {
        foreach (var m in _metaMap)
        {
            UnityEngine.Object.DestroyImmediate(m.Value.metaInstance);
        }
        qnetbehaviour.LogInfo("Releasing old qmeta instances.");
    }
}

public class ServerRoom : Room
{
    public ServerRoom(int room, IntPtr nativeServerPtr, IntPtr roomPtr, GameObject rootNode) : base(room, nativeServerPtr, roomPtr, rootNode)
    {
    }
    
    protected override void RemoteSendData( QNetMetaInstance qnetMeta, byte[] data ) {
        bool result = ServerSdk.SendTo(_nativeServerPtr, _roomPtr, (uint)qnetMeta.clientMeta.hash, data, (uint)data.Length);
        if (result == false)
        {
            qnetbehaviour.LogError($"Error sending data to client {Encoding.UTF8.GetString(data)}");
        }
    }

    protected override void RemoteBroadCastData( QNetMetaInstance qnetMeta, byte[] data ) {
        // Debug.Log($"ServerLifeCycle BroadCastData - {Encoding.UTF8.GetString(data)}");
        ServerSdk.BroadcastToClients(_nativeServerPtr, _roomPtr, data, (uint)data.Length);
    }
    
    protected override void RemoteSpawnMessageData( QNetMetaInstance qnetMeta, QNetObject netObject, byte[] data ) {
        ServerSdk.BroadcastToClients(_nativeServerPtr, _roomPtr, data, (uint)data.Length);
    }
}

public class ClientRoom : Room
{
    public ClientRoom(int room, GameObject rootNode) : base(room, IntPtr.Zero, IntPtr.Zero, rootNode)
    {
    }
    
    protected override void RemoteSendData( QNetMetaInstance qnetMeta, byte[] data ) {
        bool result = ServerSdk.SendTo(_nativeServerPtr, _roomPtr, (uint)qnetMeta.clientMeta.hash, data, (uint)data.Length);
        if (result == false)
        {
            qnetbehaviour.LogError($"Error sending data to server {Encoding.UTF8.GetString(data)}");
        }
    }

    protected override void RemoteBroadCastData( QNetMetaInstance qnetMeta, byte[] data ) {
        // Debug.Log($"ServerLifeCycle BroadCastData - {Encoding.UTF8.GetString(data)}");
        ServerSdk.BroadcastToClients(_nativeServerPtr, _roomPtr, data, (uint)data.Length);
    }
    
    protected override void RemoteSpawnMessageData( QNetMetaInstance qnetMeta, QNetObject netObject, byte[] data ) {
        ServerSdk.BroadcastToClients(_nativeServerPtr, _roomPtr, data, (uint)data.Length);
    }
}