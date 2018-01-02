using System;
using Gtk;

using CoAPNet;
using CoAPNet.Udp;
using System.Threading.Tasks;
using System.Linq;
using System.Text;
using System.IO;

public partial class MainWindow : Gtk.Window
{
    const string mruuripath = "mruuri.txt";

    public MainWindow() : base(Gtk.WindowType.Toplevel)
    {
        Build();

        // TODO: Do this in GUI Design editor later
        chkConfirmable.Active = true;

        // read in mru URI list, if present
        if (File.Exists(mruuripath))
        {
            var mruuris = File.ReadAllLines(mruuripath);

            //lstURI.a

            foreach(var uri in mruuris)
            {
                if(!string.IsNullOrWhiteSpace(uri))
                    lstURI.AppendText(uri);
            }
        }
    }

    protected void OnDeleteEvent(object sender, DeleteEventArgs a)
    {
        Application.Quit();
        var entries = lstURI.Cast<Entry>();

        using (var writer = new StreamWriter(mruuripath))
        {

            foreach (Entry entry in entries)
            {
                writer.WriteLine(entry.Text);
            }
        }

        a.RetVal = true;
    }

    protected void OnButton2Clicked(object sender, EventArgs e)
    {
        var endpoint = new CoapUdpEndPoint();
        var client = new CoapClient(endpoint);

        var confirmable = chkConfirmable.Active;
        bool coaps = false;

        var uri = (coaps ? "coaps" : "coap") + "://" + lstURI.ActiveText;
        var uriPath = "";
        string uriArgs = null;

        var message = new CoapMessage();

        message.Code = CoapMessageCode.Get;
        message.Type = confirmable ? CoapMessageType.Confirmable : CoapMessageType.NonConfirmable;

        try
        {
            // TODO: This one is more organized for our needs
            //var uri = new Uri("", )
            if (uriArgs != null)
                uriPath += "?" + uriArgs;

            message.SetUri(uri + "/" + uriPath);
            //message.SetUri()
        }
        catch(Exception _e)
        {
            using(MessageDialog md = new MessageDialog(this,
                DialogFlags.DestroyWithParent, MessageType.Info,
               ButtonsType.Close, $"Error: {_e.Message}"))
            {
                md.Run();
                md.Destroy();
                return;
            }
        }

        var requestor = Task.Run(async () =>
        {
            Console.WriteLine($"Issuing request: {uri}");

            await client.SendAsync(message);

            Console.WriteLine($"Phase 1");

            var response = await client.ReceiveAsync();
            //CoapMessageIdentifier messageId = await client.GetAsync(uri);

            //var response = await client.GetResponseAsync(messageId);

            Console.WriteLine("got a response");
            Console.WriteLine(Encoding.UTF8.GetString(response.Message.Payload));
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
