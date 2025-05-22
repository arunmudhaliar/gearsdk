using System.Collections;
using System.Collections.Generic;
using ServerPlugin;
using UnityEngine;
using UnityEngine.SceneManagement;

public class PreloaderScene : MonoBehaviour
{
    private const string SCENE_GSDK_MAIN = "gsdk-main-scene";
    private const string SCENE_GSDK_NETWORK_BASE = "gsdk-network-base-scene";
    private const string SCENE_GSDK_NETWORK = "gsdk-network-scene";
    
    // Start is called before the first frame update
    void Start()
    {
#if UNITY_SERVER
        qsdk.RPCNetworkHandler.Instance.IsServer = true;
        SceneManager.LoadScene(SCENE_GSDK_NETWORK_BASE);
#else
        qsdk.RPCNetworkHandler.Instance.IsServer = false;
        SceneManager.LoadScene(SCENE_GSDK_MAIN);
#endif
    }
}
