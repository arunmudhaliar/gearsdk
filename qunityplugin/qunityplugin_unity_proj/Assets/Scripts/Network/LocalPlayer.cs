using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LocalPlayer : qsdk.qnetbehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        //  Invoke_RegisterRPCMethods();
    }

    [qsdk.ServerRPC(RequireOwnership = false)]
    public void ProcessPlayerAction( string playerName, QH3ClientExampleTestData actionCode ) {
        Debug.Log($"Server2 received action {actionCode} from player {playerName}");
        this.TestArch("ffddf");
    }
    
    [qsdk.ClientRPC]
    public void ShowMessage( string playerName1, QH3ClientExampleTestData playerName2, string message ) {
        Debug.Log($"Client received message: {playerName1}, {playerName2}, {message}");
    }

    [qsdk.BroadcastRPC]
    public void BroadcastMessage( string message ) {
        Debug.Log($"Broadcast received message: {message}");
    }

    void TestArch(string msg)
    {
        Debug.Log(msg);
    }
}
