using System;
using Gtk;

using CoAPNet;
using CoAPNet.Udp;
using CoAPNet.Options;
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

            foreach(var uri in mruuris)
            {
                if (!string.IsNullOrWhiteSpace(uri))
                {
                    lstURI.AppendText(uri);
                    // AppendText doesn't cascade down into listURI.Cast<Entry> and strangely neither does the
                    // following.  I need to learn more about GtkSharp evidently
                    // lstURI.Add(new Entry(uri) { Text = uri });
                }
            }
        }
    }

    protected void OnDeleteEvent(object sender, DeleteEventArgs a)
    {
        Application.Quit();
        //var dbg = lstURI.Cast<System.Object>().ToArray(); // only reveals the one entry anyway, 
        // and Children also only has the one
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

        var uriHost = (coaps ? "coaps" : "coap") + "://" + lstURI.ActiveText;
        var uriPath = "";
        string uriArgs = null;

        // TODO: This one is more organized for our needs
        if (uriArgs != null)
            uriPath += "?" + uriArgs;

        var uri = new Uri(new Uri(uriHost), uriPath);

        var message = new CoapMessage();

        // Just to experiment with being explicit
        var option_cf = new ContentFormat(ContentFormatType.TextPlain);

        message.Code = CoapMessageCode.Get;
        message.Type = confirmable ? CoapMessageType.Confirmable : CoapMessageType.NonConfirmable;
        message.Options.Add(option_cf);

        try
        {
            message.SetUri(uri);
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
            Gtk.Application.Invoke(delegate
            {
                // Display exception on screen
            });
            if(x.IsFaulted)
            {
                Console.WriteLine($"Got an error: {x.Exception.InnerException.Message}");
            }
        });
    }
}
