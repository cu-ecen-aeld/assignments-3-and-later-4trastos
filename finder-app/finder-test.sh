#!/bin/sh
# finder-test-fixed.sh
# Tester script for assignment 1 and assignment 2 (robust version)

set -e
set -u

# SOLUCIÓN: Definir la función faltante
add_validate_error() {
    echo "ERROR: $1" >&2
    return 1
}

# Configuración por defecto
NUMFILES=10
WRITESTR="AELD_IS_FUN"
WRITEDIR="/tmp/aeld-data"
CONF_DIR="/etc/finder-app/conf"
WRITER="/etc/finder-app/writer.sh"
FINDER="/etc/finder-app/finder.sh"

# SOLUCIÓN: Usar ruta absoluta para archivos de conf
if [ ! -f "$CONF_DIR/username.txt" ] || [ ! -f "$CONF_DIR/assignment.txt" ]; then
    echo "Error: Missing conf files in $CONF_DIR"
    # Crear archivos de configuración por defecto si no existen
    mkdir -p "$CONF_DIR"
    echo "testuser" > "$CONF_DIR/username.txt"
    echo "assignment4" > "$CONF_DIR/assignment.txt"
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

# Crear archivos usando writer.sh
for i in $(seq 1 $NUMFILES); do
    sh "$WRITER" "$WRITEDIR/${username}_$i.txt" "$WRITESTR"
done

# Ejecutar finder.sh
OUTPUTSTRING=$(sh "$FINDER" "$WRITEDIR" "$WRITESTR")

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