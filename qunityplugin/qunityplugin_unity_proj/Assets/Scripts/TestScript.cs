using System;
using UnityEngine;
using AOT;
using System.Runtime.InteropServices;
using System.Net;
using System.Net.Sockets;

public class TestScript : MonoBehaviour
{
    public TMPro.TextMeshProUGUI appTitle;
    public UnityEngine.UI.ScrollRect scrollRect;
    public UnityEngine.UI.ScrollRect scrollRect_qsocket;
    public GameObject resultLabelPrefab;
    public GameObject qsocketTestPrefab;
    public TMPro.TMP_InputField no_of_requests;
    public UnityEngine.UI.Scrollbar progress;
    public UnityEngine.UI.Toggle userLoginRequestToggle;
    public TMPro.TMP_InputField server_ip;
    private string previous_valid_ip = "192.168.0.230";

    public int total_requests = 0;
    public int total_request_response_came = 0;
    private qunitysdk.qsocket qsocket_to_send_shutdown = null;

    struct qReqArg {
        public TestScript instance;
        public IntPtr arg;
    }

    // Start is called before the first frame update
    void Start()
    {
        Debug.Log("TestScript - Start");
        appTitle.text = $"qunityplugin {Application.version}";
        Screen.fullScreen = false;
        ShowStatusBar();
        previous_valid_ip = PlayerPrefs.GetString("server_ip", previous_valid_ip);
        server_ip.text = previous_valid_ip;
        qsocket_to_send_shutdown = new qunitysdk.qsocket();
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
            TestScript thiz = reqArg.instance;
            if ((int)reqArg.arg < thiz.scrollRect.content.childCount) {
                Transform text_game_object = thiz.scrollRect.content.GetChild((int)reqArg.arg);
                TMPro.TextMeshProUGUI result_text = text_game_object.GetComponent<TMPro.TextMeshProUGUI>();
                result_text.color = Color.green;
                result_text.text = $"{payload}, result {success}";
            }
            thiz.total_request_response_came++;
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
        msg_user_get.device.duid = qunitysdk.get_device_duid();
        msg_user_get.device.locale = qunitysdk.get_device_locale();
        Debug.Log($"device: duid - {msg_user_get.device.duid}, locale - {msg_user_get.device.locale}");

        string payload = JsonUtility.ToJson(msg_user_get);

        int batch_req_count = 10;
        if (!int.TryParse(no_of_requests.text, out batch_req_count)) {
            Debug.LogWarning("TestScript - invalid input !!!");
            batch_req_count = 10;
            no_of_requests.text = batch_req_count.ToString();
        }
        int child_count = scrollRect.content.childCount;
        for (int x = 0; x < batch_req_count; x++) {
            TMPro.TextMeshProUGUI result_text = Instantiate(resultLabelPrefab).GetComponent<TMPro.TextMeshProUGUI>();
            result_text.transform.SetParent(scrollRect.content);
            result_text.transform.localScale = Vector3.one;
            result_text.text = "waiting ...";
            result_text.color = Color.red;
            qReqArg newArg = new qReqArg();
            newArg.instance = this;
            newArg.arg = (IntPtr)(child_count+x);
            IntPtr instancePtr = (IntPtr)GCHandle.Alloc(newArg, GCHandleType.Normal);
            if (userLoginRequestToggle.isOn) {
                qunitysdk.send_async_request(server_ip.text.Trim(), "4004", "/user_get", payload, instancePtr, test_callback, 1);
            } else {
                qunitysdk.send_async_request(server_ip.text.Trim(), "4004", "/whoami", "{}", instancePtr, test_callback, 1);
            }
            total_requests++;
        }
        UpateProgress();
    }

    private void UpateProgress() {
        progress.size = (float)total_request_response_came / total_requests;
    }

    public void OnClearResults() {
        total_requests = 0;
        total_request_response_came = 0;
        progress.size = 0;
        for (int x = 0; x < scrollRect.content.childCount; x++) {
            Destroy(scrollRect.content.GetChild(x).gameObject);
        }
        scrollRect.content.DetachChildren();
    }

    public void On_Btn_Add_Qsocket() {
        OnIpAddressEntered();

        Qsocket_Test test_qsocket = Instantiate(qsocketTestPrefab).GetComponent<Qsocket_Test>();
        test_qsocket.transform.SetParent(scrollRect_qsocket.content);
        test_qsocket.server_ip = server_ip.text.Trim();
        test_qsocket.transform.localScale = Vector3.one;
    }
    public void On_PrintQsocketsInfo() {
        qunitysdk.qsocket_print_info();
    }

    public void OnIpAddressEntered() {
        if (!TestScript.IsValidIPv4(server_ip.text)) {
            Debug.LogError("Invalid ip address, resetting to previous valid value !!!");
            server_ip.text = previous_valid_ip;
            return;
        }
        previous_valid_ip = server_ip.text.Trim();
        for (int x = 0; x < scrollRect_qsocket.content.childCount; x++) {
            Qsocket_Test qsocket_test = scrollRect_qsocket.content.GetChild(x).GetComponent<Qsocket_Test>();
            qsocket_test.server_ip = server_ip.text.Trim();
        }
        PlayerPrefs.SetString("server_ip", previous_valid_ip);
    }

    public void ShutDown_QServer() {
        qsocket_to_send_shutdown.connect(previous_valid_ip, "4000", IntPtr.Zero);

        qsocket_to_send_shutdown.OnConnect = ( qunitysdk.qsocket qs, IntPtr user_data ) => {
            Debug.Log("shutdown socket : connect");
            msg_room_server_shutdown msg_room_server_shutdown_packet = new msg_room_server_shutdown();
            string payload = JsonUtility.ToJson(msg_room_server_shutdown_packet);
            Debug.Log($"sending : {payload}");
            qs.sendMessage(payload, true);
        };
        qsocket_to_send_shutdown.OnMessage = ( qunitysdk.qsocket qs, ulong recv_len, string msg ) => {
            Debug.Log($"shutdown socket : message {msg}");
        };
        qsocket_to_send_shutdown.OnReleaseConnection = ( qunitysdk.qsocket qs ) => {
            Debug.Log("shutdown socket : release");
        };
        qsocket_to_send_shutdown.OnClose = ( qunitysdk.qsocket qs ) => {
            Debug.Log("shutdown socket : closed");
        };
    }

    public void Shutdown_Qh3Server() {
        OnIpAddressEntered();
        qunitysdk.send_async_request(server_ip.text.Trim(), "4010", "/shutdown_test", "", IntPtr.Zero, null, 0);
    }
}
