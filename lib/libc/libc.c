#include "libc.h"
#include "../../kernel/mm/mm.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

// Heap management for malloc/free
typedef struct heap_block {
    size_t size;
    int free;
    struct heap_block *next;
} heap_block_t;

static heap_block_t *heap_start = NULL;
static uint8_t *heap_end = NULL;
static const size_t HEAP_SIZE = 16 * 1024 * 1024; // 16MB heap

void *malloc(size_t size) {
    if (size == 0) return NULL;

    // Align to 8 bytes
    size = (size + 7) & ~7;

    // Initialize heap on first call
    if (!heap_start) {
        heap_start = (heap_block_t*)kmalloc(HEAP_SIZE);
        heap_start->size = HEAP_SIZE - sizeof(heap_block_t);
        heap_start->free = 1;
        heap_start->next = NULL;
        heap_end = (uint8_t*)heap_start + HEAP_SIZE;
    }

    // Find free block (first fit)
    heap_block_t *current = heap_start;
    while (current) {
        if (current->free && current->size >= size) {
            // Split block if large enough
            if (current->size >= size + sizeof(heap_block_t) + 8) {
                heap_block_t *new_block = (heap_block_t*)((uint8_t*)current + sizeof(heap_block_t) + size);
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->free = 1;
                new_block->next = current->next;
                current->next = new_block;
                current->size = size;
            }
            current->free = 0;
            return (void*)((uint8_t*)current + sizeof(heap_block_t));
        }
        current = current->next;
    }

    return NULL;
}

void free(void *ptr) {
    if (!ptr) return;

    heap_block_t *block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    block->free = 1;

    // Coalesce with next block
    if (block->next && block->next->free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
    }

    // Coalesce with previous block
    heap_block_t *current = heap_start;
    while (current && current->next != block) {
        current = current->next;
    }
    if (current && current->free) {
        current->size += sizeof(heap_block_t) + block->size;
        current->next = block->next;
    }
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    heap_block_t *block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    if (block->size >= size) return ptr;

    void *new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        free(ptr);
    }
    return new_ptr;
}

// String functions
size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    char *d = dest + strlen(dest);
    size_t i;
    for (i = 0; i < n && src[i]; i++) {
        d[i] = src[i];
    }
    d[i] = '\0';
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }
    return n ? (*(unsigned char*)s1 - *(unsigned char*)s2) : 0;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return (*s == (char)c) ? (char*)s : NULL;
}

char *strrchr(const char *s, int c) {
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    if (*s == (char)c) return (char*)s;
    return (char*)last;
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char*)haystack;

    while (*haystack) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) return (char*)haystack;
        haystack++;
    }
    return NULL;
}

char *strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *new_s = malloc(len);
    if (new_s) {
        memcpy(new_s, s, len);
    }
    return new_s;
}

static char *strtok_saveptr = NULL;

char *strtok(char *str, const char *delim) {
    if (str) strtok_saveptr = str;
    if (!strtok_saveptr) return NULL;

    // Skip leading delimiters
    while (*strtok_saveptr && strchr(delim, *strtok_saveptr)) {
        strtok_saveptr++;
    }

    if (!*strtok_saveptr) return NULL;

    char *token = strtok_saveptr;

    // Find end of token
    while (*strtok_saveptr && !strchr(delim, *strtok_saveptr)) {
        strtok_saveptr++;
    }

    if (*strtok_saveptr) {
        *strtok_saveptr = '\0';
        strtok_saveptr++;
    }

    return token;
}

// Memory functions
void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;

    // Copy 8 bytes at a time if aligned
    if (((uintptr_t)d & 7) == 0 && ((uintptr_t)s & 7) == 0) {
        while (n >= 8) {
            *(uint64_t*)d = *(uint64_t*)s;
            d += 8;
            s += 8;
            n -= 8;
        }
    }

    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;

    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t*)s;

    // Set 8 bytes at a time if aligned
    if (((uintptr_t)p & 7) == 0 && n >= 8) {
        uint64_t val = (uint8_t)c;
        val |= val << 8;
        val |= val << 16;
        val |= val << 32;

        while (n >= 8) {
            *(uint64_t*)p = val;
            p += 8;
            n -= 8;
        }
    }

    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t*)s1;
    const uint8_t *p2 = (const uint8_t*)s2;

    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

void *memchr(const void *s, int c, size_t n) {
    const uint8_t *p = (const uint8_t*)s;
    while (n--) {
        if (*p == (uint8_t)c) return (void*)p;
        p++;
    }
    return NULL;
}

// Printf implementation
static void print_char(char c) {
    // TODO: Write to stdout
    write(1, &c, 1);
}

static void print_string(const char *s) {
    while (*s) print_char(*s++);
}

static void print_number(long long num, int base, int uppercase) {
    if (num < 0 && base == 10) {
        print_char('-');
        num = -num;
    }

    char buf[32];
    int i = 0;

    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num > 0) {
            int digit = num % base;
            if (digit < 10) {
                buf[i++] = '0' + digit;
            } else {
                buf[i++] = (uppercase ? 'A' : 'a') + digit - 10;
            }
            num /= base;
        }
    }

    while (i > 0) {
        print_char(buf[--i]);
    }
}

int vprintf(const char *format, va_list ap) {
    int count = 0;

    while (*format) {
        if (*format == '%') {
            format++;

            // Handle flags
            int zero_pad = 0;
            int width = 0;
            int long_flag = 0;

            if (*format == '0') {
                zero_pad = 1;
                format++;
            }

            while (*format >= '0' && *format <= '9') {
                width = width * 10 + (*format - '0');
                format++;
            }

            if (*format == 'l') {
                long_flag = 1;
                format++;
                if (*format == 'l') {
                    format++;
                }
            }

            switch (*format) {
                case 'd':
                case 'i': {
                    long long val = long_flag ? va_arg(ap, long long) : va_arg(ap, int);
                    print_number(val, 10, 0);
                    break;
                }
                case 'u': {
                    unsigned long long val = long_flag ? va_arg(ap, unsigned long long) : va_arg(ap, unsigned int);
                    print_number(val, 10, 0);
                    break;
                }
                case 'x': {
                    unsigned long long val = long_flag ? va_arg(ap, unsigned long long) : va_arg(ap, unsigned int);
                    print_number(val, 16, 0);
                    break;
                }
                case 'X': {
                    unsigned long long val = long_flag ? va_arg(ap, unsigned long long) : va_arg(ap, unsigned int);
                    print_number(val, 16, 1);
                    break;
                }
                case 'p': {
                    print_string("0x");
                    print_number((unsigned long long)va_arg(ap, void*), 16, 0);
                    break;
                }
                case 's': {
                    const char *s = va_arg(ap, const char*);
                    print_string(s ? s : "(null)");
                    break;
                }
                case 'c': {
                    print_char((char)va_arg(ap, int));
                    break;
                }
                case '%': {
                    print_char('%');
                    break;
                }
            }
            format++;
        } else {
            print_char(*format++);
            count++;
        }
    }

    return count;
}

int printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vprintf(format, ap);
    va_end(ap);
    return ret;
}

int sprintf(char *str, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsprintf(str, format, ap);
    va_end(ap);
    return ret;
}

int vsprintf(char *str, const char *format, va_list ap) {
    // TODO: Implement proper vsprintf
    return 0;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    // TODO: Implement proper vsnprintf
    return 0;
}

// Character functions
int isalpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

int islower(int c) {
    return c >= 'a' && c <= 'z';
}

int toupper(int c) {
    return islower(c) ? c - 32 : c;
}

int tolower(int c) {
    return isupper(c) ? c + 32 : c;
}

// Math functions (basic implementations)
double fabs(double x) {
    return x < 0 ? -x : x;
}

double sqrt(double x) {
    if (x < 0) return 0;
    if (x == 0) return 0;

    double guess = x / 2.0;
    double epsilon = 0.00001;

    while (fabs(guess * guess - x) > epsilon) {
        guess = (guess + x / guess) / 2.0;
    }

    return guess;
}

double pow(double x, double y) {
    if (y == 0) return 1;
    if (y == 1) return x;

    double result = 1;
    int yi = (int)y;

    for (int i = 0; i < yi; i++) {
        result *= x;
    }

    return result;
}

double floor(double x) {
    long long i = (long long)x;
    return (double)(x < i ? i - 1 : i);
}

double ceil(double x) {
    long long i = (long long)x;
    return (double)(x > i ? i + 1 : i);
}

// Conversion functions
int atoi(const char *nptr) {
    int result = 0;
    int sign = 1;

    while (isspace(*nptr)) nptr++;

    if (*nptr == '-') {
        sign = -1;
        nptr++;
    } else if (*nptr == '+') {
        nptr++;
    }

    while (isdigit(*nptr)) {
        result = result * 10 + (*nptr - '0');
        nptr++;
    }

    return sign * result;
}

long atol(const char *nptr) {
    return (long)atoi(nptr);
}

long long atoll(const char *nptr) {
    return (long long)atoi(nptr);
}

long strtol(const char *nptr, char **endptr, int base) {
    long result = 0;
    int sign = 1;

    while (isspace(*nptr)) nptr++;

    if (*nptr == '-') {
        sign = -1;
        nptr++;
    } else if (*nptr == '+') {
        nptr++;
    }

    if (base == 0) {
        if (*nptr == '0') {
            nptr++;
            if (*nptr == 'x' || *nptr == 'X') {
                base = 16;
                nptr++;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    while (*nptr) {
        int digit;
        if (isdigit(*nptr)) {
            digit = *nptr - '0';
        } else if (isalpha(*nptr)) {
            digit = tolower(*nptr) - 'a' + 10;
        } else {
            break;
        }

        if (digit >= base) break;

        result = result * base + digit;
        nptr++;
    }

    if (endptr) *endptr = (char*)nptr;
    return sign * result;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
    return (unsigned long)strtol(nptr, endptr, base);
}

// Random number generator
static unsigned long rand_seed = 1;

void srand(unsigned int seed) {
    rand_seed = seed;
}

int rand(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (int)((rand_seed / 65536) % 32768);
}

// Error handling
int errno = 0;

char *strerror(int errnum) {
    static char buf[32];
    sprintf(buf, "Error %d", errnum);
    return buf;
}

void perror(const char *s) {
    if (s && *s) {
        printf("%s: %s\n", s, strerror(errno));
    } else {
        printf("%s\n", strerror(errno));
    }
}
