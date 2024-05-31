#!/bin/bash
current_layout=$(setxkbmap -query | grep layout | awk '{print $2}')
if [ "$current_layout" = "en" ]; then
  setxkbmap ru
else
  setxkbmap en
fi
