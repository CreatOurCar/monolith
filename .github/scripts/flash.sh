#!/bin/bash

set -e

cd "firmware/monolith" || exit 1

echo "Installing esptool..."
python -m pip install esptool

echo "Flashing firmware..."
esptool --chip esp32s3 -b 460800 --before default-reset --after hard-reset write-flash "@flash_args"

echo "Done."
