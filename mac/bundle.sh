#!/usr/bin/env bash
set -euo pipefail

APP_NAME="UserDoc"
BIN_NAME="userdoc"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build"
DIST_DIR="$ROOT/dist"
APP_DIR="$DIST_DIR/${APP_NAME}.app"
CONTENTS="$APP_DIR/Contents"
MACOS="$CONTENTS/MacOS"
RESOURCES="$CONTENTS/Resources"
FRAMEWORKS="$CONTENTS/Frameworks"

# 0) Чистим и создаём папки
rm -rf "$DIST_DIR"
mkdir -p "$MACOS" "$RESOURCES" "$FRAMEWORKS"

# 1) Копируем бинарь
cp "$BUILD_DIR/$BIN_NAME" "$MACOS/$BIN_NAME"
chmod +x "$MACOS/$BIN_NAME"

# 2) Кладём Info.plist
cp "$ROOT/mac/Info.plist" "$CONTENTS/Info.plist"

# 3) Иконка (опционально)
# Если есть iconset — соберём icns
if [ -d "$ROOT/mac/AppIcon.iconset" ]; then
  mkdir -p "$ROOT/assets"
  iconutil -c icns "$ROOT/mac/AppIcon.iconset" -o "$ROOT/assets/icon.icns"
fi

# Если есть icon.icns — кладём её
if [ -f "$ROOT/assets/icon.icns" ]; then
  cp "$ROOT/assets/icon.icns" "$RESOURCES/AppIcon.icns"
  /usr/libexec/PlistBuddy -c "Add :CFBundleIconFile string AppIcon.icns" "$CONTENTS/Info.plist" || true
fi

# 4) Вшиваем зависимости через dylibbundler
# Убедимся, что он установлен (в CI ставим brew install dylibbundler)
dylibbundler -od -b \
  -x "$MACOS/$BIN_NAME" \
  -d "$FRAMEWORKS" \
  -p "@rpath/Frameworks/" \
  -of

# 5) Чиним rpath для исполняемого файла (на всякий случай)
install_name_tool -add_rpath "@executable_path/../Frameworks" "$MACOS/$BIN_NAME" || true

echo "App bundle created at: $APP_DIR"
