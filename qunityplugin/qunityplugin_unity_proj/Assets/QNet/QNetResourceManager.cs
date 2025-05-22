using System.Collections.Generic;
using UnityEngine;
using qsdk;

public class QNetResourceManager : MonoBehaviour
{
    public static QNetResourceManager Instance { get; private set; }

    [Header("Register all custom network prefabs")]
    [SerializeField]
    private List<GameObject> networkPrefabs = new List<GameObject>();
    private Dictionary<uint, GameObject> prefabHashMap = new Dictionary<uint, GameObject>();

    void Awake()
    {
        if (Instance != null && Instance != this)
        {
            Destroy(this.gameObject);
            return;
        }

        Instance = this;
        DontDestroyOnLoad(this.gameObject);

        RegisterPrefabs();
    }

    private void RegisterPrefabs()
    {
        foreach (var prefab in networkPrefabs)
        {
            RegisterPrefab(prefab);
        }
    }

    // Dynamically add a prefab
    public void AddPrefab(GameObject prefab)
    {
        if (!networkPrefabs.Contains(prefab))
        {
            networkPrefabs.Add(prefab);
            RegisterPrefab(prefab);
        }
    }

    // Dynamically remove a prefab
    public void RemovePrefab(GameObject prefab)
    {
        if (networkPrefabs.Contains(prefab))
        {
            networkPrefabs.Remove(prefab);
            uint hash = prefab.GetComponent<QNetObject>()._prefabHash;
            prefabHashMap.Remove(hash);
        }
    }

    // Helper method to register a single prefab
    private void RegisterPrefab(GameObject prefab)
    {
        var netObj = prefab.GetComponent<QNetObject>(); // your custom component
        if (netObj == null)
        {
            qnetbehaviour.LogWarn($"{prefab.name} missing QNetObject component.");
            return;
        }

        uint hash = netObj._prefabHash;
        if (!prefabHashMap.ContainsKey(hash))
        {
            prefabHashMap.Add(hash, prefab);
        }
        else
        {
            qnetbehaviour.LogWarn($"Duplicate prefab hash detected: {hash}");
        }
    }

    public GameObject GetPrefabByHash(uint hash)
    {
        prefabHashMap.TryGetValue(hash, out GameObject prefab);
        return prefab;
    }
}
