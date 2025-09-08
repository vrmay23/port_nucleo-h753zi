#!/bin/bash

# ============================================================================
# Script: start_new_port_completion.sh
#
# Description:
#   Enables autocomplete for the `start_new_port` script, allowing
#   <source_dir> and <target_dir> arguments to auto-complete with Tab.
#
# Usage:
#   source start_new_port_completion.sh
# ============================================================================

_start_new_port_completions() {
  local cur prev
  COMPREPLY=()
  cur="${COMP_WORDS[COMP_CWORD]}"
  prev="${COMP_WORDS[COMP_CWORD-1]}"

  # Autocomplete only the first and second arguments with directories
  if [[ $COMP_CWORD -eq 1 || $COMP_CWORD -eq 2 ]]; then
    COMPREPLY=( $(compgen -d -- "$cur") )
  fi
}

# Attach the completion function to the script
complete -F _start_new_port_completions ./start_new_port

