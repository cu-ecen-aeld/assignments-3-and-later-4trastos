#!/bin/sh
# finder-test.sh - Fixed version

set -e
set -u

# Fix for missing function
add_validate_error() {
    echo "ERROR: $1" >&2
    return 1
}

# Configuración por defecto
NUMFILES=10
WRITESTR="AELD_IS_FUN"
WRITEDIR="/tmp/aeld-data"
CONF_DIR="/etc/finder-app/conf"
WRITER="/bin/writer"  # Cambiado a /bin/writer (donde realmente está)
FINDER="/bin/finder.sh"  # Cambiado a /bin/finder.sh

# SOLUCIÓN: Usar ruta absoluta correcta
if [ ! -f "$CONF_DIR/username.txt" ] || [ ! -f "$CONF_DIR/assignment.txt" ]; then
    echo "Error: Missing conf files in $CONF_DIR"
    exit 1
fi

username=$(cat "$CONF_DIR/username.txt")
assignment=$(cat "$CONF_DIR/assignment.txt")

# Sobrescribir valores si se pasan argumentos
if [ $# -ge 1 ]; then
    NUMFILES=$1
fi
if [ $# -ge 2 ]; then
    WRITESTR=$2
fi
if [ $# -ge 3 ]; then
    WRITEDIR="/tmp/aeld-data/$3"
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string '${WRITESTR}' to ${WRITEDIR}"

# Limpiar y crear directorio de escritura
rm -rf "$WRITEDIR"
mkdir -p "$WRITEDIR"

# Crear archivos usando writer
for i in $(seq 1 $NUMFILES); do
    "$WRITER" "$WRITEDIR/${username}_$i.txt" "$WRITESTR"
done

# Ejecutar finder.sh
OUTPUTSTRING=$("$FINDER" "$WRITEDIR" "$WRITESTR")

# Guardar resultado
echo "$OUTPUTSTRING" > /tmp/assignment4-result.txt

# Validación
echo "$OUTPUTSTRING" | grep -q "$MATCHSTR"
if [ $? -eq 0 ]; then
    echo "success"
    exit 0
else
    echo "failed: expected '${MATCHSTR}' in output but found:"
    echo "$OUTPUTSTRING"
    exit 1
fi