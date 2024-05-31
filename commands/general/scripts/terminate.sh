#!/bin/bash
# Получить PID текущего процесса из файла
PID=$(cat /home/silenth/virtual-assistant/silence.pid)

# Завершить программу
kill $PID
