using System;
using System.Reflection;
using UnityEngine;
namespace qsdk
{
	[AttributeUsage(AttributeTargets.Method)]
	public class ServerRPCAttribute : Attribute
	{
		public bool RequireOwnership { get; set; } = true;

		public ServerRPCAttribute() { }

		public ServerRPCAttribute(bool requireOwnership)
		{
			RequireOwnership = requireOwnership;
		}
	}

	[AttributeUsage(AttributeTargets.Method)]
	public class ClientRPCAttribute : Attribute
	{
	}

	[AttributeUsage(AttributeTargets.Method)]
	public class BroadcastRPCAttribute : Attribute
	{
	}
	
	internal interface IQNetNetObjectLifeCycle
	{
		void OnNetworkSpawn(QNetMetaInstance meta);
		void OnNetworkDeSpawn();
		void OnNetworkOwnershipUpdate(string prevNetworkClientID);
	}
	
	public abstract class qnetbehaviour : MonoBehaviour, IQNetNetObjectLifeCycle
	{
		protected readonly RpcMethodRegistry _rpcRegistry = RpcMethodRegistry.Instance;
		protected readonly RPCNetworkHandler _rpcNetworkHandler = RPCNetworkHandler.Instance;
		// variables
		protected bool _isServer = RPCNetworkHandler.Instance.IsServer;
		private ulong _cachedNetworkObjectId = 0;
		private string _cachedNetworkClientID = "";
		private QNetMetaInstance _qnetMeta;
		
		// properties
		public ulong CachedNetworkObjectId => _cachedNetworkObjectId;
		public string CachedNetworkClientId => _cachedNetworkClientID;
		public bool CachedOwnership => IsOwner();
		public QNetMetaInstance CachedMetaInstance
		{
			get { return _qnetMeta; }
			set { _qnetMeta = value; }
		}

		// functions
		public static void LogInfo(string message)
		{
			Debug.Log($"[qsdk]: {message}");
		}
		public static void LogError(string message)
		{
			Debug.LogError($"[qsdk]: {message}");
		}
		public static void LogWarn(string message)
		{
			Debug.LogWarning($"[qsdk]: {message}");
		}
		
		public void Invoke_RegisterRPCMethods()
		{
			// if (_cachedNetworkObjectId == 0)
			// {
			// 	LogWarn("_cachedNetworkObjectId is zero !!!");
			// 	return;
			// }
			
			MethodInfo method = GetType().GetMethod("RegisterRPCMethods", BindingFlags.Public | BindingFlags.Instance);
			if (method == null)
			{
				LogWarn($"Method 'RegisterRPCMethods' not found in class '{GetType().FullName}'!");
				return;
			}

			// Invoke the method on the current instance
			method.Invoke(this, null);

			// Log success (assuming LogInfo is a valid method)
			LogInfo($"Successfully invoked RegisterRPCMethods() on {GetType().FullName}");
		}
		
		protected void Invoke_UnRegisterRPCMethods()
		{
			MethodInfo method = GetType().GetMethod("UnRegisterRPCMethods", BindingFlags.Public | BindingFlags.Instance);
			if (method == null)
			{
				LogWarn($"Method 'UnRegisterRPCMethods' not found in class '{GetType().FullName}'!");
				return;
			}

			// Invoke the method on the current instance
			method.Invoke(this, null);

			// Log success (assuming LogInfo is a valid method)
			LogInfo($"Successfully invoked UnRegisterRPCMethods() on {GetType().FullName}");
		}

		public void SetNetworkObjectId(ulong networkObjectId)
		{
			_cachedNetworkObjectId = networkObjectId;
		}

		public void SetOwnership(string networkClientID)
		{
			_cachedNetworkClientID = networkClientID;
		}

		private bool IsOwner()
		{
			if (string.IsNullOrEmpty(_cachedNetworkClientID))
			{
				LogError($"IsOwner is not valid, since _cachedNetworkClientID is {_cachedNetworkClientID}");
				return false;
			}
			if (_isServer)
			{
				return true;
			}
			return _qnetMeta.clientMeta.networkClientID == _cachedNetworkClientID;
		}
		
		public virtual void OnNetworkSpawn(QNetMetaInstance meta)
		{
			
		}

		public virtual void OnNetworkDeSpawn()
		{
		}
		public virtual void OnNetworkOwnershipUpdate(string prevNetworkClientID)
		{
		}
		private void OnDestroy()
		{
			Invoke_UnRegisterRPCMethods();
		}
	}
}
