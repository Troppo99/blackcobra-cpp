param (
    [string]$Sketch = "archives/milis",
    [string]$Board = "arduino:avr:mega",
    [string]$Port = "COM3"
)

Write-Host "[1/2] Compiling sketch: $Sketch"
arduino-cli compile --fqbn $Board $Sketch
if ($LASTEXITCODE -ne 0) {
    Write-Error "❌ Compile failed"
    exit $LASTEXITCODE
}

Write-Host "`n[2/2] Uploading to $Port..."
arduino-cli upload -p $Port --fqbn $Board $Sketch
if ($LASTEXITCODE -ne 0) {
    Write-Error "❌ Upload failed"
    exit $LASTEXITCODE
}

Write-Host "`n✅ Done."
pause
