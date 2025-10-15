#!/bin/sh
# finder-test.sh (corregido para Buildroot)
# Tester script for assignment 1 and assignment 2
# Autor: Adaptado por ChatGPT

set -e
set -u

# Valores por defecto
NUMFILES=10
WRITESTR=AELD_IS_FUN
WRITEDIR=/tmp/aeld-data

# Leer username y assignment de los archivos de configuración
CONFIG_DIR=/etc/finder-app/conf
username=$(cat "$CONFIG_DIR/username.txt")
assignment=$(cat "$CONFIG_DIR/assignment.txt")

# Manejo de argumentos
if [ $# -lt 3 ]; then
    echo "Using default value ${WRITESTR} for string to write"
    if [ $# -lt 1 ]; then
        echo "Using default value ${NUMFILES} for number of files to write"
    else
        NUMFILES=$1
    fi
else
    NUMFILES=$1
    WRITESTR=$2
    WRITEDIR=/tmp/aeld-data/$3
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${WRITEDIR}"

# Limpiar directorio anterior
rm -rf "${WRITEDIR}"

# Crear directorio solo si no es assignment1
if [ "$assignment" != "assignment1" ]; then
    mkdir -p "$WRITEDIR"
    if [ -d "$WRITEDIR" ]; then
        echo "$WRITEDIR created"
    else
        echo "Error: Failed to create directory"
        exit 1
    fi
fi

# Escribir archivos usando writer.sh en lugar de binario ELF
for i in $(seq 1 $NUMFILES); do
    /etc/finder-app/writer.sh "$WRITEDIR/${username}_$i.txt" "$WRITESTR"
done

# Ejecutar finder.sh y capturar salida
OUTPUTSTRING=$(/etc/finder-app/finder.sh "$WRITEDIR" "$WRITESTR")

# Guardar resultado en assignment4-result.txt
echo "$OUTPUTSTRING" > /tmp/assignment4-result.txt

# Limpiar archivos temporales
rm -rf /tmp/aeld-data

# Verificar resultado
set +e
echo "$OUTPUTSTRING" | grep "${MATCHSTR}"
if [ $? -eq 0 ]; then
    echo "success"
    exit 0
else
    echo "failed: expected '${MATCHSTR}' in ${OUTPUTSTRING} but instead found"
    exit 1
fi
