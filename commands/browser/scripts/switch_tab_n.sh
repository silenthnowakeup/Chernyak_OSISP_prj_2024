#!/bin/bash
num=$1
window_id=$(xdotool search --onlyvisible --class "firefox" | head -1)
xdotool windowactivate "$window_id"
xdotool key "alt+$num"
