using System.Collections;
using System.Collections.Generic;
using UnityEngine;

// public class mpinitialiser : MonoBehaviour
// {
//     // Start is called before the first frame update
//     void Start()
//     {
//         
//     }
//
//     // Update is called once per frame
//     void Update()
//     {
//         
//     }
// }

using MessagePack;
using MessagePack.Resolvers;
using UnityEngine.Scripting;

public static class MessagePackInitializer
{
    // [Preserve]
    // [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.BeforeSceneLoad)]
    public static void Init()
    {
        // Compose the resolver with your needed priorities
        var resolver = CompositeResolver.Create(
            ContractlessStandardResolver.Instance,
            BuiltinResolver.Instance,
            StandardResolverAllowPrivate.Instance
        );
        var options = MessagePackSerializerOptions.Standard
            .WithResolver(resolver)
            .WithCompression(MessagePackCompression.Lz4BlockArray); // Optional

        MessagePackSerializer.DefaultOptions = options;
        Debug.Log($"MessagePackInitializer Init");
    }
}
