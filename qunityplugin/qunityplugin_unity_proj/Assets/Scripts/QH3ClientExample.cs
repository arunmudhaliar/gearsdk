using System;
using UnityEngine;
using AOT;
using System.Runtime.InteropServices;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using UnityEngine.Serialization;

[Serializable]
public struct QH3ClientExampleTestData
{
    public int type;
    public string name;
}

public class QH3ClientExample : qsdk.qnetbehaviour
{
    public TMPro.TextMeshProUGUI _appTitle;
    public UnityEngine.UI.ScrollRect _scrollRect;
    public UnityEngine.UI.ScrollRect _scrollRect_qsocket;
    public GameObject _resultLabelPrefab;
    public GameObject _qsocketTestPrefab;
    public TMPro.TMP_InputField _no_of_requests;
    public UnityEngine.UI.Scrollbar _progress;
    public UnityEngine.UI.Toggle _userLoginRequestToggle;
    public TMPro.TMP_InputField _server_ip;
    public UnityEngine.UI.Toggle _forceCmdServerShutdownToggle;
    private string _previous_valid_ip = "192.168.0.230";

    public int _total_requests = 0;
    public int _total_request_response_came = 0;
    private qunitysdk.qsocket _qsocket_to_send_shutdown = null;

    struct qReqArg {
        public QH3ClientExample instance;
        public IntPtr arg;
    }

    [qsdk.ServerRPC]
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
    
    // private void Mock_SendData( QNetMetaInstance qnetMeta, byte[] data ) {
    //     Debug.Log($"Mock SendData - {Encoding.UTF8.GetString(data)}");
    // }
    //
    // private void Mock_BroadCastData( QNetMetaInstance qnetMeta, byte[] data ) {
    //     Debug.Log($"Mock BroadCastData - {Encoding.UTF8.GetString(data)}");
    // }
    
    // Start is called before the first frame update
    void Start()
    {
        // // MessagePackInitializer.Init();
        // _isServer = true;
        // Debug.Log("DSsssssS");
        // // MessagePackSerializer.Serialize(new TestData());
        // // Debug.Log("~DSSSSSS");
        //
        // qsdk.RPCNetworkHandler.Instance.RegisterSendCallback(Mock_SendData);
        // qsdk.RPCNetworkHandler.Instance.RegisterBroadCastCallback(Mock_BroadCastData);
        // //
        // Invoke_RegisterRPCMethods();
        // // var _rpcRegistry = GeneratedProxies.RpcMethodRegistry.Instance;
        // // _rpcRegistry.GetInfo();
        // // _rpcRegistry.InvokeMethod(GeneratedProxies.RpcMethodId.TestScript_ProcessPlayerAction, new object[]{"hello", 1});
        // // Invoke_UnRegisterRPCMethods();
        // TestProcessPlayerAction("TRY1", new QH3ClientExampleTestData());
        // // ProcessPlayerAction2("trr", 2);
        // TestShowMessage("A", new QH3ClientExampleTestData(), "hello");
        // TestBroadcastMessage("BBB");
        // Invoke_UnRegisterRPCMethods();
        
        Debug.Log("QH3ClientExample - Start");
        _appTitle.text = $"qunityplugin {Application.version}";
        Screen.fullScreen = false;
        ShowStatusBar();
        _previous_valid_ip = PlayerPrefs.GetString("server_ip", _previous_valid_ip);
        _server_ip.text = _previous_valid_ip;
        _qsocket_to_send_shutdown = new qunitysdk.qsocket();
    }

    public void ShowStatusBar()
    {
        if (Application.platform == RuntimePlatform.Android)
        {
            AndroidJavaClass unityPlayer = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
            AndroidJavaObject activity = unityPlayer.GetStatic<AndroidJavaObject>("currentActivity");
            AndroidJavaObject window = activity.Call<AndroidJavaObject>("getWindow");

            activity.Call("runOnUiThread", new AndroidJavaRunnable(() =>
            {
                window.Call("clearFlags", 0x0400); // 0x0400 is the flag FLAG_FULLSCREEN.
            }));
        }
    }

    public static bool IsValidIPv4( string ipString ) {
        if (IPAddress.TryParse(ipString, out IPAddress ip)) {
            return ip.AddressFamily == AddressFamily.InterNetwork;
        }
        return false;
    }

    private static qReqArg FromIntPtr( IntPtr ptr ) {
        GCHandle handle = GCHandle.FromIntPtr(ptr);
        return (qReqArg)handle.Target;
    }

    public static void FreeHandle( IntPtr ptr ) {
        GCHandle handle = GCHandle.FromIntPtr(ptr);
        handle.Free();
    }

    [MonoPInvokeCallback(typeof(qunitysdk.type_qh3client_helper_cb))]
    private static void test_callback( string payload, IntPtr arg, bool success ) {
        MainThreadDispatcher.RunOnMainThread(() => {
            Debug.Log($"{ payload}, arg { arg }, result {success}");
            qReqArg reqArg = FromIntPtr(arg);
            QH3ClientExample thiz = reqArg.instance;
            if ((int)reqArg.arg < thiz._scrollRect.content.childCount) {
                Transform text_game_object = thiz._scrollRect.content.GetChild((int)reqArg.arg);
                TMPro.TextMeshProUGUI result_text = text_game_object.GetComponent<TMPro.TextMeshProUGUI>();
                result_text.color = Color.green;
                result_text.text = $"{payload}, result {success}";
            }
            thiz._total_request_response_came++;
            thiz.UpateProgress();
            
            FreeHandle(arg);
        });
    }
    public void OnSendRequest() {
        //qunitysdk.type_qh3client_helper_cb callback = ( string payload, IntPtr arg, int result ) => {
        //    MainThreadDispatcher.RunOnMainThread(() => {
        //        //Debug.Log(payload + ", arg " + arg + ", result " + result);
        //        Transform text_game_object = scrollRect.content.GetChild((int)arg);
        //        if (text_game_object == null) {
        //            return;
        //        }
        //        TMPro.TextMeshProUGUI result_text = text_game_object.GetComponent<TMPro.TextMeshProUGUI>();
        //        result_text.color = Color.green;
        //        total_request_response_came++;
        //        UpateProgress();
        //        result_text.text = payload + ", arg " + arg + ", result " + result;
        //    });
        //};

        OnIpAddressEntered();
        
        rq_msg_user_get msg_user_get = new rq_msg_user_get();
        string osDescription = RuntimeInformation.OSDescription;
        string machineName = Environment.MachineName;
        string osArchitecture = RuntimeInformation.OSArchitecture.ToString();
        string dotNetVersion = Environment.Version.ToString();
        int processorCount = Environment.ProcessorCount;

        msg_user_get.device.sys_name = osDescription;
        msg_user_get.device.arch = osArchitecture;
        msg_user_get.device.node_name = machineName;
        msg_user_get.device.release = dotNetVersion;

        string payload = JsonSerializer.Serialize(msg_user_get);
        Debug.Log($"sending {payload}");
        int batch_req_count = 10;
        if (!int.TryParse(_no_of_requests.text, out batch_req_count)) {
            Debug.LogWarning("TestScript - invalid input !!!");
            batch_req_count = 10;
            _no_of_requests.text = batch_req_count.ToString();
        }
        int child_count = _scrollRect.content.childCount;
        for (int x = 0; x < batch_req_count; x++) {
            TMPro.TextMeshProUGUI result_text = Instantiate(_resultLabelPrefab).GetComponent<TMPro.TextMeshProUGUI>();
            result_text.transform.SetParent(_scrollRect.content);
            result_text.transform.localScale = Vector3.one;
            result_text.text = "waiting ...";
            result_text.color = Color.red;
            qReqArg newArg = new qReqArg();
            newArg.instance = this;
            newArg.arg = (IntPtr)(child_count+x);
            IntPtr instancePtr = (IntPtr)GCHandle.Alloc(newArg, GCHandleType.Normal);
            if (_userLoginRequestToggle.isOn) {
                qunitysdk.send_async_request(_server_ip.text.Trim(), "4004", "/user_get", payload, instancePtr, test_callback, 1);
            } else {
                qunitysdk.send_async_request(_server_ip.text.Trim(), "4004", "/whoami", "{}", instancePtr, test_callback, 1);
            }
            _total_requests++;
        }
        UpateProgress();
    }

    private void UpateProgress() {
        _progress.size = (float)_total_request_response_came / _total_requests;
    }

    public void OnClearResults() {
        _total_requests = 0;
        _total_request_response_came = 0;
        _progress.size = 0;
        for (int x = 0; x < _scrollRect.content.childCount; x++) {
            Destroy(_scrollRect.content.GetChild(x).gameObject);
        }
        _scrollRect.content.DetachChildren();
    }

    public void On_Btn_Add_Qsocket() {
        OnIpAddressEntered();

        StatefulClientExample test_qsocket = Instantiate(_qsocketTestPrefab).GetComponent<StatefulClientExample>();
        test_qsocket.transform.SetParent(_scrollRect_qsocket.content);
        test_qsocket.ServerIp = _server_ip.text.Trim();
        test_qsocket.transform.localScale = Vector3.one;
    }
    public void On_PrintQsocketsInfo() {
        qunitysdk.qsocket_print_info();
    }

    public void OnIpAddressEntered() {
        if (!QH3ClientExample.IsValidIPv4(_server_ip.text)) {
            Debug.LogError("Invalid ip address, resetting to previous valid value !!!");
            _server_ip.text = _previous_valid_ip;
            return;
        }
        _previous_valid_ip = _server_ip.text.Trim();
        for (int x = 0; x < _scrollRect_qsocket.content.childCount; x++) {
            StatefulClientExample qsocket_test = _scrollRect_qsocket.content.GetChild(x).GetComponent<StatefulClientExample>();
            qsocket_test.ServerIp = _server_ip.text.Trim();
        }
        PlayerPrefs.SetString("server_ip", _previous_valid_ip);
    }

    public void ShutDown_QServer() {
        _qsocket_to_send_shutdown.connect(_previous_valid_ip, "4000", IntPtr.Zero);
        _qsocket_to_send_shutdown.OnConnect = ( qunitysdk.qsocket qs, IntPtr user_data ) => {
            Debug.Log("shutdown socket : connect");
            msg_room_server_shutdown msg_room_server_shutdown_packet = new msg_room_server_shutdown();
            string payload = JsonSerializer.Serialize(msg_room_server_shutdown_packet);
            Debug.Log($"sending : {payload}");
            qs.sendMessage(payload, true);
        };
        _qsocket_to_send_shutdown.OnMessage = ( qunitysdk.qsocket qs, ulong recv_len, byte[] buffer ) => {
            string msg = System.Text.Encoding.UTF8.GetString(buffer, 0, (int)recv_len);
            Debug.Log($"shutdown socket : message {msg}");
        };
        _qsocket_to_send_shutdown.OnReleaseConnection = ( qunitysdk.qsocket qs ) => {
            Debug.Log("shutdown socket : release");
        };
        _qsocket_to_send_shutdown.OnClose = ( qunitysdk.qsocket qs ) => {
            Debug.Log("shutdown socket : closed");
        };
    }

    public void Shutdown_Qh3Server() {
        OnIpAddressEntered();
        qunitysdk.send_async_request(_server_ip.text.Trim(), "4010", _forceCmdServerShutdownToggle.isOn ? "/shutdown_cmd_center" : "/shutdown_test", "", IntPtr.Zero, null, 0);
    }
}
