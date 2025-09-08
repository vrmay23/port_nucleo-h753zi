#!/bin/bash

# ============================================================================
# Script: start_new_port.sh
#
# Description:
#   Clone an existing board port directory to create a new one.
#   This script must be executed from inside the board family directory,
#   for example: `boards/arm/stm32h7`.
#   It does NOT modify the main `boards/Kconfig` file.
#   You must manually add a 'source' entry to `boards/Kconfig` after running
#   this script to register the new board with the build system.
#
# Usage:
#   ./start_new_port.sh <old_lower> <old_upper> <old_suffix> : <new_lower> <new_upper> <new_suffix>
#
# Example:
#   ./start_new_port.sh nucleo-h743zi STM32H7_NUCLEO_H743ZI 743ZI : nucleo-h753zi STM32H7_NUCLEO_H753ZI 753ZI


#Now follow these steps to register and test the new board:"
#
# 1 - Edit 'boards/Kconfig' and add:
#
# config ARCH_BOARD
#         default "nucleo-h753zi"             if ARCH_BOARD_NUCLEO_H753ZI
#
# if ARCH_BOARD_NUCLEO_H753ZI'
# source "boards/arm/stm32h7/nucleo-h753zi/Kconfig"
# endif
#
# config ARCH_BOARD_NUCLEO_H753ZI
#         bool "STM32H753 Nucleo H753ZI"
#         depends on ARCH_CHIP_STM32H753ZI
#         select ARCH_HAVE_LEDS
#         select ARCH_HAVE_BUTTONS
#         select ARCH_HAVE_IRQBUTTONS
#         ---help---
#                 STMicro Nucleo H753ZI board based on the STMicro STM32H753ZI MCU.
#
# 2 - Run:
#     ./tools/configure.sh nucleo-h753zi:nsh"
#
# 3 - Run:
#     make menuconfig
#     -> Go to 'Board Selection/Selected target board'
#     -> Check that 'STM32H753 Nucleo H753ZI' appears.
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
  echo "Error: source directory '$OLD_LOWER' not found."
  exit 1
fi

if [ -d "$NEW_LOWER" ]; then
  echo "Warning: target directory '$NEW_LOWER' already exists. Removing it..."
  rm -rf "$NEW_LOWER"
fi

cp -r "$OLD_LOWER" "$NEW_LOWER"

# --- Step 2: Replace all relevant strings in the new directory ---
echo "Replacing identifiers inside '$NEW_LOWER'..."

find "$NEW_LOWER" -type f -exec sed -i \
  -e "s/$OLD_LOWER/$NEW_LOWER/g" \
  -e "s/$OLD_UPPER/$NEW_UPPER/g" \
  -e "s/$OLD_SUFFIX/$NEW_SUFFIX/g" \
  -e "s/CONFIG_${OLD_UPPER}/CONFIG_${NEW_UPPER}/g" \
  -e "s/ARCH_BOARD_${OLD_UPPER}/ARCH_BOARD_${NEW_UPPER}/g" \
  -e "s/CONFIG_${OLD_SUFFIX}/CONFIG_${NEW_SUFFIX}/g" \
  -e "s/CONFIG_${OLD_LOWER^^}/CONFIG_${NEW_LOWER^^}/g" {} +

# --- Step 3: Rename files and directories if needed ---
echo "Renaming files and directories inside '$NEW_LOWER'..."

find "$NEW_LOWER" -depth -name "*$OLD_LOWER*" | while read -r path; do
  newpath=$(echo "$path" | sed "s/$OLD_LOWER/$NEW_LOWER/g")
  mkdir -p "$(dirname "$newpath")"
  mv "$path" "$newpath"
done

# --- Final Check & Instructions ---
echo
echo "---"
echo "Process completed."
echo
echo "The new board port '$NEW_LOWER' was created successfully."
echo
echo "To make the build system recognize it, you must edit the file"
echo " 'boards/Kconfig' like the example on the header of this file."
echo
echo "For mor information, just open this file and check the header!"
echo
echo "Add it right below the original board entry."

