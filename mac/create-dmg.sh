#!/usr/bin/env bash
set -euo pipefail

APP_NAME="UserDoc"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST_DIR="$ROOT/dist"
APP_DIR="$DIST_DIR/${APP_NAME}.app"

if [ ! -d "$APP_DIR" ]; then
  echo "App not found: $APP_DIR"
  exit 1
fi

# Имя DMG можно параметризовать архитектурой
ARCH_SUFFIX="${1:-}"
DMG_PATH="$DIST_DIR/${APP_NAME}${ARCH_SUFFIX}.dmg"

# Создаём dmg
hdiutil create -volname "$APP_NAME" -srcfolder "$APP_DIR" -ov -format UDZO "$DMG_PATH"

echo "DMG created at: $DMG_PATH"
