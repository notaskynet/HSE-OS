#!/bin/bash

BLOCK_SIZE=4096
TIME_LIMIT=5
total_blocks=0
interrupt_count=0

handle_signal() {
    interrupt_count=$((interrupt_count + 1))
    if [ $interrupt_count -eq 2 ]; then
        echo -e "\nСигнал прерывания (Ctrl+C) получен дважды. Общее количество блоков: $total_blocks"
        echo "(Для завершения программы введите '!q')"
    fi
    return 0
}

is_recent_file() {
    local file="$1"
    local file_time=$(stat -f %a "$file")
    local current_time=$(date +%s)
    local diff_in_days=$(( (current_time - file_time) / 86400 ))

    if [ $diff_in_days -lt $TIME_LIMIT ]; then
        return 0
    else
        return 1
    fi
}

process_directory() {
    local dir_path="$1"
    if [ ! -d "$dir_path" ]; then
        echo "Не удалось открыть каталог: $dir_path"
        return
    fi

    echo "Список файлов размером более 3 блоков и измененных менее 5 дней назад:"

    for file in "$dir_path"/*; do
        if [ -f "$file" ]; then
            local blocks=$(stat -f %b "$file")
            if [ $blocks -gt 3 ] && is_recent_file "$file"; then
                echo "$(basename "$file")"
                total_blocks=$((total_blocks + blocks))
            fi
        fi
    done
}

trap handle_signal SIGINT

while true; do
    echo -n "Введите путь к каталогу (для выхода введите '!q'): "
    read -r dir_path

    if [ "$dir_path" == "!q" ]; then
        echo "Выход из программы."
        break
    fi

    if [ -z "$dir_path" ]; then
        echo "Пожалуйста, введите путь к каталогу."
        continue
    fi

    process_directory "$dir_path"
done
