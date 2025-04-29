using System;
using System.Collections.Generic;
using System.Text.Json;
using System.Text;
using GeneratedProxies;

namespace qsdk {
    public class RPCNetworkHandler {
        private static readonly RPCNetworkHandler _instance = new RPCNetworkHandler();
        public static RPCNetworkHandler Instance => _instance;

        public Action<QNetMetaInstance, byte[]> SendData;
        public Action<QNetMetaInstance, byte[]> BroadCastData;
        public Action<QNetMetaInstance, QNetObject, byte[]> SpawnMessageData;
        public bool IsServer = false;  // This has to be set before any network call.

        public void RegisterSendCallback(Action<QNetMetaInstance, byte[]> sendCallback) {
            SendData = sendCallback;
        }
        public void RegisterBroadCastCallback(Action<QNetMetaInstance, byte[]> broadCastCallback) {
            BroadCastData = broadCastCallback;
        }
        public void RegisterSpawnMessageCallback(Action<QNetMetaInstance, QNetObject, byte[]> spawnMessageCallback) {
            SpawnMessageData = spawnMessageCallback;
        }

        public void SendRPC(QNetMetaInstance qnetMeta, ulong networkObjectId, RpcMethodId methodId, object[] parameters) {
            if (qnetMeta == null) {
                 qnetbehaviour.LogWarn("qnetMeta is null, ignoring SendRPC !!!");
                 return;
            }
            var rpcMessage = new RpcMessage {
                NetworkObjectId = networkObjectId,
                NetworkClientID = qnetMeta.clientMeta.networkClientID,
                MethodId = methodId,
                Parameters = parameters
            };

            byte[] serializedData = Serialize(rpcMessage);
            SendData?.Invoke(qnetMeta, serializedData);
        }
        public void BroadcastRPC(QNetMetaInstance qnetMeta, ulong networkObjectId, RpcMethodId methodId, object[] parameters) {
            if (qnetMeta == null) {
                 qnetbehaviour.LogWarn("qnetMeta is null, ignoring BroadcastRPC !!!");
                 return;
            }
            var rpcMessage = new RpcMessage {
                NetworkObjectId = networkObjectId,
                NetworkClientID = qnetMeta.clientMeta.networkClientID,
                MethodId = methodId,
                Parameters = parameters
            };

            byte[] serializedData = Serialize(rpcMessage);
            BroadCastData?.Invoke(qnetMeta, serializedData);
        }

        public void SendSpawnMessageToClient(QNetMetaInstance qnetMeta, QNetObject netObject, bool spawn) {
            if (qnetMeta == null) {
                 qnetbehaviour.LogWarn("qnetMeta is null, ignoring SendSpawnMessageToClient !!!");
                 return;
            }
            if (netObject == null) {
                 qnetbehaviour.LogWarn("netObject is null, ignoring SendSpawnMessageToClient !!!");
                 return;
            }
            var rpcSpawnMessage = new RpcSpawnMessage {
                Flag = (byte)(spawn ? 0 : 1),
                Hash = netObject.PrefabHash,
                NetworkObjectId = netObject.NetworkObjectId,
                NetworkClientID = qnetMeta.clientMeta.networkClientID,
            };
            string json = JsonSerializer.Serialize(rpcSpawnMessage);
            byte[] serializedData = Encoding.UTF8.GetBytes(json);
            SpawnMessageData?.Invoke(qnetMeta, netObject, serializedData);
        }

        public void ReceiveData(byte[] data) {
            RpcMessage rpcMessage = Deserialize(data);
            RpcMethodRegistry.Instance.InvokeMethod(rpcMessage.NetworkObjectId, rpcMessage.MethodId, rpcMessage.Parameters);
        }

        private byte[] Serialize(RpcMessage message) {
            string json = JsonSerializer.Serialize(message);
            return Encoding.UTF8.GetBytes(json);
        }

        private RpcMessage Deserialize(byte[] data) {
            string json = Encoding.UTF8.GetString(data);
            return JsonSerializer.Deserialize<RpcMessage>(json);
        }
    }

    public class RpcMessage {
        public ulong NetworkObjectId { get; set; }
        public string NetworkClientID { get; set; }
        public RpcMethodId MethodId { get; set; }
        public object[] Parameters { get; set; }
    }
    public class RpcSpawnMessage {
        public byte Flag { get; set; } = 0;
        public uint Hash { get; set; }
        public ulong NetworkObjectId { get; set; }
        public string NetworkClientID { get; set; }
    }
}
