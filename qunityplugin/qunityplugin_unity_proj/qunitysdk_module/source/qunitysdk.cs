using System;
using System.Runtime.InteropServices;
using AOT;
using UnityEngine;
using System.Collections.Generic;

public class qunitysdk : MonoBehaviour
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void type_qh3client_helper_cb(string payload, IntPtr arg, bool success);

    // qsocket
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void type_qsocket_onconnect(ulong guid_crc);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void type_qsocket_onmessage( ulong guid_crc, ulong recvLen, IntPtr buf);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void type_qsocket_onreleaseconnection( ulong guid_crc );
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void type_qsocket_onclose( ulong guid_crc );

    public abstract class mqsocket {
        abstract protected void onconnect();
        abstract protected void onmessage( ulong recv_len, string buf );
        abstract protected void onreleaseconnection();
        abstract protected void onclose();
    }
    public class qsocket : mqsocket {
        public Guid guid { get; private set; }
        public ulong guid_crc = 0;

        public Action<qsocket> OnConnect;
        public Action<qsocket, ulong, string> OnMessage;
        public Action<qsocket> OnReleaseConnection;
        public Action<qsocket> OnClose;

        public qsocket() {
            guid = Guid.NewGuid();
            string guid_str = guid.ToString();
            guid_crc = get_crc32(guid_str, guid_str.Length);
            Debug.Log("qsocket guid_crc - " + guid_crc);
        }

        protected override void onconnect() {
            OnConnect(this);
        }

        protected override void onmessage( ulong recv_len, string buf ) {
            OnMessage(this, recv_len, buf);
        }

        protected override void onreleaseconnection() {
            OnReleaseConnection(this);
        }

        protected override void onclose() {
            OnClose(this);
        }

        [MonoPInvokeCallback(typeof(qunitysdk.type_qsocket_onconnect))]
        private static void global_onconnect( ulong guid_crc ) {
            MainThreadDispatcher.RunOnMainThread(() => {
                if (!qunitysdk.sockets.ContainsKey(guid_crc)) {
                    return;
                }
                qunitysdk.sockets[guid_crc].onconnect();
            });
        }
        [MonoPInvokeCallback(typeof(qunitysdk.type_qsocket_onmessage))]
        private static void global_onmessage( ulong guid_crc, ulong recv_len, IntPtr buf ) {

            byte[] array = new byte[recv_len];

            // Copy the bytes from the unmanaged memory to the byte array
            Marshal.Copy(buf, array, 0, (int)recv_len);

            string msg = System.Text.Encoding.UTF8.GetString(array, 0, (int)recv_len);
            MainThreadDispatcher.RunOnMainThread(() => {
                if (!qunitysdk.sockets.ContainsKey(guid_crc)) {
                    return;
                }
                qunitysdk.sockets[guid_crc].onmessage(recv_len, msg);
            });
        }
        [MonoPInvokeCallback(typeof(qunitysdk.type_qsocket_onreleaseconnection))]
        private static void global_onreleaseconnection( ulong guid_crc ) {
            MainThreadDispatcher.RunOnMainThread(() => {
                if (!qunitysdk.sockets.ContainsKey(guid_crc)) {
                    return;
                }
                qunitysdk.sockets[guid_crc].onreleaseconnection();
                qunitysdk.sockets.Remove(guid_crc);
                Debug.Log("removing qsocket " + guid_crc + ", socket count " + qunitysdk.sockets.Count);
            });
        }
        [MonoPInvokeCallback(typeof(qunitysdk.type_qsocket_onclose))]
        private static void global_onclose( ulong guid_crc ) {
            MainThreadDispatcher.RunOnMainThread(() => {
                if (!qunitysdk.sockets.ContainsKey(guid_crc)) {
                    return;
                }
                qunitysdk.sockets[guid_crc].onclose();
            });
        }

        public bool connect(string host, string port, IntPtr arg) {
            if (qunitysdk.sockets.ContainsKey(guid_crc)) {
                Debug.LogWarning("qsocket already in use !!!");
                return false;
            }
            sockets.Add(guid_crc, this);
            return qsocket_connect(guid_crc, host, port, arg,
                global_onconnect, global_onmessage, global_onreleaseconnection, global_onclose);
        }

        public bool is_finished() {
            return qsocket_is_run_finished(guid_crc);
        }

        public int sendMessage(string buffer, bool flush) {
            byte[] bytes = System.Text.Encoding.UTF8.GetBytes(buffer);
            return qsocket_sendMessage(guid_crc, bytes, (ulong)bytes.Length, flush);
        }

        public int close() {
            return qsocket_close(guid_crc);
        }
    }

#if UNITY_EDITOR_OSX
    const string cp = "Assets/Plugins/qunitysdk/libs/macos/libqunityplugin.dylib";
#elif UNITY_STANDALONE_OSX
    const string cp = "qunityplugin";
#elif UNITY_IOS || UNITY_IPHONE
    const string cp = "__Internal";
#elif UNITY_ANDROID
    const string cp = "libqunityplugin.so";
#endif

    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern void pre_init_sdk();

    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern int send_async_request(string host, string port, string path, string payload, IntPtr arg, type_qh3client_helper_cb cb, int retry);

    // qsocket
    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern bool qsocket_connect( ulong guid_crc, string host, string port, IntPtr arg,
                         type_qsocket_onconnect cb_connect, type_qsocket_onmessage cb_message,
                         type_qsocket_onreleaseconnection cb_release_connection, type_qsocket_onclose cb_close);
    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern bool qsocket_is_run_finished( ulong guid_crc );
    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern void destroy_finished_qsockets();
    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern int qsocket_sendMessage( ulong guid_crc, byte[] buffer, ulong size, bool flush);
    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern void qsocket_print_info();
    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern ulong get_crc32(string guid, int guid_len);
    [DllImport(cp, CallingConvention = CallingConvention.Cdecl)]
    public static extern int qsocket_close( ulong guid_crc );

    private static qunitysdk instance;
    private static Dictionary<ulong, qsocket> sockets = new Dictionary<ulong, qsocket>();
    private void Start() {
        if (instance != null) {
            Destroy(this.gameObject);
            return;
        }

        pre_init_sdk();
        this.gameObject.AddComponent<MainThreadDispatcher>();
        instance = this;
        DontDestroyOnLoad(this.gameObject);
    }
}