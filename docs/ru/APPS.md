# Разработка приложений для NixCore

## Начало работы

NixCore предоставляет POSIX-совместимую среду для разработки пользовательских приложений. Вы можете писать программы на C/C++, используя стандартную библиотеку и системные вызовы.

## Hello World

```c
#include <stdio.h>

int main() {
    printf("Привет, NixCore!\n");
    return 0;
}
```

Компиляция:
```bash
gcc -o hello hello.c
./hello
```

## Системные вызовы

### Операции с файлами

```c
#include <fcntl.h>
#include <unistd.h>

int main() {
    // Открыть файл
    int fd = open("/home/user/file.txt", O_RDONLY, 0);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    // Прочитать файл
    char buffer[1024];
    ssize_t bytes = read(fd, buffer, sizeof(buffer));
    
    // Записать в stdout
    write(1, buffer, bytes);
    
    // Закрыть файл
    close(fd);
    
    return 0;
}
```

### Управление процессами

```c
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний PID: %d\n", getpid());
        execl("/bin/ls", "ls", "-la", NULL);
    } else {
        // Родительский процесс
        printf("Родительский PID: %d\n", getpid());
        int status;
        wait(&status);
        printf("Дочерний процесс завершился со статусом %d\n", status);
    }
    
    return 0;
}
```

### Сетевое программирование

```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    // Создать сокет
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Подключиться к серверу
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = inet_addr("93.184.216.34"); // example.com
    
    connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    
    // Отправить HTTP запрос
    const char *request = "GET / HTTP/1.0\r\n\r\n";
    send(sockfd, request, strlen(request), 0);
    
    // Получить ответ
    char buffer[4096];
    ssize_t bytes = recv(sockfd, buffer, sizeof(buffer), 0);
    write(1, buffer, bytes);
    
    close(sockfd);
    return 0;
}
```

## GUI приложения

### Создание окна

```c
#include <gui.h>

int main() {
    // Инициализировать GUI
    gui_init(1280, 720);
    
    // Создать окно
    window_t *win = window_create("Моё приложение", 100, 100, 800, 600, 0);
    
    // Рисовать в framebuffer окна
    for (int y = 0; y < 600; y++) {
        for (int x = 0; x < 800; x++) {
            win->framebuffer[y * 800 + x] = 0xFFFFFFFF; // Белый
        }
    }
    
    // Показать окно
    window_show(win);
    
    // Цикл событий
    while (1) {
        compositor_render();
        scheduler_yield();
    }
    
    return 0;
}
```

### Обработка событий

```c
#include <gui.h>

void on_mouse_click(int x, int y, int button) {
    printf("Клик мыши в (%d, %d) кнопка %d\n", x, y, button);
}

void on_key_press(int key) {
    printf("Нажата клавиша: %d\n", key);
}

int main() {
    gui_init(1280, 720);
    window_t *win = window_create("Демо событий", 100, 100, 800, 600, 0);
    
    // Зарегистрировать обработчики событий
    win->on_mouse_click = on_mouse_click;
    win->on_key_press = on_key_press;
    
    window_show(win);
    
    while (1) {
        gui_process_events();
        compositor_render();
    }
    
    return 0;
}
```

## Многопоточность

```c
#include <pthread.h>
#include <stdio.h>

void *thread_func(void *arg) {
    int id = *(int*)arg;
    printf("Поток %d запущен\n", id);
    return NULL;
}

int main() {
    pthread_t threads[4];
    int ids[4] = {1, 2, 3, 4};
    
    // Создать потоки
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, thread_func, &ids[i]);
    }
    
    // Дождаться потоков
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}
```

## Межпроцессное взаимодействие

### Каналы (Pipes)

```c
#include <unistd.h>

int main() {
    int pipefd[2];
    pipe(pipefd);
    
    if (fork() == 0) {
        // Дочерний: записать в канал
        close(pipefd[0]);
        const char *msg = "Привет от дочернего!";
        write(pipefd[1], msg, strlen(msg));
        close(pipefd[1]);
    } else {
        // Родительский: прочитать из канала
        close(pipefd[1]);
        char buffer[100];
        ssize_t bytes = read(pipefd[0], buffer, sizeof(buffer));
        write(1, buffer, bytes);
        close(pipefd[0]);
    }
    
    return 0;
}
```

### Разделяемая память

```c
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
    // Создать разделяемую память
    int fd = shm_open("/myshm", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, 4096);
    
    // Отобразить в адресное пространство
    void *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    // Записать данные
    sprintf(ptr, "Разделяемые данные!");
    
    // Отменить отображение и закрыть
    munmap(ptr, 4096);
    close(fd);
    
    return 0;
}
```

## Операции с файловой системой

### Список каталога

```c
#include <dirent.h>

int main() {
    DIR *dir = opendir("/home");
    if (!dir) {
        perror("opendir");
        return 1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }
    
    closedir(dir);
    return 0;
}
```

### Информация о файле

```c
#include <sys/stat.h>

int main() {
    struct stat st;
    if (stat("/home/user/file.txt", &st) == 0) {
        printf("Размер: %ld байт\n", st.st_size);
        printf("Режим: %o\n", st.st_mode);
        printf("UID: %d\n", st.st_uid);
        printf("GID: %d\n", st.st_gid);
    }
    
    return 0;
}
```

## Сборка приложений

### Пример Makefile

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lc -lgui -lpthread

SOURCES = main.c utils.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = myapp

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install:
	cp $(TARGET) /usr/bin/

.PHONY: all clean install
```

### Сборка и установка

```bash
# Собрать
make

# Установить
sudo make install

# Запустить
myapp
```

## Отладка

### Использование GDB

```bash
# Скомпилировать с отладочными символами
gcc -g -o myapp main.c

# Запустить в GDB
gdb myapp
(gdb) break main
(gdb) run
(gdb) next
(gdb) print variable
(gdb) backtrace
```

### Отладка через printf

```c
#define DEBUG 1

#ifdef DEBUG
#define DPRINTF(fmt, ...) \
    fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DPRINTF(fmt, ...)
#endif

int main() {
    DPRINTF("Запуск приложения");
    int x = 42;
    DPRINTF("x = %d", x);
    return 0;
}
```

## Лучшие практики

### Обработка ошибок

```c
#include <errno.h>

int main() {
    int fd = open("/несуществующий", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Ошибка открытия файла: %s\n", strerror(errno));
        return 1;
    }
    
    // Использовать файл...
    
    close(fd);
    return 0;
}
```

### Управление памятью

```c
int main() {
    // Выделить память
    char *buffer = malloc(1024);
    if (!buffer) {
        fprintf(stderr, "Нехватка памяти\n");
        return 1;
    }
    
    // Использовать buffer...
    
    // Всегда освобождать!
    free(buffer);
    
    return 0;
}
```

### Очистка ресурсов

```c
int main() {
    FILE *fp = fopen("file.txt", "r");
    if (!fp) return 1;
    
    int fd = open("another.txt", O_RDONLY);
    if (fd < 0) {
        fclose(fp);  // Очистить первый ресурс
        return 1;
    }
    
    // Использовать ресурсы...
    
    close(fd);
    fclose(fp);
    
    return 0;
}
```

## Примеры приложений

### Простой текстовый редактор

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 1000
#define MAX_LINE_LEN 256

char *lines[MAX_LINES];
int num_lines = 0;

void load_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return;
    
    char buffer[MAX_LINE_LEN];
    while (fgets(buffer, sizeof(buffer), fp) && num_lines < MAX_LINES) {
        lines[num_lines++] = strdup(buffer);
    }
    
    fclose(fp);
}

void save_file(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    
    for (int i = 0; i < num_lines; i++) {
        fputs(lines[i], fp);
    }
    
    fclose(fp);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Использование: %s <имя_файла>\n", argv[0]);
        return 1;
    }
    
    load_file(argv[1]);
    
    // Цикл редактирования...
    
    save_file(argv[1]);
    
    return 0;
}
```

### Сетевой чат-клиент

```c
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

void *receive_thread(void *arg) {
    int sockfd = *(int*)arg;
    char buffer[1024];
    
    while (1) {
        ssize_t bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;
        
        buffer[bytes] = '\0';
        printf("%s", buffer);
    }
    
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Использование: %s <хост> <порт>\n", argv[0]);
        return 1;
    }
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    
    pthread_t thread;
    pthread_create(&thread, NULL, receive_thread, &sockfd);
    
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), stdin)) {
        send(sockfd, buffer, strlen(buffer), 0);
    }
    
    close(sockfd);
    return 0;
}
```

## Справочник API

См. [API.md](API.md) для полной документации API.

## Решение проблем

### Segmentation Fault

- Проверить границы массивов
- Убедиться, что указатели не NULL
- Использовать valgrind (если доступен)

### Undefined Reference

- Подключить необходимые библиотеки: `-lc -lgui -lpthread`
- Проверить имена функций и заголовки

### Permission Denied

- Проверить права доступа к файлам
- Запустить с соответствующими привилегиями
- Проверить правильность путей к файлам
