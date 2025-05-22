using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;

namespace ServerPlugin
{
    public static class NativeMethods
    {
#if UNITY_EDITOR_OSX
        // const string LIB_NAME = "Assets/Plugins/qunitysdk/rpc/libs/macos/libserverplugin.dylib";
        const string LIB_NAME = "serverplugin";
#elif UNITY_STANDALONE_OSX
        const string LIB_NAME = "serverplugin";
#elif UNITY_IOS || UNITY_IPHONE
        const string LIB_NAME = "__Internal";
#elif UNITY_ANDROID
        const string LIB_NAME = "libserverplugin.so";
#elif UNITY_STANDALONE_LINUX
        const string LIB_NAME = "libserverplugin.so";
#elif UNITY_STANDALONE_WIN
        const string LIB_NAME = "serverplugin";
#endif

        [DllImport(LIB_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern void setup_signal_handler();

        [DllImport(LIB_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern void pre_init_serverplugin_sdk();

        [DllImport(LIB_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern void spawn_qh3router(
            string routerAddress,
            string mongodbUri,
            string redisAddress,
            string zkUri,
            string rootDir,
            string infFile,
            int commandPort,
            int routerPortReturn,
            string appId,
            OnRouterPreStartDelegate onPreStart,
            OnRouterStartDelegate onStart,
            OnRouterStopDelegate onStop,
            OnRouterErrorDelegate onError
        );

        [DllImport(LIB_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern int spawn_game_server(
            string serverAddress,
            string redisAddress,
            string zkUri,
            string rootDir,
            string infFile,
            string appId,
            OnServerPreStartDelegate preStart,
            OnServerStartDelegate start,
            OnServerStopDelegate stop,
            OnServerErrorDelegate error,
            OnRoomEventCreateDelegate roomCreate,
            OnRoomEventStartDelegate roomStart,
            OnRoomEventPlayerAddedDelegate playerAdded,
            OnRoomEventMessageDelegate roomMessage,
            OnRoomEventPlayerRemovedDelegate playerRemoved,
            OnRoomEventEndDelegate roomEnd,
            OnRoomEventDesroyDelegate roomDestroy,
            OnRoomCountdownToStartDelegate countdownStart,
            OnRoomCountdownCancelledDelegate countdownCancelled
        );
        
        [DllImport(LIB_NAME, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool room_broadcast_except(
            IntPtr server,
            IntPtr room,
            uint cidHash,
            byte[] msg,
            uint length);

        [DllImport(LIB_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern void room_broadcast(
            IntPtr server,
            IntPtr room,
            byte[] msg,
            uint length);

        [DllImport(LIB_NAME, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool room_send_to(
            IntPtr server,
            IntPtr room,
            uint cidHash,
            byte[] msg,
            uint length);
    }

    // Delegate types for callback functions
    public delegate void OnRouterPreStartDelegate(IntPtr router, IntPtr cdData);
    public delegate void OnRouterStartDelegate(IntPtr router);
    public delegate void OnRouterStopDelegate(IntPtr router);
    public delegate void OnRouterErrorDelegate(IntPtr router, int errorCode);

    public delegate void OnServerPreStartDelegate(IntPtr server, IntPtr cdData);
    public delegate void OnServerStartDelegate(IntPtr server);
    public delegate void OnServerStopDelegate(IntPtr server);
    public delegate void OnServerErrorDelegate(IntPtr server, int errorCode);

    public delegate void OnRoomEventCreateDelegate(IntPtr nativeServer, int roomId, IntPtr roomPtr);
    public delegate void OnRoomEventStartDelegate(IntPtr nativeServer, int roomId, IntPtr roomPtr);
    public delegate void OnRoomEventPlayerAddedDelegate(IntPtr nativeServer, int roomId, IntPtr roomPtr, string playerId, ulong cidHash);
    public delegate void OnRoomEventMessageDelegate(IntPtr nativeServer, int roomId, IntPtr roomPtr, string playerId, ulong cidHash, ulong recvLen, IntPtr buf);
    public delegate void OnRoomEventPlayerRemovedDelegate(IntPtr nativeServer, int roomId, IntPtr roomPtr, string playerId, ulong cidHash);
    public delegate void OnRoomEventEndDelegate(IntPtr nativeServer, int roomId, IntPtr roomPtr);
    public delegate void OnRoomEventDesroyDelegate(IntPtr nativeServer, int roomId, IntPtr roomPtr);
    public delegate void OnRoomCountdownToStartDelegate(IntPtr nativeServer, int roomId, IntPtr roomPtr, int count, int maxCount);
    public delegate void OnRoomCountdownCancelledDelegate(IntPtr nativeServer, int roomId, IntPtr roomPtr);

    // Configuration classes
    public class Qh3RouterInputConfig
    {
        public string RouterAddress { get; set; }
        public string MongodbUri { get; set; }
        public string MongodbDb { get; set; }
        public string RedisAddress { get; set; }
        public string RedisUser { get; set; }
        public string RedisPassword { get; set; }
        public string ZkUri { get; set; }
        public string RootDir { get; set; }
        public string InfFile { get; set; }
        public int CommandPort { get; set; }
        public int RouterPortReturn { get; set; }
        public string AppId { get; set; }
    }

    public class QServerInputConfig
    {
        public string ServerAddress { get; set; }
        public string RedisAddress { get; set; }
        public string RedisUser { get; set; }
        public string RedisPassword { get; set; }
        public string ZkUri { get; set; }
        public string RootDir { get; set; }
        public string InfFile { get; set; }
        public string AppId { get; set; }
    }

    // Enums for log levels
    public enum LogLevels
    {
        LEVEL_0,
        LEVEL_1,
        LEVEL_2,
        LEVEL_3,
        LEVEL_4
    }

    public enum ELogType
    {
        INFO_LOG,
        DEBUG_LOG,
        WARN_LOG,
        ERROR_LOG,
        LOG_TYPE_MAX
    }

    // Wrapper class for SDK
    public static class ServerSdk
    {
        // Hold references to prevent GC
        private static OnRouterPreStartDelegate _routerPreStart;
        private static OnRouterStartDelegate _routerStart;
        private static OnRouterStopDelegate _routerStop;
        private static OnRouterErrorDelegate _routerError;

        private static OnServerPreStartDelegate _preStart;
        private static OnServerStartDelegate _start;
        private static OnServerStopDelegate _stop;
        private static OnServerErrorDelegate _error;
        private static OnRoomEventCreateDelegate _roomCreate;
        private static OnRoomEventStartDelegate _roomStart;
        private static OnRoomEventPlayerAddedDelegate _playerAdded;
        private static OnRoomEventMessageDelegate _roomMessage;
        private static OnRoomEventPlayerRemovedDelegate _playerRemoved;
        private static OnRoomEventEndDelegate _roomEnd;
        private static OnRoomEventDesroyDelegate _roomDestroy;
        private static OnRoomCountdownToStartDelegate _countdownStart;
        private static OnRoomCountdownCancelledDelegate _countdownCancelled;

        static ServerSdk()
        {
            NativeMethods.setup_signal_handler();
            NativeMethods.pre_init_serverplugin_sdk();
            Debug.Log("server sdk init");
        }

        public static void StartRouter(Qh3RouterInputConfig config,
            OnRouterPreStartDelegate preStart,
            OnRouterStartDelegate start,
            OnRouterStopDelegate stop,
            OnRouterErrorDelegate error)
        {
            _routerPreStart = preStart;
            _routerStart = start;
            _routerStop = stop;
            _routerError = error;

            NativeMethods.spawn_qh3router(
                config.RouterAddress,
                config.MongodbUri,
                config.RedisAddress,
                config.ZkUri,
                config.RootDir,
                config.InfFile,
                config.CommandPort,
                config.RouterPortReturn,
                config.AppId,
                preStart,
                start,
                stop,
                error
            );
        }

        public static int StartGameServer(QServerInputConfig config,
            OnServerPreStartDelegate preStart,
            OnServerStartDelegate start,
            OnServerStopDelegate stop,
            OnServerErrorDelegate error,
            OnRoomEventCreateDelegate roomCreate,
            OnRoomEventStartDelegate roomStart,
            OnRoomEventPlayerAddedDelegate playerAdded,
            OnRoomEventMessageDelegate roomMessage,
            OnRoomEventPlayerRemovedDelegate playerRemoved,
            OnRoomEventEndDelegate roomEnd,
            OnRoomEventDesroyDelegate roomDestroy,
            OnRoomCountdownToStartDelegate countdownStart,
            OnRoomCountdownCancelledDelegate countdownCancelled)
        {
            _preStart = preStart;
            _start = start;
            _stop = stop;
            _error = error;
            _roomCreate = roomCreate;
            _roomStart = roomStart;
            _playerAdded = playerAdded;
            _roomMessage = roomMessage;
            _playerRemoved = playerRemoved;
            _roomEnd = roomEnd;
            _roomDestroy = roomDestroy;
            _countdownStart = countdownStart;
            _countdownCancelled = countdownCancelled;

            return NativeMethods.spawn_game_server(
                config.ServerAddress,
                config.RedisAddress,
                config.ZkUri,
                config.RootDir,
                config.InfFile,
                config.AppId,
                preStart,
                start,
                stop,
                error,
                roomCreate,
                roomStart,
                playerAdded,
                roomMessage,
                playerRemoved,
                roomEnd,
                roomDestroy,
                countdownStart,
                countdownCancelled
            );
        }

        public static bool SendTo(
            IntPtr server,
            IntPtr room,
            uint cidHash,
            byte[] msg,
            uint length)
        {
            return NativeMethods.room_send_to(server, room, cidHash, msg, length);
        }

        public static void BroadcastToClients(
            IntPtr server,
            IntPtr room,
            byte[] msg,
            uint length)
        {
            NativeMethods.room_broadcast(server, room, msg, length);
        }
    }
}
