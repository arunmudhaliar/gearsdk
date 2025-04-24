using System.Collections.Concurrent;
using UnityEngine;

public class MainThreadDispatcher : MonoBehaviour {
    private static MainThreadDispatcher instance;
    private ConcurrentQueue<System.Action> actions = new ConcurrentQueue<System.Action>();
    [SerializeField]
    private int actionCount = 0;
    private void Awake() {
        if (instance == null) {
            instance = this;
            DontDestroyOnLoad(this.gameObject);
        } else {
            Destroy(this.gameObject);
        }
    }

    public static void RunOnMainThread( System.Action action ) {
        if (instance) {
            instance.actions.Enqueue(action);
            instance.actionCount = instance.actions.Count;
        }
    }

    private void Update() {
        while (actions.TryDequeue(out var action)) {
            action?.Invoke();
        }
    }
}
