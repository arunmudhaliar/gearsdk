using System.Text.Json;
using UnityEngine.Serialization;
using UnityEngine.UI;

public class StatefulClientExample : QNetClientLifeCycle
{
    public Button _send_btn;
    public Button _close_btn;
    public TMPro.TextMeshProUGUI _response_text;
    
    private void Start() {
        InitLifeCycle();
    }

    private void SetResponseText(string msg)
    {
        _response_text.text = msg;
        _response_text.gameObject.SetActive(true);
    }
    
    public void On_Btn_QSocketConnect() {
        StartClientConnection();
    }

    public void On_Btn_QSocketSend() {
        SendMessage("Hello !!!", true);
    }

    public void On_Btn_SocketClose() {
        CloseConnection();
    }

    public override void OnInitLifeCycle()
    {
        LogMessage("OnInitLifeCycle");
    }

    public override void OnClientConnect()
    {
        LogMessage("OnClientConnect");
        RequestMatch();
        _close_btn.interactable = true;
    }

    public override void OnClientReleaseConnection()
    {
        _send_btn.interactable = false;
        _close_btn.interactable = false;
    }
    public override void OnClientCloseConnection(){}
    
    public override void OnRoomPlayerAdd(ulong len, byte[] buf, string msg)
    {
        // msg_room_server_event_player_add msgObj = JsonSerializer.Deserialize<msg_room_server_event_player_add>(msg);
        // LogMessage($"received PlayerAdd Event: {JsonSerializer.Serialize(msgObj)}");
        LogMessage($"received PlayerAdd Event raw msg: {msg}");
        SetResponseText(msg);
    }
    
    public override void OnRoomPlayerRemove(ulong len, byte[] buf, string msg)
    {
        // msg_room_server_event_player_remove msgObj = JsonSerializer.Deserialize<msg_room_server_event_player_remove>(msg);
        // LogMessage($"received PlayerRemove Event: {JsonSerializer.Serialize(msgObj)}");
        LogMessage($"received PlayerRemove Event raw msg: {msg}");
        SetResponseText(msg);
    }
    
    public override void OnRoomStart(ulong len, byte[] buf, string msg)
    {
        // msg_room_server_event_start msgObj = JsonSerializer.Deserialize<msg_room_server_event_start>(msg);
        // LogMessage($"received RoomStart Event: {JsonSerializer.Serialize(msgObj)}");
        LogMessage($"received RoomStart Event raw msg: {msg}");
        _send_btn.interactable = true;
        SetResponseText(msg);
    }
    
    public override void OnRoomEnd(ulong len, byte[] buf, string msg)
    {
        // msg_room_server_event_end msgObj = JsonSerializer.Deserialize<msg_room_server_event_end>(msg);
        // LogMessage($"received RoomEnd Event: {JsonSerializer.Serialize(msgObj)}");
        LogMessage($"received RoomEnd Event raw msg: {msg}");
        _send_btn.interactable = false;
        SetResponseText(msg);
    }
}
