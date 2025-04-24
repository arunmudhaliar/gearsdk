using System;
using System.Collections.Generic;
using GeneratedProxies;
namespace qsdk {
    public class RpcMethodRegistry {
        private static readonly RpcMethodRegistry _instance = new RpcMethodRegistry();
        public static RpcMethodRegistry Instance => _instance;
        private readonly Dictionary<ulong, Dictionary<RpcMethodId, Action<object[]>>> _methodLookup = new Dictionary<ulong, Dictionary<RpcMethodId, Action<object[]>>>();
        public void RegisterMethod(ulong networkObjectId, RpcMethodId id, Action<object[]> method) {
            if (!_methodLookup.TryGetValue(networkObjectId, out var methods)) {
                methods = new Dictionary<RpcMethodId, Action<object[]>>();
                _methodLookup[networkObjectId] = methods;
            }
            methods[id] = method;
        }
        public void UnregisterMethod(ulong networkObjectId, RpcMethodId id) {
            if (_methodLookup.TryGetValue(networkObjectId, out var methods)) {
                methods.Remove(id);
                if (methods.Count == 0)
                    _methodLookup.Remove(networkObjectId);
            }
        }
        public void InvokeMethod(ulong networkObjectId, RpcMethodId id, object[] parameters) {
            if (_methodLookup.TryGetValue(networkObjectId, out var methods) && methods.TryGetValue(id, out var method)) {
                method(parameters);
            } else {
                Console.WriteLine($"Unknown RPC Method ID: {id} for NetworkObjectId: {networkObjectId}");
            }
        }
        public void GetInfo() {
            foreach (var obj in _methodLookup) {
                foreach(var d in obj.Value)
                    Console.WriteLine($"_methodLookup ObjectId: {obj.Key}, {d.Key}, {d.Value.Method.DeclaringType.FullName}::{d.Value.Method.Name}");
            }
        }
    }
}
