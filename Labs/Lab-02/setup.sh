#!/bin/bash

files_recent=(
    "images/cats/cat1.jpeg"
    "images/cats/cat2.jpeg"
    "images/dogs/dog1.jpg"
)

files_old=(
    "images/cats/cat3.jpeg"
    "images/dogs/dog2.jpeg"
    "images/dogs/dog3.jpeg"
)

# Установка даты обращения на 3 дня назад (менее 5 дней)
recent_date=$(date -v-3d "+%Y%m%d%H%M.%S")
for file in "${files_recent[@]}"; do
    if [ -f "$file" ]; then
        touch -a -t "$recent_date" "$file"
        echo "Обновлена дата обращения (менее 5 дней назад) для: $file"
    else
        echo "Файл $file не найден!"
    fi
done

# Установка даты обращения на 10 дней назад (более 5 дней)
old_date=$(date -v-10d "+%Y%m%d%H%M.%S")
for file in "${files_old[@]}"; do
    if [ -f "$file" ]; then
        touch -a -t "$old_date" "$file"
        echo "Обновлена дата обращения (более 5 дней назад) для: $file"
    else
        echo "Файл $file не найден!"
    fi
done