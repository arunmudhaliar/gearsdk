using System.Collections;
using System.Collections.Generic;
using GeneratedProxies;
using qsdk;
using UnityEngine;

public class Player : qsdk.qnetbehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        // //  Invoke_RegisterRPCMethods();
        // RPCNetworkHandler instance = RPCNetworkHandler.Instance;
        // instance.SendRPC (CachedMetaInstance, CachedNetworkObjectId, RpcMethodId.LocalPlayer_ShowMessage, null);

    }

    [qsdk.ServerRPC(RequireOwnership = false)]
    public void TestProcessPlayerAction( string playerName, QH3ClientExampleTestData actionCode ) {
        Debug.Log($"Server2 received action {actionCode} from player {playerName}");
    }
    
    [qsdk.ClientRPC]
    public void TestShowMessage( string playerName1, QH3ClientExampleTestData playerName2, string message ) {
        Debug.Log($"Client received message: {playerName1}, {playerName2}, {message}");
    }

    [qsdk.BroadcastRPC]
    public void TestBroadcastMessage( string message ) {
        Debug.Log($"Broadcast received message: {message}");
    }

    public override void OnNetworkSpawn(QNetMetaInstance meta)
    {
        Debug.Log($"Player spawned message: {meta.clientMeta.networkClientID}");
    }
}
