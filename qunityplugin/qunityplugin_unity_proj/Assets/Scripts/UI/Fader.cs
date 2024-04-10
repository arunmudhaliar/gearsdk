using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Fader : MonoBehaviour
{
    float fadeTime = 0.0f;
    public float maxFadeTime = 2.0f;
    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        fadeTime -= Time.deltaTime;
        if (fadeTime<=0) {
            fadeTime = 0.0f;
            this.gameObject.SetActive(false);
        }
    }

    private void OnEnable() {
        fadeTime = maxFadeTime;
    }
}
