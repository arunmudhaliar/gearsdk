using System;
using System.Collections;
using System.Collections.Generic;
using ServerPlugin;
using UnityEngine;
using UnityEngine.SceneManagement;

public class ServerLifeCycle : QNetServerLifeCycle
{
    private QServerInputConfig config;
    
    // Start is called before the first frame update
    void Start()
    {
        InitLifeCycle("gsdk-network-scene");
    }
    
    public override void OnInitLifeCycle()
    {
        config = new QServerInputConfig
        {
            ServerAddress = "127.0.0.1:4000",
            RedisAddress = "3.109.144.159:6379",
            ZkUri = "3.109.144.159:2181",
            RootDir = ".",
            InfFile = "./serverconfig.rel.inf",
            AppId = "myapp"
        };
        StartGameServer(config);
    }

    /*
    public override void OnServerPreStart()
    {
    }
    
    public override void OnServerStarted()
    {
    }

    public override void OnServerStoped()
    {
    }

    public override void OnServerError(int err)
    {
    }

    public override void OnRoomCreate(int room)
    {
    }

    public override void OnPlayerAdd(int room, string pid)
    {
    }
    
    public override void OnPlayerRemove(int room, string pid)
    {
    }
    
    public override void OnRoomMessage(int room, string pid, string msg)
    {
    }
    public override void OnRoomStarted(int room)
    {
    }
    
    public override void OnRoomEnd(int room)
    {
    }

    public override void OnRoomDestroy(int room)
    {
    }

    public override void OnRoomCountDownStarted(int room, int cnt, int max)
    {
    }
    
    public override void OnRoomCountDownCancelled(int room)
    {
    }
    */
}
