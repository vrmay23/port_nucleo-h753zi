#!/bin/bash

# Configuration: Strict mode for robustness
set -e # Exit immediately if a command exits with a non-zero status
set -u # Exit immediately if a script tries to use an unset variable

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
# ============================================================================

# --- HINT / USAGE CHECK ---
if [ $# -eq 0 ]; then
    echo "ERROR: Missing arguments. You must specify the old and new board parameters."
    echo
    echo "Usage:"
    echo "  $0 <old_lower> <old_upper> <old_suffix> : <new_lower> <new_upper> <new_suffix>"
    echo
    echo "Example:"
    echo "  $0 nucleo-h743zi STM32H7_NUCLEO_H743ZI 743ZI : nucleo-h753zi STM32H7_NUCLEO_H753ZI 753ZI"
    echo
    echo "Available source board directories in the current folder:"
    find . -maxdepth 1 -type d ! -name '.' | sort
    exit 1
fi

# --- Execution Context Check (Improvement 3) ---
# Must be executed from inside the board family directory (e.g., boards/arm/stm32h7).
if [[ ! -f "Kconfig" ]]; then
    echo "Error: Must be executed from inside the board family directory (e.g., boards/arm/stm32h7)."
    echo "Kconfig file not found in the current directory."
    exit 1
fi

# Validate input arguments (7 arguments total, plus the ':' separator)
if [ $# -ne 7 ] || [ "$4" != ":" ]; then
    echo "Error: Invalid number of arguments or separator ':'. Expecting 7 arguments total."
    echo
    echo "Usage:"
    echo "  $0 <old_lower> <old_upper> <old_suffix> : <new_lower> <new_upper> <new_suffix>"
    echo
    echo "Example:"
    echo "  $0 nucleo-h743zi STM32H7_NUCLEO_H743ZI 743ZI : nucleo-h753zi STM32H7_NUCLEO_H753ZI 753ZI"
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
# ... (rest of the script remains the same)

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
echo "For more information, just open this file and check the header!"
echo
echo "Add it right below the original board entry."
