#!/bin/sh
# deploy_writer.sh
# Compila writer.c para ARM64 y lo copia al rootfs de Buildroot

set -e

# Ruta al rootfs del sistema embebido
ROOTFS=~/Documentos/Linux_Development/assignment-4-4trastos/buildroot/output/target

echo "Compiling writer.c for ARM64..."
make clean
make CROSS_COMPILE=aarch64-linux-gnu-

echo "Deploying writer to $ROOTFS/bin..."
mkdir -p "$ROOTFS/bin"
cp writer "$ROOTFS/bin/writer"
chmod +x "$ROOTFS/bin/writer"

echo "✅ Deployment completed successfully!"
