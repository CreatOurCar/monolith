$ErrorActionPreference = "Stop"

Set-Location -Path "firmware/monolith"

Write-Host "Installing esptool..."
python -m pip install esptool

Write-Host "Flashing firmware..."
& esptool --chip esp32s3 -b 460800 --before default-reset --after hard-reset write-flash "@flash_args"

Write-Host "Done."
