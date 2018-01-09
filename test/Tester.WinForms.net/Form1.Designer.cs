namespace Tester.WinForms.net
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.btnRequest = new System.Windows.Forms.Button();
            this.cmbURI = new System.Windows.Forms.ComboBox();
            this.chkConfirmable = new System.Windows.Forms.CheckBox();
            this.chkSecured = new System.Windows.Forms.CheckBox();
            this.txtPayload = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.tssActivity = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnRequest
            // 
            this.btnRequest.Location = new System.Drawing.Point(105, 36);
            this.btnRequest.Name = "btnRequest";
            this.btnRequest.Size = new System.Drawing.Size(75, 23);
            this.btnRequest.TabIndex = 0;
            this.btnRequest.Text = "request";
            this.btnRequest.UseVisualStyleBackColor = true;
            this.btnRequest.Click += new System.EventHandler(this.btnRequest_Click);
            // 
            // cmbURI
            // 
            this.cmbURI.FormattingEnabled = true;
            this.cmbURI.Location = new System.Drawing.Point(260, 36);
            this.cmbURI.Name = "cmbURI";
            this.cmbURI.Size = new System.Drawing.Size(211, 21);
            this.cmbURI.TabIndex = 1;
            // 
            // chkConfirmable
            // 
            this.chkConfirmable.AutoSize = true;
            this.chkConfirmable.Checked = true;
            this.chkConfirmable.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkConfirmable.Location = new System.Drawing.Point(260, 73);
            this.chkConfirmable.Name = "chkConfirmable";
            this.chkConfirmable.Size = new System.Drawing.Size(81, 17);
            this.chkConfirmable.TabIndex = 2;
            this.chkConfirmable.Text = "Confirmable";
            this.chkConfirmable.UseVisualStyleBackColor = true;
            // 
            // chkSecured
            // 
            this.chkSecured.AutoSize = true;
            this.chkSecured.Location = new System.Drawing.Point(260, 96);
            this.chkSecured.Name = "chkSecured";
            this.chkSecured.Size = new System.Drawing.Size(66, 17);
            this.chkSecured.TabIndex = 3;
            this.chkSecured.Text = "Secured";
            this.chkSecured.UseVisualStyleBackColor = true;
            // 
            // txtPayload
            // 
            this.txtPayload.BackColor = System.Drawing.SystemColors.Control;
            this.txtPayload.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.txtPayload.Location = new System.Drawing.Point(260, 157);
            this.txtPayload.Multiline = true;
            this.txtPayload.Name = "txtPayload";
            this.txtPayload.Size = new System.Drawing.Size(269, 158);
            this.txtPayload.TabIndex = 4;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(257, 141);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(58, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Response:";
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.tssActivity});
            this.statusStrip1.Location = new System.Drawing.Point(0, 318);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(541, 22);
            this.statusStrip1.TabIndex = 6;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // tssActivity
            // 
            this.tssActivity.Name = "tssActivity";
            this.tssActivity.Size = new System.Drawing.Size(39, 17);
            this.tssActivity.Text = "Ready";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(541, 340);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.txtPayload);
            this.Controls.Add(this.chkSecured);
            this.Controls.Add(this.chkConfirmable);
            this.Controls.Add(this.cmbURI);
            this.Controls.Add(this.btnRequest);
            this.Name = "Form1";
            this.Text = "CoAP tester app";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnRequest;
        private System.Windows.Forms.ComboBox cmbURI;
        private System.Windows.Forms.CheckBox chkConfirmable;
        private System.Windows.Forms.CheckBox chkSecured;
        private System.Windows.Forms.TextBox txtPayload;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel tssActivity;
    }
}

