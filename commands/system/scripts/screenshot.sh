#!/bin/bash

# Папка, куда будут сохраняться скриншоты
screenshot_dir="/home/silenth/virtual-assistant/media"

# Создание папки, если она не существует
mkdir -p "$screenshot_dir"

# Текущее время для уникальности имени файла
timestamp=$(date +"%Y-%m-%d_%H-%M-%S")

# Полный путь до файла скриншота
screenshot_path="$screenshot_dir/screenshot_$timestamp.png"

# Создание скриншота
scrot "$screenshot_path"

# Вывод пути до созданного скриншота
echo "Screenshot saved to $screenshot_path"
