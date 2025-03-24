#!/bin/sh

interrupt_count=0
last_files=""

handle_signal() {
    local total_blocks
    interrupt_count=`expr $interrupt_count + 1`
    echo ""
    
    if [ $interrupt_count -eq 2 ]; then
        total_blocks=$(du -a $last_files | awk '{sum+=$1} END {print sum}')
        echo "Interrupt signal (Ctrl+C) received twice. Total blocks read: $total_blocks"
    fi
    echo "To exit, type !q"
    return 0
}

# Устанавливаем обработчик сигнала прерывания
trap handle_signal 2

# Запускаем основной цикл
while :; do
    echo "Enter directory path (type '!q' to exit): "
    read -r dir_path

    if [ "$dir_path" = "!q" ]; then
        echo "Exiting program."
        break
    fi

    if [ -z "$dir_path" ]; then
        echo "Please enter a directory path."
        continue
    fi

    last_files=`find "$dir_path" -type f -size +3 -atime -5d`
    echo $last_files |  (echo "Files larger than 3 blocks and modified within the last 5 days:"; cat)
done
