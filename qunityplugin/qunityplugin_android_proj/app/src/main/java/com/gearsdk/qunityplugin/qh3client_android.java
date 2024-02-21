package com.gearsdk.qunityplugin;

public class qh3client_android {
    public interface request_cb_interface {
        void callback_method(String payload, Object arg, boolean success);
    }
    public native boolean send_async_request(String host, String port,
                                             String path, String payload, Object arg, request_cb_interface callback, int retry);
}
