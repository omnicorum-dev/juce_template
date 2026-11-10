#!/usr/bin/env bash

echo "⚠️  WARNING: This will reset your project!"
echo "This will permanently delete .git and tasks."
echo
read -r -p "Are you sure you want to continue? (y/N): " confirm

if [[ "$confirm" != "y" ]]; then
    echo "Cancelled. Nothing was changed."
    exit 0
fi

echo "Resetting project..."

rm -rf .git
rm -rf tasks

git init
tatr init

echo "Reset complete."
