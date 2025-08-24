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
if [ -n "$USER_MSG" ]; then
  COMMIT_MSG="$COMMIT_MSG - $USER_MSG"
fi

echo "Adding all changes..."
git add -A || { echo "Failed to add changes."; exit 1; }

if git diff --cached --quiet; then
  echo "No changes to commit. Working tree is clean."
else
  git commit -m "$COMMIT_MSG" || { echo "Failed to commit changes."; exit 1; }
fi

# --- Push logic ---
if git ls-remote --heads origin "$CURRENT_BRANCH" | grep -q "$CURRENT_BRANCH"; then
  echo "Pulling latest changes from origin/$CURRENT_BRANCH..."
  git pull origin "$CURRENT_BRANCH" || { echo "Git pull failed or conflicts, fix manually."; exit 1; }

  echo "Pushing current branch..."
  git push origin "$CURRENT_BRANCH" || { echo "Failed to push changes."; exit 1; }
else
  echo "Branch $CURRENT_BRANCH not found on remote. Doing first push..."
  git push -u origin "$CURRENT_BRANCH" || { echo "Initial push failed."; exit 1; }
fi

# --- Ask to sync main ---
read -p "Do you want to update local main from remote? (y/N): " SYNC_MAIN
if [[ "$SYNC_MAIN" =~ ^[Yy]$ ]]; then
  echo "Switching to main branch..."
  git checkout main || { echo "Failed to checkout main"; exit 1; }

  echo "Pulling latest main from origin/main..."
  git pull origin main || { echo "Failed to pull origin/main"; exit 1; }

  echo "Main updated successfully."

  echo "Switching back to $CURRENT_BRANCH..."
  git checkout "$CURRENT_BRANCH" || { echo "Failed to switch back to $CURRENT_BRANCH"; exit 1; }
fi

echo
echo "All operations completed."

