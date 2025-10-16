#!/bin/sh
# deploy_writer.sh
# Compila writer.c para ARM64 y lo copia al rootfs de Buildroot

set -e

# Ruta al rootfs del sistema embebido
ROOTFS="$HOME/Documentos/Linux_Development/assignment-4-4trastos/buildroot/output/target"

# Directorio actual (donde está writer.c y Makefile)
SRC_DIR=$(pwd)

echo "Cleaning previous build..."
make clean

echo "Compiling writer.c for ARM64..."
make CROSS_COMPILE=aarch64-linux-gnu-

# Crear carpeta bin en el rootfs si no existe
mkdir -p "$ROOTFS/bin"

# Copiar binario al rootfs
echo "Deploying writer to $ROOTFS/bin..."
cp "$SRC_DIR/writer" "$ROOTFS/bin/writer"
chmod +x "$ROOTFS/bin/writer"

# Opcional: copiar los archivos de configuración si no existen
if [ -d "$SRC_DIR/conf" ]; then
    echo "Deploying configuration files to $ROOTFS/etc/finder-app/conf..."
    mkdir -p "$ROOTFS/etc/finder-app/conf"
    cp -r "$SRC_DIR/conf/"* "$ROOTFS/etc/finder-app/conf/"
fi

echo "✅ Deployment completed successfully!"
