#!/bin/bash

echo "==========================================="
echo "=== Commit and push all local changes ===="
echo "==========================================="
echo

CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

# --- Commit message setup ---
NUTTX_HASH=$(cd nuttx && git rev-parse --short HEAD 2>/dev/null || echo "N/A")
APPS_HASH=$(cd apps && git rev-parse --short HEAD 2>/dev/null || echo "N/A")

read -p "Add extra commit message (optional): " USER_MSG

COMMIT_MSG="Update submodules: nuttx@$NUTTX_HASH apps@$APPS_HASH on $(date '+%Y-%m-%d %H:%M:%S')"
[ -n "$USER_MSG" ] && COMMIT_MSG="$COMMIT_MSG - $USER_MSG"

echo "Adding all changes..."
git add -A || { echo "Failed to add changes."; exit 1; }

if git diff --cached --quiet; then
  echo "No changes to commit."
else
  git commit -m "$COMMIT_MSG" || { echo "Failed to commit."; exit 1; }
fi

# Pull latest changes for current branch
if git ls-remote --heads origin "$CURRENT_BRANCH" | grep -q "$CURRENT_BRANCH"; then
  echo "Pulling latest changes from origin/$CURRENT_BRANCH..."
  git pull origin "$CURRENT_BRANCH" || { echo "Pull failed, fix manually."; exit 1; }
fi

echo "Pushing current branch..."
git push origin "$CURRENT_BRANCH" || { echo "Failed to push."; exit 1; }

# Ask if user wants to update main
read -p "Do you want to update local main from remote? (y/N): " UPDATE_MAIN
if [[ "$UPDATE_MAIN" =~ ^[Yy]$ ]]; then
    # Stash changes if needed
    LOCAL_CHANGES=$(git status --porcelain)
    if [ -n "$LOCAL_CHANGES" ]; then
        echo "Stashing local changes..."
        git stash push -m "temp stash before main update"
    fi

    echo "Switching to main branch..."
    git fetch origin main || { echo "Failed to fetch main"; exit 1; }
    git checkout main || { echo "Cannot switch to main. Resolve manually."; exit 1; }
    git reset --hard origin/main || { echo "Failed to reset main"; exit 1; }

    echo "Main updated successfully."

    # Return to previous branch and apply stash if needed
    git checkout "$CURRENT_BRANCH" || { echo "Failed to switch back to $CURRENT_BRANCH"; exit 1; }
    if [ -n "$LOCAL_CHANGES" ]; then
        echo "Applying stashed changes..."
        git stash pop || echo "No stash to apply."
    fi
fi

echo
echo "All operations completed."

