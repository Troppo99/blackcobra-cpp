Add-Type -AssemblyName System.Windows.Forms
[System.Windows.Forms.Application]::EnableVisualStyles()

# --- GUI Form ---
$form = New-Object System.Windows.Forms.Form
$form.Text = "Arduino Uploader"
$form.Size = New-Object System.Drawing.Size(400,250)
$form.StartPosition = "CenterScreen"

# --- Label Sketch ---
$lblSketch = New-Object System.Windows.Forms.Label
$lblSketch.Text = "Nama folder sketch (misal: milis):"
$lblSketch.Location = New-Object System.Drawing.Point(10,20)
$lblSketch.Size = New-Object System.Drawing.Size(300,20)
$form.Controls.Add($lblSketch)

# --- Input Sketch ---
$txtSketch = New-Object System.Windows.Forms.TextBox
$txtSketch.Location = New-Object System.Drawing.Point(10,40)
$txtSketch.Size = New-Object System.Drawing.Size(360,20)
$txtSketch.Text = "archives/milis"
$form.Controls.Add($txtSketch)

# --- Label COM ---
$lblCOM = New-Object System.Windows.Forms.Label
$lblCOM.Text = "Port COM (misal: COM3):"
$lblCOM.Location = New-Object System.Drawing.Point(10,70)
$lblCOM.Size = New-Object System.Drawing.Size(300,20)
$form.Controls.Add($lblCOM)

# --- Input COM ---
$txtCOM = New-Object System.Windows.Forms.TextBox
$txtCOM.Location = New-Object System.Drawing.Point(10,90)
$txtCOM.Size = New-Object System.Drawing.Size(360,20)
$txtCOM.Text = "COM3"
$form.Controls.Add($txtCOM)

# --- Checkbox Serial Monitor ---
$chkMonitor = New-Object System.Windows.Forms.CheckBox
$chkMonitor.Text = "Buka Serial Monitor setelah upload"
$chkMonitor.Location = New-Object System.Drawing.Point(10,120)
$chkMonitor.Size = New-Object System.Drawing.Size(300,20)
$chkMonitor.Checked = $false
$form.Controls.Add($chkMonitor)

# --- Tombol Upload ---
$btnUpload = New-Object System.Windows.Forms.Button
$btnUpload.Text = "Compile & Upload"
$btnUpload.Location = New-Object System.Drawing.Point(10,160)
$btnUpload.Size = New-Object System.Drawing.Size(150,30)
$form.Controls.Add($btnUpload)

# --- Output Status ---
$output = New-Object System.Windows.Forms.Label
$output.Location = New-Object System.Drawing.Point(10,200)
$output.Size = New-Object System.Drawing.Size(360,20)
$form.Controls.Add($output)

# --- Aksi Upload ---
$btnUpload.Add_Click({
    $sketch = $txtSketch.Text
    $port = $txtCOM.Text
    $board = "arduino:avr:mega"

    $output.Text = "🛠 Compile..."
    $compile = Start-Process -FilePath "arduino-cli" -ArgumentList "compile --fqbn $board $sketch" -NoNewWindow -Wait -PassThru
    if ($compile.ExitCode -ne 0) {
        $output.Text = "❌ Compile gagal."
        return
    }

    $output.Text = "⬆️ Upload..."
    $upload = Start-Process -FilePath "arduino-cli" -ArgumentList "upload -p $port --fqbn $board $sketch" -NoNewWindow -Wait -PassThru
    if ($upload.ExitCode -ne 0) {
        $output.Text = "❌ Upload gagal."
        return
    }

    $output.Text = "✅ Selesai upload."

    if ($chkMonitor.Checked) {
        Start-Process -NoNewWindow -FilePath "arduino-cli" -ArgumentList "monitor -p $port -c baudrate=9600"
    }
})

# --- Tampilkan Form ---
$form.Topmost = $true
[void]$form.ShowDialog()
