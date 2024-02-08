using System;
using System.Runtime.InteropServices;
using AOT;
using UnityEngine;

public class qunitysdk : MonoBehaviour
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void type_qh3client_helper_cb(string payload, IntPtr arg, int result);

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
}