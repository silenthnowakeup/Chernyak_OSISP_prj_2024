#!/bin/bash
# Получить PID текущего процесса
PID=$(cat /home/silenth/virtual-assistant/silence.pid)
kill $PID
exec /home/silenth/virtual-assistant/silence.pid
