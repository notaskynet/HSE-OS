#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BLOCK_SIZE 1024  // Размер блока в байтах
#define TIME_LIMIT 5     // Ограничение по времени последнего доступа в днях
#define INTERRUPT_COUNT 2 // Количество попыток прерывания, после которых выводится сообщение

static volatile sig_atomic_t interrupt_count = 0; // Счетчик прерываний
static volatile sig_atomic_t total_blocks = 0; // Общее количество блоков

// Обработчик сигнала прерывания
void handle_signal(int sig) {
    interrupt_count++;
    // Для нормального вывода в консоли после получения прерывания
    write(STDOUT_FILENO, "\n", 2);
    if (interrupt_count == INTERRUPT_COUNT) {
        printf("Got interept signal twice. Read blocks: %d\n", total_blocks);
        printf("(To exit enter '!q')\n");
    }
}

// int is_directory(const char *dirpath) {
//     DIR *dir = opendir(dirpath);
//     if (dir) {
//         closedir(dir);
//         return 1; // Это каталог
//     } else {
//         return 0; // Не каталог или ошибка
//     }
// }
// Функция запуска утилиты find в отдельном процессе
void process_files(const char *dirpath) {
    pid_t pid;
    int pipefd[2];
    
    if (pipe(pipefd) == -1) {
        perror("Error creating pipe");
        exit(EXIT_FAILURE);
    }

    if ((pid = fork()) == -1) {
        perror("Error: fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Дочерний процесс
        close(pipefd[0]); // Закрываем чтение
        dup2(pipefd[1], STDOUT_FILENO); // Перенаправляем stdout в pipe
        close(pipefd[1]);

        char size_arg[10];
        char time_arg[10];

        snprintf(size_arg, sizeof(size_arg), "+%dc", BLOCK_SIZE * 3);
        snprintf(time_arg, sizeof(time_arg), "-%d", TIME_LIMIT);
        
        execlp("find", "find", dirpath, "-type", "f", "-atime", time_arg, "-size", size_arg, NULL);
        perror("Error while processing find");
        exit(EXIT_FAILURE);
    } else { // Родительский процесс
        close(pipefd[1]); // Закрываем запись
        char buffer[1024];
        ssize_t bytes_read;

        // Читаем весь вывод из pipe, пока не получим EOF
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';  // Завершаем строку null-символом

            // Теперь работаем с указателем на строку, а не на массив
            char *line_start = buffer;
            char *newline = strchr(line_start, '\n');
            
            while (newline) {
                *newline = '\0'; // Обрезаем символ новой строки
                printf("Found file: %s\n", line_start);  // Выводим найденный файл
                line_start = newline + 1;  // Переходим к следующему пути
                newline = strchr(line_start, '\n');  // Ищем следующую новую строку
            }

            total_blocks += (bytes_read + BLOCK_SIZE - 1) / BLOCK_SIZE;
        }

        if (bytes_read == -1) {
            perror("Error reading from pipe");
        }

        close(pipefd[0]);
        waitpid(pid, NULL, 0); // Дожидаемся завершения
    }
}

int main() {
    char dirpath[1024];
    struct sigaction sigact;

    memset(&sigact, 0, sizeof(struct sigaction));
    sigact.sa_handler = handle_signal;
    sigaction(SIGINT, &sigact, NULL);

    while (1) {
        memset(dirpath, 0, sizeof(dirpath));
        write(STDOUT_FILENO, "Enter path to directory (to exit type '!q'):\n", 46);
        read(STDIN_FILENO, dirpath, sizeof(dirpath));
        dirpath[strcspn(dirpath, "\n")] = '\0'; // Убираем символ новой строки

        if (strcmp(dirpath, "!q") == 0) {
            write(STDOUT_FILENO, "Exiting...\n", 12);
            break;
        }
        process_files(dirpath);
    }
    return EXIT_SUCCESS;
}
