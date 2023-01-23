using UnityEngine;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System;

public class InputController
{
    public int CurrentValue = 2;

    public void Begin(string ipAddress, int port)
    {
        var thread = new Thread(() =>
        {
            try
            {
                while (true)
                {
                    TcpClient socketConnection = new TcpClient(ipAddress, port);
                    Byte[] bytes = new Byte[1024];
                    // Get a stream object for reading
                    using (NetworkStream stream = socketConnection.GetStream())
                    {
                        int length;
                        // Read incomming stream into byte arrary.
                        while ((length = stream.Read(bytes, 0, bytes.Length)) != 0)
                        {
                            var incommingData = new byte[length];
                            Array.Copy(bytes, 0, incommingData, 0, length);
                            // Convert byte array to string message.
                            string serverMessage = Encoding.ASCII.GetString(incommingData);

                            if (serverMessage != "\r")
                            {
                                CurrentValue = int.Parse(serverMessage);
                                Debug.Log("server message received as: " + serverMessage);
                            }
                        }
                    }
                }
            }
            catch (SocketException socketException)
            {
                Debug.Log("Socket exception: " + socketException);
            }
        });

        thread.Start();
    }
}