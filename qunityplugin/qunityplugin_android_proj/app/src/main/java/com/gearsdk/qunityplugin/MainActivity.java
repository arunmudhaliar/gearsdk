package com.gearsdk.qunityplugin;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;
import android.os.Handler;
import android.os.Looper;

import com.gearsdk.qunityplugin.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'qunityplugin' library on application startup.
    static {
        System.loadLibrary("ev");
        System.loadLibrary("qunityplugin");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());

        qh3client_android client = new qh3client_android();
        qh3client_android.request_cb_interface callback = (payload, arg, result) -> {
            System.out.println("qplugin-app - Callback received: " + payload +", arg "+arg +", result "+result);
            Handler mainHandler = new Handler(Looper.getMainLooper());
            mainHandler.post(new Runnable() {
                @Override
                public void run() {
                    tv.setText(payload+", arg "+arg +", result "+result);
                }
            });
        };
        for (int x=0;x<10;x++) {
            client.send_async_request("192.168.0.230", "4004", "/whoami", "{}", x, callback);
        }
    }

    /**
     * A native method that is implemented by the 'qunityplugin' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}