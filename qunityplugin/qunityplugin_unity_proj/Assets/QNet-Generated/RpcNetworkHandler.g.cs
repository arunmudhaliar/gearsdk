using System;
using System.Collections.Generic;
using System.Text.Json;
using System.Text;
using GeneratedProxies;

namespace qsdk {
    public class RPCNetworkHandler {
        private static readonly RPCNetworkHandler _instance = new RPCNetworkHandler();
        public static RPCNetworkHandler Instance => _instance;

        public Action<byte[]> SendData;
        public Action<byte[]> BroadCastData;
        public bool IsServer = false;  // This has to be set before any network call.

        public void RegisterSendCallback(Action<byte[]> sendCallback) {
            SendData = sendCallback;
        }
        public void RegisterBroadCastCallback(Action<byte[]> broadCastCallback) {
            BroadCastData = broadCastCallback;
        }

        public void SendRPC(ulong networkObjectId, RpcMethodId methodId, object[] parameters) {
            var rpcMessage = new RpcMessage {
                NetworkObjectId = networkObjectId,
                MethodId = methodId,
                Parameters = parameters
            };

            byte[] serializedData = Serialize(rpcMessage);
            SendData?.Invoke(serializedData);
        }
        public void BroadcastRPC(ulong networkObjectId, RpcMethodId methodId, object[] parameters) {
            var rpcMessage = new RpcMessage {
                NetworkObjectId = networkObjectId,
                MethodId = methodId,
                Parameters = parameters
            };

            byte[] serializedData = Serialize(rpcMessage);
            BroadCastData?.Invoke(serializedData);
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
        public RpcMethodId MethodId { get; set; }
        public object[] Parameters { get; set; }
    }
}
