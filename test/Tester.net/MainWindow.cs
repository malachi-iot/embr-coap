using System;
using Gtk;

using CoAPNet;
using CoAPNet.Udp;
using System.Threading.Tasks;
using System.Linq;
using System.Text;

public partial class MainWindow : Gtk.Window
{
    public MainWindow() : base(Gtk.WindowType.Toplevel)
    {
        Build();
    }

    protected void OnDeleteEvent(object sender, DeleteEventArgs a)
    {
        Application.Quit();
        a.RetVal = true;
    }

    protected void OnButton2Clicked(object sender, EventArgs e)
    {
        var client = new CoapClient(new CoapUdpEndPoint());

        var requestor = Task.Run(async () =>
        {
            var uri = lstURI.ActiveText;

            Console.WriteLine($"Issuing request: {uri}");

            CoapMessageIdentifier messageId = await client.GetAsync(uri);

            var response = await client.GetResponseAsync(messageId);

            Console.WriteLine("got a response");
            Console.WriteLine(Encoding.UTF8.GetString(response.Payload));
        });

        requestor.ContinueWith(x => 
        {
            if(x.IsFaulted)
            {
                Console.WriteLine($"Got an error: {x.Exception.InnerException.Message}");
            }
        });
    }
}
