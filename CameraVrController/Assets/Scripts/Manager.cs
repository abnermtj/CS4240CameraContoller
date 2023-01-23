using System.Collections;
using UnityEngine;

public class Manager : MonoBehaviour
{
    private InputController inputController;
    private bool waitState = true;
    public GameObject cube;

    private const string ip = "192.168.86.26";
    private const int port = 80;

    private void Start()
    {
        //This will do the network stuff
        inputController = new InputController();
        inputController.Begin(ip, port);
    }

    private void Update()
    {
        /*        if (waitState)
                {
                    waitState = false;
        */
        Signal();
    }

    private int prevButtonValue = 3;

    public void Signal()
    {
        //Debug.Log(inputController.CurrentValue & 1);

        if ((inputController.CurrentValue & 1) != prevButtonValue)
        {
            if ((inputController.CurrentValue & 1) == 1)
            {
                //Debug.Log("Button Not Pressed");
                cube.GetComponent<Renderer>().materials[0].color = Color.black;
            }
            else
            {
                //Debug.Log("Button Pressed");
                cube.GetComponent<Renderer>().materials[0].color = Color.blue;
            }
            prevButtonValue = inputController.CurrentValue & 1;
        }

        if ((inputController.CurrentValue & 6) == 4)
        {
            Debug.Log("Encoder Clockwise Rot");
            cube.GetComponent<Renderer>().materials[0].color += new Color(0, 0.04f, .04f, 0);
        }
        else if ((inputController.CurrentValue & 6) == 2)
        {
            Debug.Log("Encoder Anti Clockswise Rot");
            cube.GetComponent<Renderer>().materials[0].color -= new Color(0, 0.04f, 0.04f, 0);
        }

        //StartCoroutine(Wait());
    }

    /*    private IEnumerator Wait()
        {
            yield return new WaitForSeconds(2);
            waitState = true;
        }
    */
}