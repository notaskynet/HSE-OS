# Лабораторная работа №1 - Вариант 23

## Задание

Написать программу поиска в одном из соседних каталогов файлов размером более, чем в три блока, к которым обращались менее, чем пять дней назад. Полученную информацию через файловую систему послать в параллельный процесс. Предусмотреть возможность неоднократного внешнего прерывания. При поступлении 2-го прерывания вывести информацию об общем количестве блоков, занимаемых этими файлами.

## Файлы

```md
├── README.md
├── images
│   ├── cats
│   │   ├── cat1.jpeg
│   │   ├── cat2.jpeg
│   │   └── cat3.jpeg
│   └── dogs
│       ├── dog1.jpg
│       ├── dog2.jpeg
│       └── dog3.jpeg
├── main.c
└── setup.sh
```

## Запуск

Для того, чтобы программа корректно отработала, нужно запустить скрипт `setup.sh`. Он изменит дату последнего обращения у файлов `cat1.jpeg`, `cat2.jpeg` и `dog1.jpg` на 3 дня назад, а у файлов `cat3.jpeg`, `dog2.jpeg` и `dog3.jpeg` - 10 дней назад.

```bash
chmod +x setup.sh
./setup.sh
```

Далее нужно скомпилировать `main.c`:

```bash
clang main.c -o main
./main
```

Далее программа запросит путь до директории, где искать файлы. Для того, чтобы завершить работу программы, нужно ввести `!q`, когда программа запросить путь до директории.
