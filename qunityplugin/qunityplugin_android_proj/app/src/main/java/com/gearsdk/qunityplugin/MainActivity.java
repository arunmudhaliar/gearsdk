package com.gearsdk.qunityplugin;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import com.gearsdk.qunityplugin.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'qunityplugin' library on application startup.
    static {
        System.loadLibrary("ev");
//        System.loadLibrary("quiche");
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
    }

    /**
     * A native method that is implemented by the 'qunityplugin' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}