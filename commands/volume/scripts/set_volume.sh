# Первый аргумент - уровень громкости
VOLUME_LEVEL=$1
amixer set Master ${VOLUME_LEVEL}%