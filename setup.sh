#!/bin/bash
echo "==> Instalando dependencias..."
sudo apt-get update -qq && sudo apt-get install -y gcc make

echo "==> Descargando stb_image_write.h..."
curl -sL https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h \
     -o stb_image_write.h

