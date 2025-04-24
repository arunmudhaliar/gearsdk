using System;
using System.Collections;
using System.Collections.Generic;
using qsdk;
using ServerPlugin;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.Serialization;

internal interface IQNetServerNetObjectLifeCycle
{
    void OnSpawn(qnetbehaviour networkObject);
    void OnDeSpawn(qnetbehaviour networkObject);
}
internal interface IQNetServerLifeCycle
{
    void OnLoadTemplateGameScene();
    void OnInitLifeCycle();
    void OnServerPreStart();
    void OnServerStarted();
    void OnServerStoped();
    void OnServerError(int err);
    void OnRoomCreate(int room);
    void OnPlayerAdd(int room, string pid);
    void OnPlayerRemove(int room, string pid);
    void OnRoomMessage(int room, string pid, string msg);
    void OnRoomStarted(int room);
    void OnRoomEnd(int room);
    void OnRoomDestroy(int room);
    void OnRoomCountDownStarted(int room, int cnt, int max);
    void OnRoomCountDownCancelled(int room);
}
public abstract class QNetServerLifeCycle : MonoBehaviour, IQNetServerLifeCycle, IQNetServerNetObjectLifeCycle
{
    [FormerlySerializedAs("gamescene")] [SerializeField]
    private string templateGameSceneName = "gsdk-network-scene";
    
    public void InitLifeCycle(string templateSceneName)
    {
        qnetbehaviour.LogInfo("Initing server lifecycle.");
        templateGameSceneName = templateSceneName;
        LoadTemplateGameScene();
    }
    
    protected void CreateGameNodeForMatch(IntPtr nativeServerPtr, IntPtr roomPtr, int room, string newRoomName)
    {
        Scene templateGameScene = SceneManager.GetSceneByName(templateGameSceneName);
        if (!templateGameScene.IsValid())
        {
            qnetbehaviour.LogError("Couldn't get game scene !!!");
            return;
        }

        GameObject newRoot = SetupGameNode(newRoomName, templateGameScene, nativeServerPtr, roomPtr, room);
        if (newRoot == null)
        {
            qnetbehaviour.LogError("Couldn't create new root !!!");
        }
    }
    
    GameObject SetupGameNode(string newRoomName, Scene templateScene, IntPtr nativeServerPtr, IntPtr roomPtr, int room)
    {
        // create the root object
        GameObject newRoot = new GameObject(newRoomName);
        
        // add QNetMetaInstance component
        GameObject methodRegistryInstance = new GameObject("QNetMetaInstance");
        QNetMetaInstance metaInstance = methodRegistryInstance.AddComponent<QNetMetaInstance>();
        metaInstance.serverMeta.nativeServer = nativeServerPtr;
        metaInstance.serverMeta.roomPtr = roomPtr;
        metaInstance.room = room;
        methodRegistryInstance.transform.SetParent(newRoot.transform);
        
        // copy objects
        GameObject[] rootObjects = templateScene.GetRootGameObjects();
        foreach (var go in rootObjects)
        {
            var cam = go.GetComponent<Camera>();
            if (cam)
            {
                cam.enabled = false;
            }

            var audioListener = go.GetComponent<AudioListener>();
            if (audioListener)
            {
                audioListener.enabled = false;
            }
            Instantiate(go, newRoot.transform);
        }
        
        // physics
        // Create a new scene for the room with a physics world
        var parameters = new CreateSceneParameters(LocalPhysicsMode.Physics3D);
        Scene roomScene = SceneManager.CreateScene(newRoomName, parameters);
        // Get the PhysicsScene associated with the newly created scene
        PhysicsScene roomPhysicsScene = roomScene.GetPhysicsScene();
        // Move all objects under this room node into the new physics scene
        SceneManager.MoveGameObjectToScene(newRoot, roomScene);
        //
        
        return newRoot;
    }
    
    public void UnloadPhysicsRoomScene(string roomName)
    {
        // Find the scene to unload
        Scene roomScene = SceneManager.GetSceneByName(roomName);

        if (roomScene.isLoaded)
        {
            // Unload the scene and its physics world
            SceneManager.UnloadSceneAsync(roomScene);
            qnetbehaviour.LogInfo($"{roomName} physics world unloaded.");
        }
        else
        {
            Debug.LogWarning($"{roomName} scene is not loaded.");
        }
    }
    
    protected void StartGameServer(QServerInputConfig config)
    {
        int result = ServerSdk.StartGameServer(
            config,
            (s, d) => MainThreadDispatcher.RunOnMainThread(ServerPreStart),
            s => MainThreadDispatcher.RunOnMainThread(ServerStarted),
            s => MainThreadDispatcher.RunOnMainThread(ServerStoped),
            (s, err) =>
            {
                MainThreadDispatcher.RunOnMainThread(() => { ServerError(err); });
            },
            (srv, room, ptr) =>
            {
                MainThreadDispatcher.RunOnMainThread(() =>
                {
                    RoomCreate(srv, ptr, room);
                });
            },
            (srv, room, ptr) =>
            {
                MainThreadDispatcher.RunOnMainThread(() =>
                {
                    RoomStarted(room);
                });
            },
            (srv, room, ptr, pid, hash) =>
            {
                MainThreadDispatcher.RunOnMainThread(() =>
                {
                    PlayerAdd(room, pid);
                });
            },
            (srv, room, ptr, pid, hash, msg) => 
            {
                MainThreadDispatcher.RunOnMainThread(() =>
                {
                    RoomMessage(room, pid, msg);
                });
            },
            (srv, room, ptr, pid, hash) =>
            {
                MainThreadDispatcher.RunOnMainThread(() =>
                {
                    PlayerRemove(room, pid);
                });
            },
            (srv, room, ptr) =>
            {
                MainThreadDispatcher.RunOnMainThread(() =>
                {
                    RoomEnd(room);
                });
            },
            (srv, room, ptr) =>
            {
                MainThreadDispatcher.RunOnMainThread(() =>
                {
                    RoomDestroy(room);
                });
            },
            (srv, room, ptr, cnt, max) =>
            {
                MainThreadDispatcher.RunOnMainThread(() =>
                {
                    RoomCountDownStarted(room, cnt, max);
                });
            },
            (srv, room, ptr) =>
            {
                MainThreadDispatcher.RunOnMainThread(() =>
                {
                    RoomCountDownCancelled(room);
                });
            });
    }
    
    private void LoadTemplateGameScene()
    {
        AsyncOperation loadOp = SceneManager.LoadSceneAsync(templateGameSceneName, LoadSceneMode.Additive);
        if (loadOp == null)
        {
            qnetbehaviour.LogError("LoadScene failed to return a valid AsyncOperation!");
            return;
        }
        loadOp.completed += (op) =>
        {
            qnetbehaviour.LogInfo($"Template Scene {templateGameSceneName} loaded.");
            OnLoadTemplateGameScene();
            OnInitLifeCycle();
            qnetbehaviour.LogInfo("Server lifecycle inited.");
        };
    }
    private void ServerPreStart()
    {
        qnetbehaviour.LogInfo("Server PreStart");
        OnServerPreStart();
    }
    private void ServerStarted()
    {
        qnetbehaviour.LogInfo("Server Started");
        OnServerStarted();
    }
    private void ServerStoped()
    {
        qnetbehaviour.LogInfo("Server Stopped");
        OnServerStoped();
    }
    private void ServerError(int err){
        Debug.LogError($"Server Error: {err}. Check <../Unity/Editor.log> for details.");
        OnServerError(err);
    }
    private void RoomCreate(IntPtr nativeServerPtr, IntPtr roomPtr, int room)
    {
        qnetbehaviour.LogInfo($"Room {room} Created");
        CreateGameNodeForMatch(nativeServerPtr, roomPtr, room, $"id-{room}");
        OnRoomCreate(room);
    }
    private void PlayerAdd(int room, string pid)
    {
        qnetbehaviour.LogInfo($"Player {pid} added to Room {room}");
        OnPlayerAdd(room, pid);
    }
    private void PlayerRemove(int room, string pid)
    {
        qnetbehaviour.LogInfo($"Player {pid} removed from Room {room}");
        OnPlayerRemove(room, pid);
    }
    private void RoomMessage(int room, string pid, string msg)
    {
        qnetbehaviour.LogInfo($"Message in Room {room} from {pid}: {msg}");
        OnRoomMessage(room, pid, msg);
    }
    private void RoomStarted(int room)
    {
        qnetbehaviour.LogInfo($"Room {room} Started");
        OnRoomStarted(room);
    }
    private void RoomEnd(int room)
    {
        qnetbehaviour.LogInfo($"Room {room} Ended");
        OnRoomEnd(room);
    }
    private void RoomDestroy(int room)
    {
        qnetbehaviour.LogInfo($"Room {room} Destroyed");
        var rootName = $"id-{room}";
        UnloadPhysicsRoomScene(rootName);
        GameObject obj = GameObject.Find(rootName);
        if (obj != null)
        {
            Destroy(obj);
            qnetbehaviour.LogInfo($"Destroyed object: {rootName}");
        }
        else
        {
            Debug.LogWarning($"No GameObject found with name: {rootName}");
        }
        OnRoomDestroy(room);
    }
    private void RoomCountDownStarted(int room, int cnt, int max)
    {
        qnetbehaviour.LogInfo($"Countdown: {cnt}/{max} in Room {room}");
        OnRoomCountDownStarted(room, cnt, max);
    }
    private void RoomCountDownCancelled(int room)
    {
        qnetbehaviour.LogInfo($"Countdown Cancelled in Room {room}");
        OnRoomCountDownCancelled(room);
    }

    // interface functions, derived classes can get events through this if implemented
    public virtual void OnLoadTemplateGameScene(){}
    public virtual void OnInitLifeCycle(){}
    public virtual void OnServerPreStart(){}
    public virtual void OnServerStarted(){}
    public virtual void OnServerStoped(){}
    public virtual void OnServerError(int err){}
    public virtual void OnRoomCreate(int room){}
    public virtual void OnPlayerAdd(int room, string pid){}
    public virtual void OnPlayerRemove(int room, string pid){}
    public virtual void OnRoomMessage(int room, string pid, string msg){}
    public virtual void OnRoomStarted(int room){}
    public virtual void OnRoomEnd(int room){}
    public virtual void OnRoomDestroy(int room){}
    public virtual void OnRoomCountDownStarted(int room, int cnt, int max){}
    public virtual void OnRoomCountDownCancelled(int room){}
    
    // network object callbacks
    public virtual void OnSpawn(qnetbehaviour networkObject){}
    public virtual void OnDeSpawn(qnetbehaviour networkObject){}
}
