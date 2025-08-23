#!/bin/bash

echo "==========================================="
echo "=== Commit and push current branch ===="
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
  echo "Committing..."
  git commit -m "$COMMIT_MSG" || { echo "Commit failed"; exit 1; }
fi

# Push current branch
echo "Pushing branch $CURRENT_BRANCH..."
git push origin "$CURRENT_BRANCH" || { echo "Push failed"; exit 1; }

echo
echo "=== WAIT: Confirm that PR was merged into main remotely ==="
read -p "Press Enter to continue after merge..."

# Update main local
echo "Fetching latest main from remote..."
git fetch origin main || { echo "Fetch failed"; exit 1; }

echo "Switching to main..."
git checkout main || { echo "Checkout main failed"; exit 1; }

echo "Resetting local main to match origin/main..."
git reset --hard origin/main || { echo "Reset failed"; exit 1; }

# Go back to original branch
echo "Switching back to $CURRENT_BRANCH..."
git checkout "$CURRENT_BRANCH" || { echo "Checkout $CURRENT_BRANCH failed"; exit 1; }

# Rebase on top of updated main
echo "Rebasing $CURRENT_BRANCH on top of main..."
git rebase main || {
    echo "Rebase failed. Resolve conflicts manually."
    exit 1
}

echo
echo "Done. Branch $CURRENT_BRANCH is now up-to-date with main."

