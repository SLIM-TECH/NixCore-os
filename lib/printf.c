#include "../include/kernel.h"
#include "../include/string.h"

static char* itoa(int value, char* str, int base) {
    char* rc;
    char* ptr;
    char* low;
    
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    rc = ptr = str;
    
    if (value < 0 && base == 10) {
        *ptr++ = '-';
        value = -value;
    }
    
    low = ptr;
    
    do {
        int digit = value % base;
        *ptr++ = (digit < 10) ? '0' + digit : 'a' + digit - 10;
        value /= base;
    } while (value);
    
    *ptr-- = '\0';
    
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    
    return rc;
}

static char* uitoa(unsigned int value, char* str, int base) {
    char* rc;
    char* ptr;
    char* low;
    
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    rc = ptr = str;
    low = ptr;
    
    do {
        unsigned int digit = value % base;
        *ptr++ = (digit < 10) ? '0' + digit : 'a' + digit - 10;
        value /= base;
    } while (value);
    
    *ptr-- = '\0';
    
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    
    return rc;
}

int vsprintf(char* buffer, const char* format, va_list args) {
    char* ptr = buffer;
    char temp[32];
    
    for (; *format != '\0'; format++) {
        if (*format == '%') {
            format++;
            
            switch (*format) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    *ptr++ = c;
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (!s) s = "(null)";
                    while (*s) {
                        *ptr++ = *s++;
                    }
                    break;
                }
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    itoa(value, temp, 10);
                    char* s = temp;
                    while (*s) {
                        *ptr++ = *s++;
                    }
                    break;
                }
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    uitoa(value, temp, 10);
                    char* s = temp;
                    while (*s) {
                        *ptr++ = *s++;
                    }
                    break;
                }
                case 'x': {
                    unsigned int value = va_arg(args, unsigned int);
                    uitoa(value, temp, 16);
                    char* s = temp;
                    while (*s) {
                        *ptr++ = *s++;
                    }
                    break;
                }
                case '%': {
                    *ptr++ = '%';
                    break;
                }
                default: {
                    *ptr++ = '%';
                    *ptr++ = *format;
                    break;
                }
            }
        } else {
            *ptr++ = *format;
        }
    }
    
    *ptr = '\0';
    return ptr - buffer;
}

int sprintf(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsprintf(buffer, format, args);
    va_end(args);
    return result;
}
