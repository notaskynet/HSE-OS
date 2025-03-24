#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <sys/stat.h>

#define BUFFER_SIZE     1024 // Размер буффера для считывания файлов
#define INTERRUPT_COUNT 2    // Количество попыток прерывания, после которых выводится сообщение

int interrupt_count = 0; // Счетчик прерываний
char buffer[BUFFER_SIZE];

// Подсчет количества блоков в одном файле
long get_file_blocks(const char* filepath) {
    struct stat file_stat;
    
    if (stat(filepath, &file_stat) == -1) {
        perror("stat");
        return 0;
    }
    return file_stat.st_blocks;
}

// Подсчет количества блоков в буффере
long count_blocks_from_buffer() {
    char *line_start = buffer;
    char *newline = strchr(line_start, '\n');
    long total_blocks = 0;
    
    while (newline) {
        *newline = '\0'; // Обрезаем символ новой строки
        total_blocks += get_file_blocks(line_start); // Считаем количество блоков
        line_start = newline + 1; // Переходим к следующему пути
        newline = strchr(line_start, '\n'); // Ищем следующую новую строку
    }
    return total_blocks;
}

// Обработчик сигнала прерывания
void handle_signal(int sig) {
    int total_blocks;

    interrupt_count++;
    // Для нормального вывода в консоли после получения прерывания
    write(STDOUT_FILENO, "\n", 2);
    if (interrupt_count == INTERRUPT_COUNT) {
        total_blocks = count_blocks_from_buffer();
        printf("Got interept signal twice. Read blocks: %d\n", total_blocks);
    }
    write(STDOUT_FILENO, "Enter `!q` to quit\n", 41);
}

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
        execlp("find", "find", dirpath, "-type", "f", "-atime", "-5d", "-size", "+3", NULL);
        perror("Error while processing find");
        exit(EXIT_FAILURE);
    } else { // Родительский процесс
        waitpid(pid, NULL, 0); // Дожидаемся завершения
        close(pipefd[1]); // Закрываем запись

        ssize_t bytes_read = read(pipefd[0], buffer, BUFFER_SIZE - 1);
        if (bytes_read == -1) {
            perror("Error reading from pipe");
        } else {
            buffer[bytes_read] = '\0'; // Завершаем строку null-символом
        }
        close(pipefd[0]);
    }
}

// Вывод файлов
void print_files_from_buffer() {
    char *line_start = buffer;
    char *newline = strchr(line_start, '\n');
    
    while (newline) {
        *newline = '\0'; // Обрезаем символ новой строки
        printf("Found file: %s\n", line_start);
        line_start = newline + 1; // Переходим к следующему пути
        newline = strchr(line_start, '\n'); // Ищем следующую новую строку
    }
}


int main() {
    char dirpath[BUFFER_SIZE];
    struct sigaction sigact;

    memset(&sigact, 0, sizeof(struct sigaction));
    sigact.sa_handler = handle_signal;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, NULL);

    while (1) {
        memset(dirpath, 0, BUFFER_SIZE);

        write(STDOUT_FILENO, "Enter path to directory (to exit type '!q'):\n", 46);
        
        if (read(STDIN_FILENO, dirpath, sizeof(dirpath)) == -1) {
            write(STDERR_FILENO, "\nError uccured while reading path. Try again!\n", 47);
            continue;
        }
        
        dirpath[strcspn(dirpath, "\n")] = '\0'; // Убираем символ новой строки
    
        if (strcmp(dirpath, "!q") == 0) {
            write(STDOUT_FILENO, "Exiting...\n", 12);
            break;
        }
    
        if (access(dirpath, F_OK) == 0) {
            process_files(dirpath);
        }
        else {
            write(STDOUT_FILENO, "Error: No such directory. Try again!\n", 38);
            continue;     
        }
    }
    return EXIT_SUCCESS;
}
