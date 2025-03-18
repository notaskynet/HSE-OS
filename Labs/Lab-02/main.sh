#!/bin/bash

BLOCK_SIZE=4096
TIME_LIMIT=5
total_blocks=0
interrupt_count=0
fifo="/tmp/file_info_fifo"         # Переменная для FIFO канала
total_blocks_file="/tmp/total_blocks.txt"  # Временный файл для хранения total_blocks

# Создаем FIFO, если его нет
if [[ ! -p $fifo ]]; then
    mkfifo "$fifo"
fi

# Устанавливаем права доступа для FIFO: чтение и запись для всех
chmod 666 "$fifo"

# Инициализация total_blocks в файл
if [[ ! -e $total_blocks_file ]]; then
    touch "$total_blocks_file"
fi
echo "0" > "$total_blocks_file"

handle_signal() {
    interrupt_count=$(expr $interrupt_count + 1)  # Увеличиваем счетчик прерываний
    echo ""
    
    if [ $interrupt_count -eq 2 ]; then
        # Читаем актуальное значение общего размера (в байтах) из файла
        total_bytes=$(cat "$total_blocks_file")
        
        # Если total_bytes пустой, присваиваем 0
        if [ -z "$total_bytes" ]; then
            total_bytes=0
        fi
        
        # Проверяем, что total_bytes является числом
        if [[ ! "$total_bytes" =~ ^[0-9]+$ ]]; then
            echo "Error: total_bytes is not a valid number."
            return 1
        fi
        
        # Вычисляем количество блоков по формуле: (total_bytes + BLOCK_SIZE - 1) / BLOCK_SIZE
        total_blocks=$(( (total_bytes + BLOCK_SIZE - 1) / BLOCK_SIZE ))

        echo -e "\nInterrupt signal (Ctrl+C) received twice. Total blocks read: $total_blocks"
        echo "(To exit, type '!q')"
    fi
    return 0
}

# Функция для обработки файлов в фоновом процессе
process_file_info() {
    while true; do
        if read -r file_info < "$fifo"; then
            file_name=$(echo "$file_info" | cut -d' ' -f1)
            file_blocks=$(echo "$file_info" | cut -d' ' -f2)
            if [ -n "$file_name" ] && [ -n "$file_blocks" ]; then
                # Считываем текущее значение total_blocks из файла
                current_total=$(cat "$total_blocks_file")
                total_blocks=$(expr $current_total + $file_blocks)
                echo $total_blocks > "$total_blocks_file"
            fi
        else
            sleep 0.1  # Задержка для предотвращения частых ошибок
        fi
    done
}

# Функция для поиска файлов
find_files() {
    local dir_path="$1"
    
    # Подставляем значения BLOCK_SIZE и TIME_LIMIT
    find "$dir_path" -type f -size +$(expr $BLOCK_SIZE \* 3)c -atime -$TIME_LIMIT -exec stat -f"%N %z" {} \; | while read -r line; do
        # Рассчитываем количество блоков (размер файла / BLOCK_SIZE), округляя вверх
        file_size=$(echo "$line" | cut -d' ' -f2)
        
        # Если размер файла больше 0, вычисляем число блоков, иначе присваиваем 0
        if [ "$file_size" -gt 0 ]; then
            file_blocks=$(expr \( $file_size + $BLOCK_SIZE - 1 \) / $BLOCK_SIZE)
        else
            file_blocks=0
        fi
        
        # Выводим информацию о найденном файле
        echo "Found file: $line, blocks: $file_blocks"
        
        echo "$line $file_blocks" > "$fifo" &  # Отправляем информацию в FIFO
    done
}

# Функция для обработки каталога
process_directory() {
    local dir_path="$1"
    if [ ! -d "$dir_path" ]; then
        echo "Failed to open directory: $dir_path"
        return
    fi

    echo "Files larger than 3 blocks and modified within the last 5 days:"
    find_files "$dir_path"
}

# Устанавливаем обработчик сигнала прерывания
trap handle_signal SIGINT

# Запускаем обработку файлов в фоновом процессе
process_file_info &

# Запоминаем ID фона
process_pid=$!

# Запускаем основной цикл
while true; do
    echo -n "Enter directory path (type '!q' to exit): "
    read -r dir_path

    if [ "$dir_path" == "!q" ]; then
        echo "Exiting program."
        kill $process_pid  # Завершаем фоновый процесс
        wait $process_pid  # Ждем завершения процесса
        break
        break
    fi

    if [ -z "$dir_path" ]; then
        echo "Please enter a directory path."
        continue
    fi

    process_directory "$dir_path"
done

wait

rm -f "$fifo"
rm -f "$total_blocks_file"