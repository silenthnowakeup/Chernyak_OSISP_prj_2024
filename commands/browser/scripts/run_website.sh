#!/bin/bash
window_id=$(xdotool search --onlyvisible --class "firefox" | head -1)
xdotool windowactivate "$window_id"
# Первый аргумент - URL сайта
URL=$1
firefox "$URL" &
