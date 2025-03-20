#!/bin/sh

interrupt_count=0
last_dir=""

count_blocks() {
    local files
    local filesizes
    local sum

    # Находим все файлы, которые соответствуют условиям
    files=`find "$last_dir" -type f -size +3 -atime -5d`
    # Получаем размеры файлов
    filesizes=`du -a $files | cut -d $'\t' -f 1 | tr '\n' '+'`
    # Убираем лишний плюс в конце
    filesizes=`echo "$filesizes" | sed 's/+$//'`
    # Выполняем вычисление через expr
    # Заменяем плюсы на пробелы, чтобы expr мог сложить числа
    filesizes=`echo "$filesizes" | sed 's/+/ /g'`
    sum=0
    for size in $filesizes; do
        sum=`expr $sum + $size`
    done
    echo $sum
}

handle_signal() {
    local total_blocks
    interrupt_count=`expr $interrupt_count + 1`
    echo ""
    
    if [ $interrupt_count -eq 2 ]; then
        total_blocks=`count_blocks $last_dir`
        echo "Interrupt signal (Ctrl+C) received twice. Total blocks read: $total_blocks"
    fi
    echo "To exit, type '!q"
    return 0
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

    find $dir_path -type f -size +3 -atime -5d | (echo "Files larger than 3 blocks and modified within the last 5 days:"; cat)
done
