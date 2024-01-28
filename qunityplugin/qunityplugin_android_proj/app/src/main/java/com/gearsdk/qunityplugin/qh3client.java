package com.gearsdk.qunityplugin;

public class qh3client {
    public interface request_cb_interface {
        void callback_method(String message);
    }
    public native boolean send_async_request(String host, String port,
                                             String path, String payload, request_cb_interface callback);
}
