#!/bin/sh
# deploy_writer.sh
# Compila writer.c para ARM64 y lo copia al rootfs Buildroot

set -e

# Ruta al rootfs Buildroot
ROOTFS=/etc/finder-app/rootfs

# Directorio donde está writer.c y Makefile
SRC_DIR=$(pwd)

# Compilar writer
echo "Compiling writer.c for ARM64..."
make CROSS_COMPILE=aarch64-linux-gnu-

# Crear carpeta bin en rootfs si no existe
mkdir -p "$ROOTFS/bin"

# Copiar binario
echo "Deploying writer to $ROOTFS/bin..."
cp writer "$ROOTFS/bin/writer"
chmod +x "$ROOTFS/bin/writer"

echo "Deployment completed successfully!"
