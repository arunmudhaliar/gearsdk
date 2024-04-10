using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI; // Include this if you are using Unity's built-in UI
using TMPro; // Uncomment if you're using TextMeshPro

public class LogToUI : MonoBehaviour {
    public TMPro.TextMeshProUGUI logText; // Reference to your UI Text element. Assign this in the inspector.
    // public TextMeshProUGUI logText; // Use this line instead if you're using TextMeshPro

    private void OnEnable() {
        // Subscribe to the log message event
        Application.logMessageReceived += HandleLog;
    }

    private void OnDisable() {
        // Unsubscribe from the log message event when this object is disabled
        Application.logMessageReceived -= HandleLog;
    }

    // This method is called whenever a log message is received
    private void HandleLog( string logString, string stackTrace, LogType type ) {
        // Append the log message to the text element's text
        // This example adds the log type as a prefix, but you can customize it as needed
        logText.text += $"\n[{type}] {logString}";

        // Optionally log the stack trace if it's an error or exception
        if (type == LogType.Error || type == LogType.Exception) {
            logText.text += $"\n{stackTrace}";
        }

        // To prevent the text from growing indefinitely, you might want to limit the number of lines or characters.
        // Here's a simple way to keep only the last N characters:
        // int keepCharacters = 5000;
        // if (logText.text.Length > keepCharacters)
        // {
        //     logText.text = logText.text.Substring(logText.text.Length - keepCharacters);
        // }
    }

    public void clearLogs() {
        logText.text = "";
    }
}
