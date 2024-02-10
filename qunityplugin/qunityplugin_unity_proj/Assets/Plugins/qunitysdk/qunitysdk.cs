using System;
using System.Runtime.InteropServices;
using AOT;
using UnityEngine;

public class qunitysdk : MonoBehaviour
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void type_qh3client_helper_cb(string payload, IntPtr arg, int result);

    // qsocket
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void type_qsocket_onconnect();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void type_qsocket_onmessage(ulong recvLen, IntPtr buf);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void type_qsocket_onreleaseconnection();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void type_qsocket_onclose();

    public abstract class mqsocket {
        abstract protected void onconnect();
        abstract protected void onmessage( ulong recv_len, IntPtr buf );
        abstract protected void onreleaseconnection();
        abstract protected void onclose();
    }
    public class qsocket : mqsocket {
        public Guid guid { get; private set; }
        public string guid_str;

        public qsocket() {
            guid = Guid.NewGuid();
            guid_str = guid.ToString();
            Debug.Log("qsocket guid - " + guid.ToString());
        }

        protected override void onconnect() {
            Debug.Log("qsocket connected");
        }

        protected override void onmessage( ulong recv_len, IntPtr buf ) {
            Debug.Log("qsocket message received");
        }

        protected override void onreleaseconnection() {
            Debug.Log("qsocket release connection");
        }

        protected override void onclose() {
            Debug.Log("qsocket close connection");
        }

        public bool connect(string host, string port, IntPtr arg) {
            return qsocket_connect(guid_str, guid_str.Length, host, port, arg, onconnect, onmessage, onreleaseconnection, onclose);
        }

        public bool is_finished() {
            return qsocket_is_run_finished(guid_str, guid_str.Length);
        }

        public int sendMessage(string buffer, bool flush) {
            byte[] bytes = System.Text.Encoding.UTF8.GetBytes(buffer);
            return qsocket_sendMessage(guid_str, guid_str.Length, bytes, (ulong)bytes.Length, flush);
        }
    }

#if UNITY_EDITOR_OSX
    const string cp = "Assets/Plugins/qunitysdk/libs/macos/libqunityplugin.dylib";
#elif UNITY_IOS || UNITY_IPHONE
    const string cp = "__Internal";
#elif UNITY_ANDROID
    const string cp = "libqunityplugin.so";
#endif

    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern int entry_point();

    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern int send_async_request(string host, string port, string path, string payload, IntPtr arg, type_qh3client_helper_cb cb);

    // qsocket
    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern bool qsocket_connect( string guid, int guid_len, string host, string port, IntPtr arg,
                         type_qsocket_onconnect cb_connect, type_qsocket_onmessage cb_message,
                         type_qsocket_onreleaseconnection cb_release_connection, type_qsocket_onclose cb_close);
    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern bool qsocket_is_run_finished(string guid, int guid_len );
    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern void destroy_finished_qsockets();
    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern int qsocket_sendMessage(string guid, int guid_len, byte[] buffer, ulong size, bool flush);
}