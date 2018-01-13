using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using CoAPNet;
using CoAPNet.Udp;

namespace Tester.WinForms.net
{
    public partial class Form1 : Form
    {
        const string mruUriFile = "mruuri.txt";

        public Form1()
        {
            InitializeComponent();

            if(File.Exists(mruUriFile))
            {
                var mruLines = File.ReadAllLines(mruUriFile);
                mruUris.AddRange(mruLines);
            }

            cmbURI.DataSource = mruUris;
            cmbPath.Text = "request_path";
        }

        List<string> mruUris = new List<string>();

        private async void btnRequest_Click(object sender, EventArgs e)
        {
            txtPayload.BackColor = SystemColors.Control;
            txtPayload.Text = null;

            var endpoint = new CoapUdpEndPoint();
            var client = new CoapClient(endpoint);

            var confirmable = chkConfirmable.Checked;
            bool coaps = chkSecured.Checked;

            var uriHost = (coaps ? "coaps" : "coap") + "://" + cmbURI.Text;
            var uriPath = cmbPath.Text;
            string uriArgs = null;

            // TODO: This one is more organized for our needs
            if (uriArgs != null)
                uriPath += "?" + uriArgs;

            var uri = new Uri(new Uri(uriHost), uriPath);

            var message = new CoapMessage();

            message.Code = CoapMessageCode.Get;
            message.Type = confirmable ? CoapMessageType.Confirmable : CoapMessageType.NonConfirmable;

            try
            {
                message.SetUri(uri);
            }
            catch (Exception _e)
            {
                MessageBox.Show($"Error: {_e.Message}");
            }
            /*
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

            requestor.ContinueWith(t =>
            {
                if (t.IsFaulted)
                {
                    Console.WriteLine($"Error: {t.Exception.InnerException.Message}");
                }
            }); */

            Console.WriteLine($"Issuing request: {uri}");

            try
            {
                tssActivity.Text = "Sending message";

                await client.SendAsync(message);

                Console.WriteLine($"Waiting for a response");

                tssActivity.Text = "Waiting for response";

                // NOTE: NON doesn't yet work here ( I think CoAPNet itself doesnt yet support it)
                var response = await client.ReceiveAsync();

                Console.Write("got a response: ");

                tssActivity.Text = "Got response";

                var payload = Encoding.UTF8.GetString(response.Message.Payload);

                Console.WriteLine(payload);
                txtPayload.Text = payload;
            }
            catch(Exception _e)
            {
                var _message = $"Error: {_e.Message}";
                Console.WriteLine(_message);
                tssActivity.Text = "Error";
                txtPayload.BackColor = Color.OrangeRed;
                txtPayload.Text = _message;
            }
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            File.WriteAllLines(mruUriFile, mruUris);
        }

        private async void btnDebug_Click(object sender, EventArgs e)
        {
            var endpoint = new CoapUdpEndPoint();
            var client = new CoapClient(endpoint);

            var message = new CoapMessage();

            message.Code = CoapMessageCode.Get;
            message.SetUri("coap://" + cmbURI.Text + "/" + cmbPath.Text);

            tssActivity.Text = "Sending message";

            await client.SendAsync(message);

            tssActivity.Text = "Awaiting response";

            // NOTE: NON doesn't yet work here ( I think CoAPNet itself doesnt yet support it)
            var result = await client.ReceiveAsync();
        }
    }
}
