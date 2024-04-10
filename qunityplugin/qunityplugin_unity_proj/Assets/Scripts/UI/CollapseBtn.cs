using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class CollapseBtn : MonoBehaviour
{
    Button button;
    public List<GameObject> list = new List<GameObject>();

    // Start is called before the first frame update
    void Start()
    {
        button = this.GetComponent<Button>();
        if (button != null) {
            button.onClick.AddListener(OnClickCollapse);
        }
    }

    void OnClickCollapse() {
        foreach(var g in list) {
            g.SetActive(!g.activeSelf);
        }
    }

    void OnDestroy() {
        if (button != null) {
            button.onClick.RemoveListener(OnClickCollapse);
        }
    }
}
