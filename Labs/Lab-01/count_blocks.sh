#!/bin/sh

# Находим все файлы, которые соответствуют условиям
files=`find "$1" -type f -size +3 -atime -5d`
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
