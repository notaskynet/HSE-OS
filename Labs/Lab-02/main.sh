#!/bin/bash

BLOCK_SIZE=512
TIME_LIMIT=5
total_blocks=0
interrupt_count=0

handle_signal() {
    interrupt_count=`expr $interrupt_count + 1`
    echo ""
    
    if [ $interrupt_count -eq 2 ]; then
        echo -e "Interrupt signal (Ctrl+C) received twice. Total blocks read: $total_blocks"
    fi
    echo "To exit, type '!q"
    return 0
}

# Функция для поиска файлов и подсчета их блоков
find_files() {
    local dir_path="$1"
    last_dir="$dir_path"
    local blocks_read=0

    while read -r file_name file_size; do
        blocks=`expr $file_size + $BLOCK_SIZE - 1`
        blocks=`expr $blocks / $BLOCK_SIZE`
        echo "Found file: $file_name, blocks: $blocks"
        blocks_read=`expr $blocks_read + $blocks`
    done < <(find "$dir_path" -type f -size +`expr $BLOCK_SIZE \* 3`c -atime -$TIME_LIMIT -exec stat -f"%N %z" {} \;)

    echo "Total blocks read in this request: $blocks_read"
    total_blocks=`expr $total_blocks + $blocks_read`  # Суммируем со всеми запросами
}

# Устанавливаем обработчик сигнала прерывания
trap handle_signal SIGINT

# Запускаем основной цикл
while :; do
    echo "Enter directory path (type '!q' to exit): "
    read -r dir_path

    if [ "$dir_path" == "!q" ]; then
        echo "Exiting program."
        break
    fi

    if [ -z "$dir_path" ]; then
        echo "Please enter a directory path."
        continue
    fi

    echo "Files larger than 3 blocks and modified within the last 5 days:"
    find_files "$dir_path"
done
