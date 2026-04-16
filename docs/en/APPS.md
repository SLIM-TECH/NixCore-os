# Application Development for NixCore

## Getting Started

NixCore provides a POSIX-compatible environment for developing userspace applications. You can write programs in C/C++ using the standard library and system calls.

## Hello World

```c
#include <stdio.h>

int main() {
    printf("Hello, NixCore!\n");
    return 0;
}
```

Compile:
```bash
gcc -o hello hello.c
./hello
```

## System Calls

### File Operations

```c
#include <fcntl.h>
#include <unistd.h>

int main() {
    // Open file
    int fd = open("/home/user/file.txt", O_RDONLY, 0);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    // Read file
    char buffer[1024];
    ssize_t bytes = read(fd, buffer, sizeof(buffer));
    
    // Write to stdout
    write(1, buffer, bytes);
    
    // Close file
    close(fd);
    
    return 0;
}
```

### Process Management

```c
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        printf("Child PID: %d\n", getpid());
        execl("/bin/ls", "ls", "-la", NULL);
    } else {
        // Parent process
        printf("Parent PID: %d\n", getpid());
        int status;
        wait(&status);
        printf("Child exited with status %d\n", status);
    }
    
    return 0;
}
```

### Network Programming

```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Connect to server
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = inet_addr("93.184.216.34"); // example.com
    
    connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    
    // Send HTTP request
    const char *request = "GET / HTTP/1.0\r\n\r\n";
    send(sockfd, request, strlen(request), 0);
    
    // Receive response
    char buffer[4096];
    ssize_t bytes = recv(sockfd, buffer, sizeof(buffer), 0);
    write(1, buffer, bytes);
    
    close(sockfd);
    return 0;
}
```

## GUI Applications

### Creating a Window

```c
#include <gui.h>

int main() {
    // Initialize GUI
    gui_init(1280, 720);
    
    // Create window
    window_t *win = window_create("My App", 100, 100, 800, 600, 0);
    
    // Draw to window framebuffer
    for (int y = 0; y < 600; y++) {
        for (int x = 0; x < 800; x++) {
            win->framebuffer[y * 800 + x] = 0xFFFFFFFF; // White
        }
    }
    
    // Show window
    window_show(win);
    
    // Event loop
    while (1) {
        compositor_render();
        scheduler_yield();
    }
    
    return 0;
}
```

### Handling Events

```c
#include <gui.h>

void on_mouse_click(int x, int y, int button) {
    printf("Mouse clicked at (%d, %d) button %d\n", x, y, button);
}

void on_key_press(int key) {
    printf("Key pressed: %d\n", key);
}

int main() {
    gui_init(1280, 720);
    window_t *win = window_create("Event Demo", 100, 100, 800, 600, 0);
    
    // Register event handlers
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

## Threading

```c
#include <pthread.h>
#include <stdio.h>

void *thread_func(void *arg) {
    int id = *(int*)arg;
    printf("Thread %d running\n", id);
    return NULL;
}

int main() {
    pthread_t threads[4];
    int ids[4] = {1, 2, 3, 4};
    
    // Create threads
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, thread_func, &ids[i]);
    }
    
    // Wait for threads
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}
```

## Inter-Process Communication

### Pipes

```c
#include <unistd.h>

int main() {
    int pipefd[2];
    pipe(pipefd);
    
    if (fork() == 0) {
        // Child: write to pipe
        close(pipefd[0]);
        const char *msg = "Hello from child!";
        write(pipefd[1], msg, strlen(msg));
        close(pipefd[1]);
    } else {
        // Parent: read from pipe
        close(pipefd[1]);
        char buffer[100];
        ssize_t bytes = read(pipefd[0], buffer, sizeof(buffer));
        write(1, buffer, bytes);
        close(pipefd[0]);
    }
    
    return 0;
}
```

### Shared Memory

```c
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
    // Create shared memory
    int fd = shm_open("/myshm", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, 4096);
    
    // Map to address space
    void *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    // Write data
    sprintf(ptr, "Shared data!");
    
    // Unmap and close
    munmap(ptr, 4096);
    close(fd);
    
    return 0;
}
```

## File System Operations

### Directory Listing

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

### File Information

```c
#include <sys/stat.h>

int main() {
    struct stat st;
    if (stat("/home/user/file.txt", &st) == 0) {
        printf("Size: %ld bytes\n", st.st_size);
        printf("Mode: %o\n", st.st_mode);
        printf("UID: %d\n", st.st_uid);
        printf("GID: %d\n", st.st_gid);
    }
    
    return 0;
}
```

## Building Applications

### Makefile Example

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

### Build and Install

```bash
# Build
make

# Install
sudo make install

# Run
myapp
```

## Debugging

### Using GDB

```bash
# Compile with debug symbols
gcc -g -o myapp main.c

# Run in GDB
gdb myapp
(gdb) break main
(gdb) run
(gdb) next
(gdb) print variable
(gdb) backtrace
```

### Printf Debugging

```c
#define DEBUG 1

#ifdef DEBUG
#define DPRINTF(fmt, ...) \
    fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DPRINTF(fmt, ...)
#endif

int main() {
    DPRINTF("Starting application");
    int x = 42;
    DPRINTF("x = %d", x);
    return 0;
}
```

## Best Practices

### Error Handling

```c
#include <errno.h>

int main() {
    int fd = open("/nonexistent", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error opening file: %s\n", strerror(errno));
        return 1;
    }
    
    // Use file...
    
    close(fd);
    return 0;
}
```

### Memory Management

```c
int main() {
    // Allocate memory
    char *buffer = malloc(1024);
    if (!buffer) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }
    
    // Use buffer...
    
    // Always free!
    free(buffer);
    
    return 0;
}
```

### Resource Cleanup

```c
int main() {
    FILE *fp = fopen("file.txt", "r");
    if (!fp) return 1;
    
    int fd = open("another.txt", O_RDONLY);
    if (fd < 0) {
        fclose(fp);  // Clean up first resource
        return 1;
    }
    
    // Use resources...
    
    close(fd);
    fclose(fp);
    
    return 0;
}
```

## Example Applications

### Simple Text Editor

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
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    load_file(argv[1]);
    
    // Edit loop...
    
    save_file(argv[1]);
    
    return 0;
}
```

### Network Chat Client

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
        printf("Usage: %s <host> <port>\n", argv[0]);
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

## API Reference

See [API.md](API.md) for complete API documentation.

## Troubleshooting

### Segmentation Fault

- Check array bounds
- Verify pointers are not NULL
- Use valgrind (if available)

### Undefined Reference

- Link required libraries: `-lc -lgui -lpthread`
- Check function names and headers

### Permission Denied

- Check file permissions
- Run with appropriate privileges
- Verify file paths are correct
