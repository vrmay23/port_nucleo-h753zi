#!/bin/bash

# Configuration: Strict mode for robustness
set -e  # Exit immediately if a command fails
set -u  # Exit immediately if an unset variable is used

# Global Variables
CURRENT_BRANCH=""
NUTTX_HASH="N/A"
APPS_HASH="N/A"
COMMIT_MSG=""

# Define the editor to be used. Prioritizes $EDITOR, but uses 'vi' as fallback.
: ${EDITOR:=vi}

# --- Utility Functions ---

print_header() {
    echo "==========================================="
    echo "=== Commit and Push All Local Changes ===="
    echo "==========================================="
    echo
}

get_submodule_hashes() {
    # Captures the short-hashes of the submodules. If fails, returns 'N/A'.
    # Uses the last committed hash (or current HEAD if committed locally).
    NUTTX_HASH=$(cd nuttx && git rev-parse --short HEAD 2>/dev/null || echo "N/A")
    APPS_HASH=$(cd apps && git rev-parse --short HEAD 2>/dev/null || echo "N/A")
}

commit_submodules() {
    local SUBMODULES="nuttx apps"
    
    echo "--- Checking Submodules for Local Changes ---"
    
    for MODULE in $SUBMODULES; do
        if [ -d "$MODULE" ]; then
            (
                # Entra no submódulo
                cd "$MODULE" || { echo "FATAL: Cannot enter $MODULE directory."; exit 1; }
                
                # Verifica se há alterações não comitadas ou arquivos não rastreados
                # ^(M|??)\s detecta arquivos modificados (M) ou não rastreados (??)
                if git status --porcelain | grep -q '^\(M\|??\)\s'; then
                    echo "Changes detected in $MODULE. Staging and committing..."
                    
                    # 1. Adiciona todos os arquivos modificados/não rastreados
                    git add -A || { echo "FATAL: Failed to stage changes in $MODULE."; exit 1; }
                    
                    # 2. Comita as mudanças, forçando o usuário a fornecer uma mensagem
                    echo "Please enter a commit message for the $MODULE submodule. (Editor will open)"
                    git commit -a || { echo "WARN: Commit cancelled or message was empty in $MODULE. Skipping to next submodule."; }
                    echo "$MODULE commit completed (if message provided)."
                else
                    echo "No local changes to commit in $MODULE."
                fi
            )
        else
            echo "WARN: Submodule directory $MODULE not found. Skipping."
        fi
    done
    
    echo "--- Submodule check complete ---"
}


generate_commit_message() {
    local DATE_TIME=$(date '+%Y-%m-%d %H:%M:%S')
    local TEMP_FILE=$(mktemp)

    # Initial content for the commit file
    cat > "$TEMP_FILE" <<- EOM
Update submodules: nuttx@$NUTTX_HASH apps@$APPS_HASH on $DATE_TIME

# ------------------------
# Enter your commit message above. Lines starting with '#' will be ignored.
# The first line is the subject (max 50 chars), separated by a blank line from the body.
EOM

    # Open the editor (vi/nano/etc.)
    "$EDITOR" "$TEMP_FILE"

    # Read the final message, ignoring comments
    COMMIT_MSG=$(grep -v '^\s*#' "$TEMP_FILE" | sed '/^\s*$/d' | tr '\n' '\t' | sed 's/\t$//')
    rm "$TEMP_FILE"

    if [ -z "$COMMIT_MSG" ]; then
        echo "FATAL: Commit message is empty. Aborting commit."
        exit 1
    fi
}

perform_commit() {
    # Commit staged changes using the generated message
    echo "Performing commit on $CURRENT_BRANCH..."
    # Replace tabs with newlines for the commit message format
    COMMIT_MSG_FORMATTED=$(echo "$COMMIT_MSG" | tr '\t' '\n')
    git commit -m "$COMMIT_MSG_FORMATTED" || { echo "FATAL: Git commit failed."; exit 1; }
    echo "Commit successful."
}

pull_and_push() {
    local PUSH_CONFIRM=""
    read -p "Do you want to PULL (rebase) and then PUSH to origin/$CURRENT_BRANCH? (y/N): " PUSH_CONFIRM
    
    if [[ "$PUSH_CONFIRM" =~ ^[Yy]$ ]]; then
        echo "Pulling latest changes with rebase..."
        # Use pull --rebase to ensure linear history
        git pull --rebase origin "$CURRENT_BRANCH" || { echo "FATAL: Failed to pull/rebase. Fix conflicts and run the script again."; exit 1; }

        echo "Pushing commit to origin/$CURRENT_BRANCH..."
        git push origin "$CURRENT_BRANCH" || { echo "FATAL: Failed to push to origin/$CURRENT_BRANCH."; exit 1; }
        echo "Push successful."
    else
        echo "Commit saved locally. Skipping pull/push."
    fi
}

sync_main() {
    local SYNC_MAIN=""
    read -p "Do you want to update local main from remote? (y/N): " SYNC_MAIN
    
    if [[ "$SYNC_MAIN" =~ ^[Yy]$ ]]; then
        echo "Switching to main branch..."
        git checkout main || { echo "FATAL: Failed to checkout main."; exit 1; }

        echo "Pulling latest main from origin/main with rebase (ensuring linear history)..."
        # Pulls and rebases main to guarantee linearity
        git pull --rebase origin main || { echo "FATAL: Failed to pull/rebase origin/main. Fix before switching back."; exit 1; }

        echo "Main updated successfully."

        echo "Switching back to $CURRENT_BRANCH..."
        git checkout "$CURRENT_BRANCH" || { echo "FATAL: Failed to switch back to $CURRENT_BRANCH. Check your working tree state."; exit 1; }
    fi
}

# --- Main Execution ---
main() {
    print_header

    # Determines the current branch
    CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

    # 1. Comita todas as alterações DENTRO dos submódulos
    commit_submodules
    
    # 2. Captura os novos hashes (depois do commit)
    get_submodule_hashes
    
    # 3. Faz o stage das alterações no repositório principal (o novo hash do submódulo)
    echo "Staging all changes (including new submodule hashes)..."
    git add -A || { echo "FATAL: Failed to stage changes."; exit 1; }

    if git diff --cached --quiet; then
        echo "No changes staged. Nothing to commit."
    else
        generate_commit_message
        perform_commit
        pull_and_push
    fi
    
    # sync_main # Função desabilitada por padrão, se for para o MR, ela pode ser dispensada.
    
    echo "Script finished."
}

# Call main function
main
