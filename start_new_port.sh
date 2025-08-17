#!/bin/bash

# ============================================================================
# Script: start_new_port.sh
#
# Description:
#   Clone a board port directory from an existing one.
#   This script operates from inside a board-specific directory, such as
#   `boards/arm/stm32h7`. It does NOT modify the main `boards/Kconfig` file.
#   You must manually add a 'source' entry to `boards/Kconfig` after running
#   this script to register the new board with the build system.
#
# Usage:
#   ./start_new_port.sh <old_lower> <old_upper> <old_suffix> : <new_lower> <new_upper> <new_suffix>
#
# Example:
#   ./start_new_port.sh nucleo-h743zi STM32H7_NUCLEO_H743ZI 743ZI : nucleo-h753zi STM32H7_NUCLEO_H753ZI 753ZI
# ============================================================================

# Validate input arguments
if [ $# -ne 7 ] || [ "$4" != ":" ]; then
  echo "Usage:"
  echo "  $0 <old_lower> <old_upper> <old_suffix> : <new_lower> <new_upper> <new_suffix>"
  echo
  echo "Example:"
  echo "  $0 nucleo-h743zi STM32H7_NUCLEO_H743ZI 743ZI : nucleo-h753zi STM32H7_NUCLEO_H753ZI 753ZI"
  echo
  echo "Available directories:"
  find . -maxdepth 1 -type d ! -name '.' | sort
  exit 1
fi

# Set variables from inputs
OLD_LOWER="$1"
OLD_UPPER="$2"
OLD_SUFFIX="$3"
NEW_LOWER="$5"
NEW_UPPER="$6"
NEW_SUFFIX="$7"

# --- Step 1: Clone the board directory ---
echo "Cloning '$OLD_LOWER' to '$NEW_LOWER'..."

if [ ! -d "$OLD_LOWER" ]; then
  echo "Error: Source directory '$OLD_LOWER' not found."
  exit 1
fi

cp -r "$OLD_LOWER" "$NEW_LOWER"

# --- Step 2: Replace all relevant strings in the new directory ---
echo "Replacing old identifiers with new ones in '$NEW_LOWER'..."

# Use a safer find/sed combination
find "$NEW_LOWER" -type f -exec sed -i \
  -e "s/$OLD_LOWER/$NEW_LOWER/g" \
  -e "s/$OLD_UPPER/$NEW_UPPER/g" \
  -e "s/$OLD_SUFFIX/$NEW_SUFFIX/g" \
  -e "s/CONFIG_${OLD_UPPER}/CONFIG_${NEW_UPPER}/g" \
  -e "s/ARCH_BOARD_${OLD_UPPER}/ARCH_BOARD_${NEW_UPPER}/g" \
  -e "s/CONFIG_${OLD_SUFFIX}/CONFIG_${NEW_SUFFIX}/g" \
  -e "s/CONFIG_${OLD_LOWER^^}/CONFIG_${NEW_LOWER^^}/g" {} +

# --- Step 3: Rename files and directories ---
echo "Renaming files and directories inside '$NEW_LOWER'..."

find "$NEW_LOWER" -depth -name "*$OLD_LOWER*" | while read -r path; do
  newpath=$(echo "$path" | sed "s/$OLD_LOWER/$NEW_LOWER/g")
  mkdir -p "$(dirname "$newpath")"
  mv "$path" "$newpath"
done

# --- Final Check & Instructions ---
echo
echo "---"
echo "Processo concluído."
echo
echo "O port para '$NEW_LOWER' foi criado com sucesso."
echo "Para que o sistema de build do NuttX reconheça a nova placa, você deve adicionar"
echo "a seguinte linha ao arquivo 'boards/Kconfig':"
echo
echo "    source \"boards/arm/stm32h7/$NEW_LOWER/Kconfig\""
echo
echo "Adicione-a logo abaixo da linha 'source' do board original."
