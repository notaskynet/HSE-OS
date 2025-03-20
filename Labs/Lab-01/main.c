#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h> 

#define BUFFER_SIZE     1024 // Размер буффера для считывания файлов
#define INTERRUPT_COUNT 2    // Количество попыток прерывания, после которых выводится сообщение

int interrupt_count = 0; // Счетчик прерываний
const char *script_path = "/Users/daspiridonov/Projects/HSE-OS/Labs/Lab-01/count_blocks.sh";
char lastdir[BUFFER_SIZE];

int count_blocks(char dir[]) {
    int pipefd[2];

    // Проверка существования директории
    if (access(dir, F_OK) != 0) {
        perror("No such directory.");
        return -1;
    }

    // Создание pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Дочерний процесс
        close(pipefd[0]);  // Закрываем чтение в дочернем процессе
        dup2(pipefd[1], STDOUT_FILENO);  // Перенаправляем stdout в pipe
        close(pipefd[1]);  // Закрываем запись после перенаправления

        // Запуск скрипта в дочернем процессе
        execl("/bin/sh", "sh", script_path, dir, (char *)NULL);
        perror("execl");  // Если execl не выполнится
        exit(EXIT_FAILURE);
    } else {  // Родительский процесс
        close(pipefd[1]);  // Закрываем запись в родительском процессе

        // Ожидаем завершения дочернего процесса
        waitpid(pid, NULL, 0); 

        // Чтение данных из pipe
        char buffer[32];  
        ssize_t count = read(pipefd[0], buffer, sizeof(buffer) - 1);
        close(pipefd[0]);  // Закрываем чтение после завершения работы с ним

        if (count > 0) {
            buffer[count] = '\0';  // Завершаем строку
            return atoi(buffer);   // Конвертируем в число
        } else {
            perror("read");
            return -1;
        }
    }
}

// Обработчик сигнала прерывания
void handle_signal(int sig) {
    int total_blocks;

    interrupt_count++;
    // Для нормального вывода в консоли после получения прерывания
    write(STDOUT_FILENO, "\n", 2);
    if (interrupt_count == INTERRUPT_COUNT) {
        total_blocks = count_blocks(lastdir);
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

        char buffer[BUFFER_SIZE];
        long bytes_read;
        
        // Очищаем буффер
        memset(buffer, 0, BUFFER_SIZE);

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
        }

        if (bytes_read == -1) {
            perror("Error reading from pipe");
        }
        close(pipefd[0]);
    }
}


int main() {
    char dirpath[BUFFER_SIZE];
    struct sigaction sigact;

    memset(&sigact, 0, sizeof(struct sigaction));
    sigprocmask(0, 0, &sigact.sa_mask);
    sigact.sa_handler = handle_signal;
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
            memcpy(lastdir, dirpath, BUFFER_SIZE);
            printf("%s\n", lastdir);
            printf("%s\n", dirpath);
            process_files(dirpath);
        }
        else {
            write(STDOUT_FILENO, "Error: No such directory. Try again!\n", 38);
            continue;     
        }
    }
    return EXIT_SUCCESS;
}
