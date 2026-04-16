#ifndef LIBC_H
#define LIBC_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

// Standard types
typedef long ssize_t;
typedef long off_t;
typedef unsigned long mode_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef long time_t;
typedef long pid_t;

// File operations
int open(const char *pathname, int flags, ...);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
int ioctl(int fd, unsigned long request, ...);
int fcntl(int fd, int cmd, ...);

// Directory operations
int mkdir(const char *pathname, mode_t mode);
int rmdir(const char *pathname);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);

// File info
struct stat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint64_t st_size;
    uint64_t st_blksize;
    uint64_t st_blocks;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
};

int stat(const char *pathname, struct stat *statbuf);
int fstat(int fd, struct stat *statbuf);
int lstat(const char *pathname, struct stat *statbuf);

// Process management
pid_t fork(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);
int execv(const char *pathname, char *const argv[]);
int execl(const char *pathname, ...);
void exit(int status);
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
pid_t getpid(void);
pid_t getppid(void);

// Memory management
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
int brk(void *addr);
void *sbrk(intptr_t increment);

// String functions
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);
char *strdup(const char *s);
char *strtok(char *str, const char *delim);

// Memory functions
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);

// Formatted I/O
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int vprintf(const char *format, va_list ap);
int vfprintf(FILE *stream, const char *format, va_list ap);
int vsprintf(char *str, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

int scanf(const char *format, ...);
int fscanf(FILE *stream, const char *format, ...);
int sscanf(const char *str, const char *format, ...);

// Character functions
int isalpha(int c);
int isdigit(int c);
int isalnum(int c);
int isspace(int c);
int isupper(int c);
int islower(int c);
int toupper(int c);
int tolower(int c);

// Math functions
double sin(double x);
double cos(double x);
double tan(double x);
double sqrt(double x);
double pow(double x, double y);
double exp(double x);
double log(double x);
double fabs(double x);
double floor(double x);
double ceil(double x);

// Time functions
time_t time(time_t *tloc);
struct tm *localtime(const time_t *timep);
struct tm *gmtime(const time_t *timep);
char *asctime(const struct tm *tm);
char *ctime(const time_t *timep);
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
int nanosleep(const struct timespec *req, struct timespec *rem);
unsigned int sleep(unsigned int seconds);
int usleep(useconds_t usec);

// Signal handling
typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);
int kill(pid_t pid, int sig);
int raise(int sig);

// Threads (POSIX)
typedef struct pthread pthread_t;
typedef struct pthread_attr pthread_attr_t;
typedef struct pthread_mutex pthread_mutex_t;
typedef struct pthread_cond pthread_cond_t;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **retval);
void pthread_exit(void *retval);
pthread_t pthread_self(void);
int pthread_mutex_init(pthread_mutex_t *mutex, const void *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_cond_init(pthread_cond_t *cond, const void *attr);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_destroy(pthread_cond_t *cond);

// Environment
char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);

// Random
int rand(void);
void srand(unsigned int seed);

// Conversion
int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);
double atof(const char *nptr);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);

// Error handling
extern int errno;
char *strerror(int errnum);
void perror(const char *s);

#endif
