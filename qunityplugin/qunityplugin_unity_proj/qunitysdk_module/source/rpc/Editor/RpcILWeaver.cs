using System;
using System.IO;
using System.Linq;
using Mono.Cecil;
using Mono.Cecil.Cil;
using UnityEditor;
using UnityEditor.Compilation;
using UnityEngine;
using System.Collections.Generic;

[InitializeOnLoad]
public class RpcILWeaver
{
    // private static int methodId = 1;
	public static void LogInfo(string format, params object[] args)
	{
		Debug.Log($"[RpcILWeaver]: {string.Format(format, args)}");
	}
	public static void LogError(string format, params object[] args)
	{
		Debug.LogError($"[RpcILWeaver]: {string.Format(format, args)}");
	}
	public static void LogWarn(string format, params object[] args)
	{
		Debug.LogWarning($"[RpcILWeaver]: {string.Format(format, args)}");
	}
	static RpcILWeaver()
	{
		CompilationPipeline.assemblyCompilationFinished += OnAssemblyCompiled;
	}

	private static void OnAssemblyCompiled(string assemblyPath, CompilerMessage[] messages)
	{
		if (!assemblyPath.EndsWith("Assembly-CSharp.dll")) return;

		ModifyAssembly(assemblyPath);
	}

	private static void ModifyAssembly(string assemblyPath)
	{
		// methodId = 1;
		string backupPath = assemblyPath + ".bak";
		File.Copy(assemblyPath, backupPath, true); // Backup original DLL

		var resolver = new DefaultAssemblyResolver();
		resolver.AddSearchDirectory(Path.GetDirectoryName(assemblyPath));

		using (var assembly = AssemblyDefinition.ReadAssembly(assemblyPath, new ReaderParameters { ReadWrite = true, AssemblyResolver = resolver }))
		{
			bool methodEnumCreated = false;
			TypeDefinition rpcMethodIdEnum = null;
			Dictionary<MethodDefinition, MethodDefinition> rewriteMethodRefMap =
				new Dictionary<MethodDefinition, MethodDefinition>();
			List<MethodDefinition> newLambdas = new List<MethodDefinition>();
			foreach (var module in assembly.Modules)
			{
				LogInfo(module.Name);
				if (!methodEnumCreated)
				{
					// rpcMethodIdEnum = GenerateRpcMethodIdEnum(module);
					methodEnumCreated = true;
				}

				foreach (var type in module.Types)
				{
					ProcessClass(type, rpcMethodIdEnum, module, rewriteMethodRefMap, newLambdas);
				}

				// rewrite method refrences
				foreach (var type in module.Types)
				{
					foreach (var m in rewriteMethodRefMap)
					{
						MethodReference originalMethodRef = module.ImportReference(m.Key);
						MethodReference newMethodRef = module.ImportReference(m.Value);
						bool modified = false;
						RewriteCalls(type, originalMethodRef, newMethodRef, m.Value, newLambdas, ref modified);
					}
				}
			}

			assembly.Write();
		}

		Debug.Log("✅ RPC IL Weaving complete: " + assemblyPath);
	}

	private static void ProcessClass(TypeDefinition type, TypeDefinition rpcMethodIdEnum, ModuleDefinition module, 
		Dictionary<MethodDefinition, MethodDefinition> rewriteMethodRefMap, List<MethodDefinition> newLambdas)
	{
		bool processInitialisation = true;
		foreach (var method in type.Methods.Where(m => m.HasCustomAttributes).ToList())
		{
			var rpcAttribute = method.CustomAttributes.FirstOrDefault(attr =>
				attr.AttributeType.Name == "ServerRPCAttribute" ||
				attr.AttributeType.Name == "ClientRPCAttribute" ||
				attr.AttributeType.Name == "BroadcastRPCAttribute");

			if (rpcAttribute == null)
			{
				continue;
			}

			FieldDefinition registryField = InjectRpcRegistryVariable(type);
			FieldDefinition netorkHandlerField = InjectRPCNetworkHandlerVariable(type);
			if (processInitialisation)
			{
				List<MethodDefinition> lambdas = InjectRPCRegistrationMethods(type, module, registryField);
				foreach (var l in lambdas)
				{
					newLambdas.Add(l);
				}
				processInitialisation = false;
			}
			var newMethod = InjectRpcWrapper(type, method, module);
			if (newMethod != null)
			{
				rewriteMethodRefMap[method] = newMethod;
			}
		}
	}
	
	private static TypeDefinition Get_qnetbehaviour_BaseClass(TypeDefinition type)
	{
		while (type != null)
		{
			if (type.Name == "qnetbehaviour")
				return type;

			type = type.BaseType?.Resolve(); // Move to the next parent
		}
		return null;
	}
	
	private static void RewriteCalls(TypeDefinition type, MethodReference fromMethod, MethodReference toMethod, 
		MethodDefinition newMethod, List<MethodDefinition> newLambdas, ref bool modified)
	{
		foreach (var method in type.Methods)
		{
			if (!method.HasBody || newMethod == method || newLambdas.Contains(method))
				continue;

			var il = method.Body.Instructions;
			// var debugInfo = method.DebugInformation;
			for (int i = 0; i < il.Count; i++)
			{
				var instr = il[i];

				// Replace all call/callvirt instructions that point to the original method
				if ((instr.OpCode == OpCodes.Call || instr.OpCode == OpCodes.Callvirt) &&
				    instr.Operand is MethodReference methodRef &&
				    methodRef.FullName == fromMethod.FullName)
				{
					// // Try get line number using sequence point
					// SequencePoint sequencePoint = debugInfo.GetSequencePoint(instr);
					// int lineNumber = sequencePoint?.StartLine ?? -1;
					// string fileName = sequencePoint?.Document?.Url ?? "<unknown file>";
					
					instr.Operand = toMethod;
					modified = true;
					LogInfo($"Updating method ref for `{fromMethod.Name}` in `{method.FullName}` +{i}");
				}
			}
		}

		// Recurse into nested types
		foreach (var nested in type.NestedTypes)
		{
			RewriteCalls(nested, fromMethod, toMethod, newMethod, newLambdas, ref modified);
		}
	}

	private static void InjectOwnershipCheck(CustomAttribute rpcAttr, MethodDefinition method, ModuleDefinition module)
	{
		if (rpcAttr == null)
			return;
	
		// Read RequireOwnership = true from attribute
		var requireOwnership = true; // default is true
		var requireOwnershipArg = rpcAttr.Properties.FirstOrDefault(p => p.Name == "RequireOwnership");
		if (requireOwnershipArg.Name != null)
			requireOwnership = (bool)requireOwnershipArg.Argument.Value;
	
		if (!requireOwnership)
			return; // skip injecting if RequireOwnership is false
	
		var ilProcessor = method.Body.GetILProcessor();
		var firstInstruction = method.Body.Instructions[0];
	
		// Find the CachedOwnership property getter method
		var qnetbehaviourBaseType = Get_qnetbehaviour_BaseClass(method.DeclaringType);
		var property = qnetbehaviourBaseType.Properties.FirstOrDefault(p => p.Name == "CachedOwnership");
		if (property == null || property.GetMethod == null)
			throw new Exception("CachedOwnership getter not found");
	
		var getCachedOwnership = module.ImportReference(property.GetMethod);
		var logErrMethod = qnetbehaviourBaseType.Methods.FirstOrDefault(m => m.Name == "LogError");

		// Create IL instructions:
		var continueLabel = ilProcessor.Create(OpCodes.Nop); // label to continue execution
	
		ilProcessor.InsertBefore(firstInstruction, ilProcessor.Create(OpCodes.Ldarg_0)); // this
		ilProcessor.InsertBefore(firstInstruction, ilProcessor.Create(OpCodes.Call, getCachedOwnership)); // call get_CachedOwnership
		ilProcessor.InsertBefore(firstInstruction, ilProcessor.Create(OpCodes.Brtrue_S, continueLabel)); // if true, continue
		// Inject LogInfo("[Ownership Check Failed] Skipping RPC: <methodName>")
		ilProcessor.InsertBefore(firstInstruction, ilProcessor.Create(OpCodes.Ldstr, $"[Ownership Check Failed] Skipping RPC: {method.Name}")); // Load string argument
		ilProcessor.InsertBefore(firstInstruction, ilProcessor.Create(OpCodes.Call, module.ImportReference(logErrMethod))); // Call instance method
		ilProcessor.InsertBefore(firstInstruction, ilProcessor.Create(OpCodes.Ret)); // else return
		ilProcessor.InsertBefore(firstInstruction, continueLabel);
	}
	
	private static MethodDefinition InjectRpcWrapper(TypeDefinition type, MethodDefinition originalMethod, ModuleDefinition module)
	{
	    string originalName = "Original_" + originalMethod.Name;
	    originalMethod.Name = originalName; // Rename original method
	    
	    var newMethod = new MethodDefinition(originalMethod.Name.Replace("Original_", ""), originalMethod.Attributes, originalMethod.ReturnType);
	    foreach (var param in originalMethod.Parameters)
	    {
	        // newMethod.Parameters.Add(new ParameterDefinition(param.ParameterType));
	        newMethod.Parameters.Add(new ParameterDefinition(param.Name, param.Attributes, param.ParameterType));
	    }

	    var ilProcessor = newMethod.Body.GetILProcessor();
	    var qnetbehaviourBaseType = Get_qnetbehaviour_BaseClass(type);
	    var logInfoMethod = qnetbehaviourBaseType.Methods.FirstOrDefault(m => m.Name == "LogInfo");
	    InjectDebugLog(ilProcessor, module, logInfoMethod, $"Invoking modified {newMethod.Name}");
	    
	   FieldDefinition isServerField = null;
	   isServerField = qnetbehaviourBaseType.Fields.FirstOrDefault(f => f.Name == "_isServer");
	   
		// Check if we found the field
		if (isServerField == null)
		   throw new InvalidOperationException("_isServer field not found in any base class hierarchy.");
		
	    var serverAttr = originalMethod.CustomAttributes.FirstOrDefault(attr =>
		    attr.AttributeType.Name == "ServerRPCAttribute");
	    var clientAttr = originalMethod.CustomAttributes.FirstOrDefault(attr =>
		    attr.AttributeType.Name == "ClientRPCAttribute");
	    var broadcastAttr = originalMethod.CustomAttributes.FirstOrDefault(attr =>
		    attr.AttributeType.Name == "BroadcastRPCAttribute");
	    
		bool isServerRPC = serverAttr != null;
		bool isClientRPC = clientAttr != null;
		bool isBroadcastRPC = broadcastAttr != null;
		
		if (serverAttr != null)
		{
			InjectOwnershipCheck(serverAttr, originalMethod, module);
		}
		
		FieldReference importedIsServerField = module.ImportReference(isServerField);
		ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg_0)); 
		ilProcessor.Append(ilProcessor.Create(OpCodes.Ldfld, importedIsServerField)); // if (_isServer)
	    
		// Branch to call Original_ProcessPlayerAction if _isServer is true
		var callOriginal = ilProcessor.Create(OpCodes.Nop);
		
		if (isServerRPC)
		{
			ilProcessor.Append(ilProcessor.Create(OpCodes.Brtrue_S, callOriginal));
		}
		else if (isClientRPC || isBroadcastRPC)
		{
			ilProcessor.Append(ilProcessor.Create(OpCodes.Brfalse_S, callOriginal));
		}
		else
		{
			throw new InvalidOperationException("Invalid attribute !!!");
		}
		
		// Ensure `RPCNetworkHandler` type exists in the assembly
	    TypeReference rpcHandlerType = module.GetType("qsdk.RPCNetworkHandler") ?? 
	                                   throw new InvalidOperationException("RPCNetworkHandler type not found");

	    // Ensure `RpcMethodId` enum type exists
	    TypeReference rpcMethodIdType = module.GetType("GeneratedProxies.RpcMethodId") ??
	                                    throw new InvalidOperationException("RpcMethodId type not found");

	    // Get `RPCNetworkHandler.Instance` (singleton pattern)
	    var instanceGetter = rpcHandlerType.Resolve().Properties.First(p => p.Name == "Instance").GetMethod;
	    ilProcessor.Append(ilProcessor.Create(OpCodes.Call, module.ImportReference(instanceGetter)));

	    // Reference to your custom getter 'CachedMetaInstance'
	    var getCachedMetaInstanceGetter = qnetbehaviourBaseType.Properties
		    .First(p => p.Name == "CachedMetaInstance").GetMethod;
	    var getCachedMetaInstanceGetterRef = module.ImportReference(getCachedMetaInstanceGetter);
	    ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg_0));
	    // Call 'this.CachedMetaInstance'
	    ilProcessor.Append(ilProcessor.Create(OpCodes.Callvirt, getCachedMetaInstanceGetterRef));
	    
	    // Reference to your custom getter 'CachedNetworkObjectId'
	    var getCachedNetworkObjectIdGetter = qnetbehaviourBaseType.Properties
		    .First(p => p.Name == "CachedNetworkObjectId").GetMethod;
	    var getCachedNetworkObjectIdGetterRef = module.ImportReference(getCachedNetworkObjectIdGetter);
	    ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg_0));
	    // Call 'this.CachedNetworkObjectId'
	    ilProcessor.Append(ilProcessor.Create(OpCodes.Callvirt, getCachedNetworkObjectIdGetterRef));

	    
	    // Get the specific enum value for `RpcMethodId.TestScript_ProcessPlayerAction`
	    FieldReference rpcMethodField = rpcMethodIdType.Resolve().Fields.FirstOrDefault(f => f.Name == $"{type.Name}_{newMethod.Name}");
	    if (rpcMethodField == null)
	        throw new InvalidOperationException($"RpcMethodId.{type.Name}_{newMethod.Name} not found");

	    var rpcMethodIdField = rpcMethodIdType.Resolve().Fields.FirstOrDefault(f => f.Name == $"{type.Name}_{newMethod.Name}");
	    int enumValue = (int)rpcMethodIdField.Constant;
	    ilProcessor.Append(ilProcessor.Create(OpCodes.Ldc_I4, enumValue)); // Push enumValue onto the stack
	    
	    // Create object[] array for arguments
	    ilProcessor.Append(ilProcessor.Create(OpCodes.Ldc_I4, originalMethod.Parameters.Count));
	    ilProcessor.Append(ilProcessor.Create(OpCodes.Newarr, module.ImportReference(typeof(object))));
	    
	    for (int i = 0; i < originalMethod.Parameters.Count; i++)
	    {
		    ilProcessor.Append(ilProcessor.Create(OpCodes.Dup)); // Duplicate array reference
		    ilProcessor.Append(ilProcessor.Create(OpCodes.Ldc_I4, i)); // Load index
		    ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg, i + 1)); // Load argument
		    // Check if parameter is value type and box it
		    var paramType = originalMethod.Parameters[i].ParameterType;
		    if (paramType.IsValueType || paramType.IsGenericParameter)
		    {
			    ilProcessor.Append(ilProcessor.Create(OpCodes.Box, module.ImportReference(paramType)));
		    }
		    ilProcessor.Append(ilProcessor.Create(OpCodes.Stelem_Ref)); // Store argument in array
	    }
	    
	    if (isServerRPC || isClientRPC)
	    {
		    // Call `SendRPC(QNetMetaInstance qnetMeta, ulong networkObjectId, RpcMethodId methodId, object[] parameters)`
		    MethodReference sendRpcMethod = rpcHandlerType.Resolve().Methods
			    .FirstOrDefault(m => m.Name == "SendRPC" && m.Parameters.Count == 4);
		    if (sendRpcMethod == null)
			    throw new InvalidOperationException("SendRPC method not found in RPCNetworkHandler");

		    ilProcessor.Append(ilProcessor.Create(OpCodes.Callvirt, module.ImportReference(sendRpcMethod)));
	    }
	    else
	    {
		    // Call `BroadcastRPC(QNetMetaInstance qnetMeta, ulong networkObjectId, RpcMethodId methodId, object[] parameters)`
		    MethodReference broadcastRpcMethod = rpcHandlerType.Resolve().Methods
			    .FirstOrDefault(m => m.Name == "BroadcastRPC" && m.Parameters.Count == 4);
		    if (broadcastRpcMethod == null)
			    throw new InvalidOperationException("BroadcastRPC method not found in RPCNetworkHandler");

		    ilProcessor.Append(ilProcessor.Create(OpCodes.Callvirt, module.ImportReference(broadcastRpcMethod)));
	    }

	    ilProcessor.Append(ilProcessor.Create(OpCodes.Ret)); // Return after RPC call

	    // If `_isServer`: Call `Original_ProcessPlayerAction`
	    if (!isBroadcastRPC)
	    {
		    ilProcessor.Append(callOriginal);
		    ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg_0));
		    for (int i = 0; i < originalMethod.Parameters.Count; i++)
		    {
			    ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg, i + 1));
		    }

		    ilProcessor.Append(ilProcessor.Create(OpCodes.Call, originalMethod));
	    }
	    else
	    {
		    ilProcessor.Append(callOriginal);
		    InjectDebugLog(ilProcessor, module, logInfoMethod, $"{newMethod.Name}, can't be called by client !!!");
	    }
		
	    ilProcessor.Append(ilProcessor.Create(OpCodes.Ret));

	    type.Methods.Add(newMethod);
	    return newMethod;
	}

	public static void InjectDebugLog(ILProcessor ilProcessor, ModuleDefinition module, MethodDefinition logInfoMethod, string message)
	{
		if (logInfoMethod == null)
		{
			LogWarn("logInfoMethod == null !!!, ignoring ...");
			return;
		}
		// ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg_0)); // Load 'this' reference
		ilProcessor.Append(ilProcessor.Create(OpCodes.Ldstr, message)); // Load string argument
		// ilProcessor.Append(ilProcessor.Create(OpCodes.Callvirt, module.ImportReference(logInfoMethod))); // Call instance method
		ilProcessor.Append(ilProcessor.Create(OpCodes.Call, module.ImportReference(logInfoMethod))); // Call instance method
	}
	
	private static List<MethodDefinition> InjectRPCRegistrationMethods(TypeDefinition type, ModuleDefinition module, FieldDefinition rpcRegistryField)
	{
		// Find the RegisterMethod method inside RpcMethodRegistry
		if (type.BaseType == null || type.BaseType.Name != "qnetbehaviour")
		{
			var baseTypeStr = type.BaseType != null ? type.BaseType.Name : "";
			throw new InvalidOperationException($"Invalid baseclass `{baseTypeStr}` found in {type.Name}");
		}
		TypeDefinition baseTypeDef = Get_qnetbehaviour_BaseClass(type);
		
	    var registerMethod = new MethodDefinition("RegisterRPCMethods",
	        MethodAttributes.Public, module.TypeSystem.Void);
	    
	    var ilProcessor = registerMethod.Body.GetILProcessor();
	    var logInfoMethod = baseTypeDef.Methods.FirstOrDefault(m => m.Name == "LogInfo");
	    InjectDebugLog(ilProcessor, module, logInfoMethod, $"{registerMethod.Name}");

	    var registerMethodRef = GetRegisterMethodReference(module); // Ensure correct reference
	    // Find RpcMethodId enum dynamically
	    var rpcMethodIdEnum = module.Types.FirstOrDefault(t => t.Name == "RpcMethodId");
	    if (rpcMethodIdEnum == null)
	        throw new Exception("RpcMethodId enum not found in the module.");

	    // Reference to your custom getter 'CachedNetworkObjectId'
	    var getCachedNetworkObjectIdGetter = baseTypeDef.Properties
		    .First(p => p.Name == "CachedNetworkObjectId").GetMethod;
	    var getCachedNetworkObjectIdGetterRef = module.ImportReference(getCachedNetworkObjectIdGetter);
	    
	    var generatedMethods = new List<MethodDefinition>();
	    foreach (var method in type.Methods.Where(m => m.HasCustomAttributes))
	    {
	        var rpcAttribute = method.CustomAttributes.FirstOrDefault(attr =>
	            attr.AttributeType.Name == "ServerRPCAttribute" ||
	            attr.AttributeType.Name == "ClientRPCAttribute" ||
	            attr.AttributeType.Name == "BroadcastRPCAttribute");
	        if (rpcAttribute == null) continue;

	        // Find the corresponding enum field dynamically
	        var rpcMethodIdField = rpcMethodIdEnum.Fields.FirstOrDefault(f => f.Name == $"{type.Name}_{method.Name}");
	        
	        if (rpcMethodIdField == null)
	            throw new Exception($"Enum value RpcMethodId.{type.Name}_{method.Name} not found.");
	        
	        LogInfo($"Register method --> [{type.Name}] `{rpcMethodIdField.Name} = {(int)rpcMethodIdField.Constant}`");
			// Step 1: Load `_rpcRegistry`
			ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg_0));
			ilProcessor.Append(ilProcessor.Create(OpCodes.Ldfld, rpcRegistryField));

			// Call 'this.CachedNetworkObjectId'
			ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg_0));
			ilProcessor.Append(ilProcessor.Create(OpCodes.Callvirt, getCachedNetworkObjectIdGetterRef));
			
			// Step 2: Load dynamically found enum value
			int enumValue = (int)rpcMethodIdField.Constant;
			ilProcessor.Append(ilProcessor.Create(OpCodes.Ldc_I4, enumValue)); // Push enumValue onto the stack

			// Step 3: Create inline delegate (args => this.Original_ProcessPlayerAction((string)args[0], (int)args[1]))

			// Step 3.1: Create new `Action<object[]>` delegate
			// var delegateType = module.ImportReference(typeof(Action<object[]>));
			var delegateCtor = module.ImportReference(
			    typeof(Action<object[]>).GetConstructor(new[] { typeof(object), typeof(IntPtr) })
			);

			// Step 3.2: Define instance method (not static!)
			var lambdaMethod = new MethodDefinition($"Lambda_{method.Name}",
			    MethodAttributes.Private | MethodAttributes.HideBySig, // ✅ Remove 'Static' attribute
			    module.TypeSystem.Void
			);

			// Step 3.3: Add `object[] args` parameter
			var argsParam = new ParameterDefinition("args", ParameterAttributes.None, 
			    module.ImportReference(typeof(object[]))
			);
			lambdaMethod.Parameters.Add(argsParam);
			generatedMethods.Add(lambdaMethod); // Add method to type

			// Step 3.4: Generate method body (✅ No casting this reference!)
			var methodIL = lambdaMethod.Body.GetILProcessor();
			InjectDebugLog(methodIL, module, logInfoMethod, $"Invoking {lambdaMethod.Name}");
			methodIL.Append(methodIL.Create(OpCodes.Nop));
			methodIL.Append(methodIL.Create(OpCodes.Ldarg_0));  // ✅ Load `this` (not casted!)

			// Unpack each argument dynamically based on known parameter types
			for (int i = 0; i < method.Parameters.Count; i++)
			{
				methodIL.Append(methodIL.Create(OpCodes.Ldarg_1)); // Load args[]
				methodIL.Append(methodIL.Create(OpCodes.Ldc_I4, i)); // Index
				methodIL.Append(methodIL.Create(OpCodes.Ldelem_Ref)); // Load args[i]
			    
			    // Cast to the correct type
			    var paramType = method.Parameters[i].ParameterType;
			    if (paramType.IsValueType)
			    {
				    methodIL.Append(methodIL.Create(OpCodes.Unbox_Any, module.ImportReference(paramType))); // ✅ Correct for value types
			    }
			    else
			    {
				    methodIL.Append(methodIL.Create(OpCodes.Castclass, module.ImportReference(paramType))); // ✅ Keep cast for reference types
			    }
			}
			methodIL.Append(methodIL.Create(OpCodes.Call, method));
			methodIL.Append(methodIL.Create(OpCodes.Nop));
			
			methodIL.Append(methodIL.Create(OpCodes.Nop));
			methodIL.Append(methodIL.Create(OpCodes.Ret));

			// Step 3.5: Create delegate instance (✅ Use instance method)
			ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg_0)); // ✅ Load `this`
			ilProcessor.Append(ilProcessor.Create(OpCodes.Ldftn, lambdaMethod)); // Load function pointer
			ilProcessor.Append(ilProcessor.Create(OpCodes.Newobj, delegateCtor)); // Create Action<object[]>

			// Step 4: Call `_rpcRegistry.RegisterMethod`
			ilProcessor.Append(ilProcessor.Create(OpCodes.Callvirt, registerMethodRef));
	    }

	    ilProcessor.Append(ilProcessor.Create(OpCodes.Ret));
	    type.Methods.Add(registerMethod);

	    foreach (var m in generatedMethods)
	    {
		    type.Methods.Add(m);
	    }
	    
	    // Generate UnRegisterRPCMethods()
	    var unregisterMethod = new MethodDefinition("UnRegisterRPCMethods",
	        MethodAttributes.Public, module.TypeSystem.Void);
	    
	    ilProcessor = unregisterMethod.Body.GetILProcessor();
	    InjectDebugLog(ilProcessor, module, logInfoMethod, $"{unregisterMethod.Name}");
	    var unregisterMethodRef = GetUnRegisterMethodReference(module); // Ensure correct reference

	    foreach (var method in type.Methods.Where(m => m.HasCustomAttributes))
	    {
	        var rpcAttribute = method.CustomAttributes.FirstOrDefault(attr =>
	            attr.AttributeType.Name == "ServerRPCAttribute" ||
	            attr.AttributeType.Name == "ClientRPCAttribute" ||
	            attr.AttributeType.Name == "BroadcastRPCAttribute");
	        if (rpcAttribute == null) continue;

	        // Find the corresponding enum field dynamically
	        var rpcMethodIdField = rpcMethodIdEnum.Fields.FirstOrDefault(f => f.Name == $"{type.Name}_{method.Name}");
	        if (rpcMethodIdField == null)
	            throw new Exception($"Enum value RpcMethodId.{type.Name}_{method.Name} not found.");
	        
	        // LogInfo($"enum check u : {rpcMethodIdField.Name} = {(int)rpcMethodIdField.Constant}");
	        ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg_0));
	        ilProcessor.Append(ilProcessor.Create(OpCodes.Ldfld, rpcRegistryField)); // Load _rpcRegistry

	        // Call 'this.CachedNetworkObjectId'
	        ilProcessor.Append(ilProcessor.Create(OpCodes.Ldarg_0));
	        ilProcessor.Append(ilProcessor.Create(OpCodes.Callvirt, getCachedNetworkObjectIdGetterRef));
	        int enumValue = (int)rpcMethodIdField.Constant;
	        ilProcessor.Append(ilProcessor.Create(OpCodes.Ldc_I4, enumValue)); // Push enumValue onto the stack
	        ilProcessor.Append(ilProcessor.Create(OpCodes.Callvirt, unregisterMethodRef));
	    }

	    ilProcessor.Append(ilProcessor.Create(OpCodes.Ret));
	    type.Methods.Add(unregisterMethod);
	    return generatedMethods;
	}

	private static TypeDefinition Get_RpcMethodRegistry(TypeDefinition type)
	{
		var registryType = type.Module.Types.FirstOrDefault(t =>
			t.Namespace == "qsdk" && t.Name == "RpcMethodRegistry");
		return registryType;
	}
	
	private static TypeDefinition Get_RPCNetworkHandler(TypeDefinition type)
	{
		var networkHandlerType = type.Module.Types.FirstOrDefault(t =>
			t.Namespace == "qsdk" && t.Name == "RPCNetworkHandler");
		return networkHandlerType;
	}
	
	private static MethodReference GetRegisterMethodReference(ModuleDefinition module)
	{
		// Find the RpcMethodRegistry type in the generated module
		var rpcRegistryType = module.Types.FirstOrDefault(t => t.FullName == "qsdk.RpcMethodRegistry");
    
		if (rpcRegistryType == null)
			throw new InvalidOperationException("RpcMethodRegistry not found in the module");

		// Find the RegisterMethod method inside RpcMethodRegistry
		var registerMethod = rpcRegistryType.Methods.FirstOrDefault(m => m.Name == "RegisterMethod");

		if (registerMethod == null)
			throw new InvalidOperationException("RegisterMethod not found in RpcMethodRegistry");

		// Import the method reference into the module
		return module.ImportReference(registerMethod);
	}

	private static MethodReference GetUnRegisterMethodReference(ModuleDefinition module)
	{
		// Find the RpcMethodRegistry type in the generated module
		var rpcRegistryType = module.Types.FirstOrDefault(t => t.FullName == "qsdk.RpcMethodRegistry");
    
		if (rpcRegistryType == null)
			throw new InvalidOperationException("RpcMethodRegistry not found in the module");

		// Find the RegisterMethod method inside RpcMethodRegistry
		var registerMethod = rpcRegistryType.Methods.FirstOrDefault(m => m.Name == "UnregisterMethod");

		if (registerMethod == null)
			throw new InvalidOperationException("UnregisterMethod not found in RpcMethodRegistry");

		// Import the method reference into the module
		return module.ImportReference(registerMethod);
	}
	
	private static FieldDefinition InjectRpcRegistryVariable(TypeDefinition type)
	{
		TypeDefinition baseTypeDef = Get_qnetbehaviour_BaseClass(type);
		var existingField = baseTypeDef.Fields.FirstOrDefault(f => f.Name == "_rpcRegistry");
		if (existingField != null) 
			return existingField;
		
		// check on the current type
		existingField = type.Fields.FirstOrDefault(f => f.Name == "_rpcRegistry");
		if (existingField != null) 
			return existingField;
		
		// 🔹 Find RpcMethodRegistry inside the 'GeneratedProxies' namespace
		var registryType = Get_RpcMethodRegistry(type);
		if (registryType == null)
		{
			LogError("RpcMethodRegistry not found (source generator issue)");
			return null; // ❌ RpcMethodRegistry not found (source generator issue)
		}

		var registryField = new FieldDefinition("_rpcRegistry",
			FieldAttributes.Family,
			// FieldAttributes.Private | FieldAttributes.InitOnly,
			registryType);

		type.Fields.Add(registryField);

		var constructor = type.Methods.FirstOrDefault(m => m.IsConstructor && !m.IsStatic);
		if (constructor == null) return registryField;

		var il = constructor.Body.GetILProcessor();
		var firstInstruction = constructor.Body.Instructions.First();

		// 🔹 Find the 'Instance' property inside RpcMethodRegistry
		var instanceProperty = registryType.Properties.FirstOrDefault(p => p.Name == "Instance");
		if (instanceProperty == null) return registryField;

		// 🔹 Inject `_rpcRegistry = RpcMethodRegistry.Instance;`
		il.InsertBefore(firstInstruction, il.Create(OpCodes.Ldarg_0));
		il.InsertBefore(firstInstruction, il.Create(OpCodes.Call, instanceProperty.GetMethod));
		il.InsertBefore(firstInstruction, il.Create(OpCodes.Stfld, registryField));
		LogInfo($"✅ injected _rpcRegistry for: {type.Name}");
		return registryField;
	}
	
	private static FieldDefinition InjectRPCNetworkHandlerVariable(TypeDefinition type)
	{
		TypeDefinition baseTypeDef = Get_qnetbehaviour_BaseClass(type);
		
		var existingField = baseTypeDef.Fields.FirstOrDefault(f => f.Name == "_rpcNetworkHandler");
		if (existingField != null) 
			return existingField;
		
		// check on the current type
		existingField = type.Fields.FirstOrDefault(f => f.Name == "_rpcNetworkHandler");
		if (existingField != null) 
			return existingField;
		
		// 🔹 Find RPCNetworkHandler inside the 'GeneratedProxies' namespace
		var rpcNetworkHandlerType = Get_RPCNetworkHandler(type);
		if (rpcNetworkHandlerType == null)
		{
			LogError("RPCNetworkHandler not found (source generator issue)");
			return null; // ❌ RPCNetworkHandler not found (source generator issue)
		}

		var rpcNetworkHandlerField = new FieldDefinition("_rpcNetworkHandler",
			// FieldAttributes.Private | FieldAttributes.InitOnly,
			FieldAttributes.Family,
			rpcNetworkHandlerType);

		type.Fields.Add(rpcNetworkHandlerField);

		// var constructor = type.Methods.FirstOrDefault(m => m.IsConstructor && !m.IsStatic);
		// if (constructor == null) return rpcNetworkHandlerField;
		//
		// var il = constructor.Body.GetILProcessor();
		// var firstInstruction = constructor.Body.Instructions.First();
		//
		// // 🔹 Find the 'Instance' property inside RPCNetworkHandler
		// var instanceProperty = rpcNetworkHandlerType.Properties.FirstOrDefault(p => p.Name == "Instance");
		// if (instanceProperty == null) return rpcNetworkHandlerField;
		//
		// // 🔹 Inject `_rpcNetworkHandler = RPCNetworkHandler.Instance;`
		// il.InsertBefore(firstInstruction, il.Create(OpCodes.Ldarg_0));
		// il.InsertBefore(firstInstruction, il.Create(OpCodes.Call, instanceProperty.GetMethod));
		// il.InsertBefore(firstInstruction, il.Create(OpCodes.Stfld, rpcNetworkHandlerField));
		LogInfo($"✅ injected _rpcNetworkHandler for: {type.Name}");
		return rpcNetworkHandlerField;
	}
	
	private static TypeDefinition GenerateRpcMethodIdEnum(ModuleDefinition module)
	{
		const string namespaceName = "GeneratedProxies";
		const string enumName = "RpcMethodId";

		// 🔹 Check if the enum already exists, and remove it if needed
		var existingEnum = module.Types.FirstOrDefault(t => t.Name == enumName);
		if (existingEnum != null)
		{
			LogWarn("RpcMethodId already exists, clearing !!!");
			// module.Types.Remove(existingEnum);
			// // 🔹 Iterate through fields and clear constant values
			// foreach (var field in existingEnum.Fields)
			// {
			// 	if (field.HasConstant)
			// 	{
			// 		LogInfo($"enum --> {field.Name}");
			// 		field.Constant = null; // ✅ Remove constant value
			// 	}
			// }
			existingEnum.Fields.Clear();
			return existingEnum; // Return the existing enum after clearing constants
		}

		// 🔹 Create a new Enum definition
		var enumType = new TypeDefinition(namespaceName, enumName,
			TypeAttributes.Public | TypeAttributes.Sealed | TypeAttributes.BeforeFieldInit,
			module.ImportReference(typeof(Enum)));

		module.Types.Add(enumType);

		// 🔹 Add underlying field for enum (int32)
		var enumBackingField = new FieldDefinition("value__",
			FieldAttributes.Private | FieldAttributes.SpecialName | FieldAttributes.RTSpecialName,
			module.ImportReference(typeof(int)));

		enumType.Fields.Add(enumBackingField);
		return enumType;
	}
}