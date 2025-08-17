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

echo "Adding all changes to Git index..."
git add -A || { echo "Failed to add changes."; exit 1; }

if git diff --cached --quiet; then
  echo "No changes to commit. Working tree is clean."
else
  echo "Committing changes..."
  git commit -m "$COMMIT_MSG" || { echo "Failed to commit changes."; exit 1; }
fi

# Check if branch exists on remote before pull
if git ls-remote --heads origin "$CURRENT_BRANCH" | grep -q "$CURRENT_BRANCH"; then
  echo "Pulling latest changes from origin/$CURRENT_BRANCH before pushing..."
  if git pull origin "$CURRENT_BRANCH"; then
    echo "Successfully pulled latest changes."
  else
    echo "Git pull failed or conflicts, fix manually."
    exit 1
  fi
else
  echo "Branch $CURRENT_BRANCH does not exist on remote. Skipping pull."
fi

echo "Pushing changes to origin/$CURRENT_BRANCH..."
git push origin "$CURRENT_BRANCH" || { echo "Failed to push changes to remote."; exit 1; }

# Merge into main branch if current is not main
if [ "$CURRENT_BRANCH" != "main" ]; then
  echo "Checking out main branch..."
  git checkout main || { echo "Failed to checkout main"; exit 1; }

  echo "Pulling latest main from origin/main..."
  git pull origin main || { echo "Failed to pull origin/main"; exit 1; }

  echo "Merging $CURRENT_BRANCH into main..."
  git merge --no-ff "$CURRENT_BRANCH" -m "Merge branch '$CURRENT_BRANCH' into main" || {
    echo "Merge failed, resolve conflicts manually."
    exit 1
  }

  echo "Pushing updated main branch..."
  git push origin main || { echo "Failed to push main branch"; exit 1; }

  echo "Switching back to $CURRENT_BRANCH..."
  git checkout "$CURRENT_BRANCH" || { echo "Failed to switch back to $CURRENT_BRANCH"; exit 1; }
fi

echo
echo "All changes committed, pushed, and merged into main where applicable."

