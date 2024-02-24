using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using AOT;
using System.Runtime.InteropServices;
using System.Text;
using UnityEngine.UI;
using System.Net;
using System.Net.Sockets;

public class TestScript : MonoBehaviour
{
    public UnityEngine.UI.ScrollRect scrollRect;
    public GameObject resultLabelPrefab;
    public TMPro.TextMeshProUGUI titleText;
    public TMPro.TMP_InputField no_of_requests;
    public UnityEngine.UI.Scrollbar progress;

    public TMPro.TextMeshProUGUI qsocket1_response_text;
    public TMPro.TextMeshProUGUI qsocket2_response_text;

    public Button qsocket1_send_btn;
    public Button qsocket2_send_btn;

    public Button qsocket1_close_btn;
    public Button qsocket2_close_btn;
    public TMPro.TMP_InputField server_ip;
    private string previous_valid_ip = "192.168.0.230";

    public int total_requests = 0;
    public int total_request_response_came = 0;

    struct qReqArg {
        public TestScript instance;
        public IntPtr arg;
    }
    // Start is called before the first frame update
    void Start()
    {
        Debug.Log("TestScript - Start");
        qsocket1 = new qunitysdk.qsocket();
        qsocket2 = new qunitysdk.qsocket();
        previous_valid_ip = PlayerPrefs.GetString("server_ip", previous_valid_ip);
        server_ip.text = previous_valid_ip;
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
            //Debug.Log(payload + ", arg " + arg + ", result " + result);
            qReqArg reqArg = FromIntPtr(arg);
            TestScript thiz = reqArg.instance;
            if ((int)reqArg.arg < thiz.scrollRect.content.childCount) {
                Transform text_game_object = thiz.scrollRect.content.GetChild((int)reqArg.arg);
                TMPro.TextMeshProUGUI result_text = text_game_object.GetComponent<TMPro.TextMeshProUGUI>();
                result_text.color = Color.green;
                result_text.text = payload + ", result " + success;
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
            result_text.text = "waiting ...";
            result_text.color = Color.red;
            qReqArg newArg = new qReqArg();
            newArg.instance = this;
            newArg.arg = (IntPtr)(child_count+x);
            IntPtr instancePtr = (IntPtr)GCHandle.Alloc(newArg, GCHandleType.Normal);
            qunitysdk.send_async_request(server_ip.text.Trim(), "4004", "/whoami", "{}", instancePtr, test_callback, 1);
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

    // qsocket
    qunitysdk.qsocket qsocket1 = null;
    qunitysdk.qsocket qsocket2 = null;

    public void On_C1_QSocketConnect() {
        OnIpAddressEntered();

        qsocket1.connect(server_ip.text.Trim(), "4000", IntPtr.Zero);
        qsocket1.OnConnect = onconnect;
        qsocket1.OnMessage = onmessage;
        qsocket1.OnReleaseConnection = onreleaseconnection;
        qsocket1.OnClose = onclose;
    }

    public void On_C1_QSocketSend() {
        qsocket1.sendMessage("Hello c2 !!!", true);
    }

    public void On_C2_QSocketConnect() {
        OnIpAddressEntered();

        qsocket2.connect(server_ip.text.Trim(), "4000", IntPtr.Zero);
        qsocket2.OnConnect = onconnect;
        qsocket2.OnMessage = onmessage;
        qsocket2.OnReleaseConnection = onreleaseconnection;
        qsocket2.OnClose = onclose;
    }

    public void On_C2_QSocketSend() {
        qsocket2.sendMessage("Hello c1 !!!", true);
    }

    public void On_C1_SocketClose() {
        qsocket1.close();
    }
    public void On_C2_SocketClose() {
        qsocket2.close();
    }
    public void On_PrintQsocketsInfo() {
        qunitysdk.qsocket_print_info();
    }


    ///
    protected void onconnect(qunitysdk.qsocket qs) {
        Debug.Log("qsocket connected " + qs.guid_crc);
        if (qsocket1 == qs) {
            qsocket1_send_btn.interactable = true;
            qsocket1_close_btn.interactable = true;
        } else {
            qsocket2_send_btn.interactable = true;
            qsocket2_close_btn.interactable = true;
        }
    }

    protected void onmessage( qunitysdk.qsocket qs, ulong recv_len, string msg ) {
        Debug.Log("qsocket message received " + qs.guid_crc);
        if (qsocket1 == qs) {
            qsocket2_response_text.text = msg;
            qsocket2_response_text.gameObject.SetActive(true);
        } else {
            qsocket1_response_text.text = msg;
            qsocket1_response_text.gameObject.SetActive(true);
        }
        Debug.Log("msg : len " + recv_len +" - "+ msg);
    }

    protected void onreleaseconnection( qunitysdk.qsocket qs ) {
        Debug.Log("qsocket release connection " + qs.guid_crc);
    }

    protected void onclose( qunitysdk.qsocket qs ) {
        Debug.Log("qsocket close connection " + qs.guid_crc);
        if (qsocket1 == qs) {
            qsocket1_send_btn.interactable = false;
            qsocket1_close_btn.interactable = false;
        } else {
            qsocket2_send_btn.interactable = false;
            qsocket2_close_btn.interactable = false;
        }
    }

    public void OnIpAddressEntered() {
        if (!TestScript.IsValidIPv4(server_ip.text)) {
            Debug.LogError("Invalid ip address, resetting to previous valid value !!!");
            server_ip.text = previous_valid_ip;
            return;
        }
        previous_valid_ip = server_ip.text.Trim();
        PlayerPrefs.SetString("server_ip", previous_valid_ip);
    }
}
