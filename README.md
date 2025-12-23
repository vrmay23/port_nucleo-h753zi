# STM32H753ZI_modular_arch

Port for the **STM32-H753ZI Nucleo** board to the **NuttX OS**, utilizing a modular architecture for bring-up and configuration via `defconfig`.

---

# Embedded Development Workflow Scripts (NuttX)

This repository contains essential Bash scripts to streamline NuttX-based embedded development, focusing on clean Git history management and accelerated new board port creation.

## 1. update_myRepo.sh (Commit and Synchronization)

Automates `git add -A`, `commit`, and `push`. The script ensures that the *short-hashes* of the `nuttx` and `apps` submodules are included in every commit, and offers optional synchronization of the `main` branch after pushing.

### Functionality

* **Formatted Commit:** Generates a commit message that includes the short-hash of both `nuttx` and `apps` submodules for state traceability.
    *Format:* Update submodules: nuttx@<HASH> apps@<HASH> on <DATE/TIME> - <USER_MESSAGE>
* **Pull/Push:** Performs a `git pull` before the push to ensure the history is up-to-date and linear (if no conflicts occur).
* **Sync main:** Offers to check out to `main`, run `git pull`, and return to the current working branch to keep the local `main` synchronized.

### Usage

1.  Execute the script at the repository root:
    ./update_myRepo.sh
2.  Follow the prompts for an optional extra commit message and `main` branch synchronization.

---

## 2. start_new_port.sh (New Board Port Creation)

Clones an existing board directory to create a new port, performing critical string replacements (lowercase, uppercase, and suffix) across all files and directory names.

### Usage

The script requires 7 arguments to manage the different string formats used by the NuttX build system:

Format:
./start_new_port.sh <old_lower> <old_upper> <old_suffix> : <new_lower> <new_upper> <new_suffix>

Example:
./start_new_port.sh nucleo-h743zi STM32H7_NUCLEO_H743ZI 743ZI : nucleo-h753zi STM32H7_NUCLEO_H753ZI 753ZI

### ⚠️ Critical Execution Context

The script **must** be executed from **inside** the board family directory (e.g., `nuttx/boards/arm/stm32h7/`), where the family's `Kconfig` file resides.

### Post-Execution

The build system will **not** automatically recognize the new board. You must register the new port by manually editing the family's `Kconfig` file (usually the parent file in the board family) and adding a `source` statement.

---

## 3. autocompletion_start_new_port.sh (Autocompletion Utility)

Adds **Tab-completion** functionality for the `start_new_port.sh` script.

### Usage

You must **source** this file in your current shell session:

source autocompletion_start_new_port.sh

This enables autocompletion of available board directory names for the first and second arguments of `start_new_port.sh`.
