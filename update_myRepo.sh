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
  echo "Committing changes..."
  git commit -m "$COMMIT_MSG" || { echo "Failed to commit changes."; exit 1; }
fi

echo "Pulling latest changes from origin/$CURRENT_BRANCH..."
git pull origin "$CURRENT_BRANCH" || { echo "Git pull failed or conflicts, fix manually."; exit 1; }

echo "Pushing current branch..."
git push origin "$CURRENT_BRANCH" || { echo "Failed to push changes to remote."; exit 1; }

# Sempre mostrar link do PR
REPO_URL=$(git config --get remote.origin.url | sed -E 's#(git@|https://)([^/:]+)[:/]([^/]+)/([^/]+)(\.git)?#https://\2/\3/\4#')
echo
echo "👉 Abra ou atualize o PR aqui:"
echo "   $REPO_URL/pull/new/$CURRENT_BRANCH"
echo

read -p "Do you want to update local main from remote? (y/N): " UPDATE_MAIN
if [[ "$UPDATE_MAIN" =~ ^[Yy]$ ]]; then
  echo "Switching to main branch..."
  git checkout main || { echo "Failed to checkout main"; exit 1; }

  echo "Pulling latest main from origin/main..."
  git fetch origin main || { echo "Failed to fetch origin/main"; exit 1; }
  git reset --hard origin/main || { echo "Failed to reset local main"; exit 1; }

  echo "Main updated successfully."
  git checkout "$CURRENT_BRANCH" || { echo "Failed to switch back to $CURRENT_BRANCH"; exit 1; }
fi

echo
echo "All operations completed."

