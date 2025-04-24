using System.Collections;
using System.Collections.Generic;
using System.Text;
using UnityEngine;

public class NetworkScene : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
#if UNITY_SERVER
        qsdk.RPCNetworkHandler.Instance.IsServer = true;
#else
        qsdk.RPCNetworkHandler.Instance.IsServer = false;
#endif
        qsdk.RPCNetworkHandler.Instance.RegisterSendCallback(Mock_SendData);
        qsdk.RPCNetworkHandler.Instance.RegisterBroadCastCallback(Mock_BroadCastData);
    }
    
    private static void Mock_SendData( byte[] data ) {
        Debug.Log($"Mock SendData - {Encoding.UTF8.GetString(data)}");
    }

    private static void Mock_BroadCastData( byte[] data ) {
        Debug.Log($"Mock BroadCastData - {Encoding.UTF8.GetString(data)}");
    }
}
