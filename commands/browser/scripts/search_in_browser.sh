#!/bin/bash

# Проверка на наличие аргумента
if [ -z "$1" ]; then
  echo "Usage: $0 <search_query>"
  exit 1
fi

# Объединение всех аргументов в одну строку
search_query="$*"

# Активируем окно браузера
window_id=$(xdotool search --onlyvisible --class "firefox" | head -1)
xdotool windowactivate "$window_id"

# Открываем новую вкладку
xdotool key "ctrl+t"

# Вводим URL поиска и выполняем его
xdotool type "https://www.google.com/search?q=$search_query"
xdotool key Return
