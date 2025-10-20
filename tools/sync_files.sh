#!/usr/bin/env bash
# Usage: ./sync_sysdep.sh <PREFIX> <MODEL>
# Example: GPL_DIR:> $REPO/tools/sync_files.sh $REPO TUF-AX3000
# Copies files from A to X1/X2 only if they already exist, skipping symlinks.

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <PREFIX> <MODEL>"
  exit 1
fi

PREFIX="$1"
MODEL="$2"

A="./release/src/router/www/sysdep/$MODEL/www"
X1="$PREFIX/release/src/router/www/sysdep/$MODEL/www"
X2="$PREFIX/release/src/router/www/sysdep/FUNCTION/TUF_UI"
LOGFILE="missing_files.log"

> "$LOGFILE"

echo "Source (A): $A"
echo "Targets (X):"
echo "  1) $X1"
echo "  2) $X2"
echo

# Recursively scan all files under A
find "$A" -type f | while read -r file_A; do
  rel_path="${file_A#$A/}"
  found=false

  for file_X in "$X1/$rel_path" "$X2/$rel_path"; do
    if [ -L "$file_X" ]; then
      echo "⚠️  Skipping symlink: $file_X"
      found=true
      break
    elif [ -f "$file_X" ]; then
      found=true
      echo "Match found: $file_X"
      cp -v "$file_A" "$file_X"
      break
    fi
  done

  if [ "$found" = false ]; then
    echo "❌ Missing in X: $rel_path"
    echo "$rel_path" >> "$LOGFILE"
  fi
done

echo
echo "✅ Done. Missing files logged to $LOGFILE"

