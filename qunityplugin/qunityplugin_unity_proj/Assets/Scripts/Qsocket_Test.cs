using System;
using UnityEngine;
using UnityEngine.UI;

public class Qsocket_Test : MonoBehaviour
{
    qunitysdk.qsocket qsocket = null;
    public Button send_btn;
    public Button close_btn;
    public TMPro.TextMeshProUGUI response_text;
    public string server_ip = "127.0.0.1";

    private void Start() {
        qsocket = new qunitysdk.qsocket();
    }

    public void On_Btn_QSocketConnect() {
        qsocket.connect(server_ip, "4000", IntPtr.Zero);
        qsocket.OnConnect = onconnect;
        qsocket.OnMessage = onmessage;
        qsocket.OnReleaseConnection = onreleaseconnection;
        qsocket.OnClose = onclose;
    }

    public void On_Btn_QSocketSend() {
        qsocket.sendMessage("Hello !!!", true);
    }

    public void On_Btn_SocketClose() {
        qsocket.close();
    }


    protected void onconnect( qunitysdk.qsocket qs ) {
        Debug.Log("qsocket connected " + qs.guid_crc);
        msg_room_match_request msg_room_config_packet = new msg_room_match_request();
        msg_room_config_packet.room_config = new msg_room_config();
        string payload = JsonUtility.ToJson(msg_room_config_packet);
        Debug.Log("-->" + payload);
        qs.sendMessage(payload, true);
        send_btn.interactable = true;
        close_btn.interactable = true;
    }

    protected void onmessage( qunitysdk.qsocket qs, ulong recv_len, string msg ) {
        Debug.Log("qsocket message received " + qs.guid_crc);
        response_text.text = msg;
        response_text.gameObject.SetActive(true);
        Debug.Log("msg : len " + recv_len + " - " + msg);
    }

    protected void onreleaseconnection( qunitysdk.qsocket qs ) {
        Debug.Log("qsocket release connection " + qs.guid_crc);
    }

    protected void onclose( qunitysdk.qsocket qs ) {
        Debug.Log("qsocket close connection " + qs.guid_crc);
        send_btn.interactable = false;
        close_btn.interactable = false;
    }
}
