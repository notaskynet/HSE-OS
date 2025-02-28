#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>

#define BLOCK_SIZE 4096
#define TIME_LIMIT 5

int total_blocks = 0;
int interrupt_count = 0;

void handle_signal(int sig) {
    interrupt_count++;
    if (interrupt_count == 2) {
        printf("\nСигнал прерывания (Ctrl+C) получен дважды. Общее количество блоков: %d\n", total_blocks);
        printf("(Для завершения программы введите '!q')\n");
    }
}

int is_recent_file(const struct stat *file_stat) {
    time_t current_time = time(NULL);
    double diff_in_days = difftime(current_time, file_stat->st_atime) / (60 * 60 * 24);
    return diff_in_days < TIME_LIMIT;
}

void process_directory(const char *dir_path) {
    struct dirent *entry;
    struct stat file_stat;
    DIR *dir;

    if ((dir = opendir(dir_path)) == NULL) {
        perror("Не удалось открыть каталог");
        return;
    }

    printf("Список файлов размером более 3 блоков и измененных менее 5 дней назад:\n");

    while ((entry = readdir(dir)) != NULL) {
        char file_path[1024];
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

        if (stat(file_path, &file_stat) == -1) {
            perror("Не удалось получить информацию о файле");
            continue;
        }

        if (S_ISREG(file_stat.st_mode) && file_stat.st_blocks > 3 && is_recent_file(&file_stat)) {
            printf("%s\n", entry->d_name);
            total_blocks += file_stat.st_blocks;
        }
    }

    closedir(dir);
}

int main() {
    char dir_path[1024];
    struct sigaction sigact;

    memset(&sigact, 0, sizeof(struct sigaction));
    sigact.sa_handler = handle_signal;
    sigprocmask(0, 0, &sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction *)NULL); 

    while (1) {
        printf("\nВведите путь к каталогу (для выхода введите '!q'): ");
        if (fgets(dir_path, sizeof(dir_path), stdin) == NULL) {
            printf("Ошибка ввода.\n");
            continue;
        }

        dir_path[strcspn(dir_path, "\n")] = '\0';

        if (strcmp(dir_path, "!q") == 0) {
            printf("Выход из программы.\n");
            break;
        }

        process_directory(dir_path);
    }

    return 0;
}