#include "../include/string.h"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*dest++ = *src++));
    return d;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*dest)
        dest++;
    while ((*dest++ = *src++));
    return d;
}

char* strncat(char* dest, const char* src, size_t n) {
    size_t dest_len = strlen(dest);
    size_t i;
    
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[dest_len + i] = src[i];
    dest[dest_len + i] = '\0';
    
    return dest;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

int strncmp(const char* str1, const char* str2, size_t n) {
    if (n == 0)
        return 0;
    
    while (n-- && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

char* strchr(const char* str, int c) {
    while (*str != '\0') {
        if (*str == c)
            return (char*)str;
        str++;
    }
    return NULL;
}

char* strrchr(const char* str, int c) {
    const char* last = NULL;
    
    while (*str != '\0') {
        if (*str == c)
            last = str;
        str++;
    }
    
    return (char*)last;
}

char* strstr(const char* haystack, const char* needle) {
    size_t needle_len = strlen(needle);
    if (needle_len == 0)
        return (char*)haystack;
    
    while (*haystack) {
        if (*haystack == *needle && strncmp(haystack, needle, needle_len) == 0)
            return (char*)haystack;
        haystack++;
    }
    
    return NULL;
}

char* strtok(char* str, const char* delim) {
    static char* last = NULL;
    char* token;
    
    if (str == NULL)
        str = last;
    
    str += strspn(str, delim);
    if (*str == '\0') {
        last = str;
        return NULL;
    }
    
    token = str;
    str = strpbrk(token, delim);
    if (str == NULL) {
        last = strchr(token, '\0');
    } else {
        *str = '\0';
        last = str + 1;
    }
    
    return token;
}

size_t strspn(const char* str, const char* accept) {
    const char* s = str;
    
    while (*str) {
        const char* a = accept;
        while (*a && *a != *str)
            a++;
        if (*a == '\0')
            break;
        str++;
    }
    
    return str - s;
}

char* strpbrk(const char* str, const char* accept) {
    while (*str) {
        const char* a = accept;
        while (*a) {
            if (*a++ == *str)
                return (char*)str;
        }
        str++;
    }
    return NULL;
}

char utf8_to_ascii(const char** text) {
    const unsigned char* s = (const unsigned char*)(*text);
    unsigned int codepoint = 0;

    if (*s == '\0') {
        return '\0';
    }

    if (s[0] < 0x80) {
        (*text)++;
        return (char)s[0];
    }

    if ((s[0] & 0xE0) == 0xC0 && s[1] != '\0') {
        codepoint = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        *text += 2;
    } else if ((s[0] & 0xF0) == 0xE0 && s[1] != '\0' && s[2] != '\0') {
        codepoint = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        *text += 3;
    } else if ((s[0] & 0xF8) == 0xF0 && s[1] != '\0' && s[2] != '\0' && s[3] != '\0') {
        codepoint = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) |
                    ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        *text += 4;
    } else {
        (*text)++;
        return '?';
    }

    switch (codepoint) {
        case 0x2500:
        case 0x2501:
        case 0x2504:
        case 0x2505:
        case 0x2550:
            return '-';
        case 0x2502:
        case 0x2503:
        case 0x2506:
        case 0x2507:
        case 0x2551:
            return '|';
        case 0x250C:
        case 0x2510:
        case 0x2514:
        case 0x2518:
        case 0x251C:
        case 0x2524:
        case 0x252C:
        case 0x2534:
        case 0x253C:
        case 0x2554:
        case 0x2557:
        case 0x255A:
        case 0x255D:
        case 0x2560:
        case 0x2563:
        case 0x2566:
        case 0x2569:
        case 0x256C:
            return '+';
        case 0x2580:
        case 0x2584:
        case 0x2588:
        case 0x2591:
        case 0x2592:
        case 0x2593:
            return '#';
        case 0x00AB:
            return '<';
        case 0x00BB:
            return '>';
        case 0x2013:
        case 0x2014:
            return '-';
        case 0x2018:
        case 0x2019:
        case 0x2032:
            return '\'';
        case 0x201C:
        case 0x201D:
        case 0x2033:
            return '"';
        case 0x2022:
            return '*';
        case 0x2026:
            return '.';
        default:
            break;
    }

    if (codepoint >= 0x0410 && codepoint <= 0x044F) {
        static const char cyrillic_map[] = {
            'A','B','V','G','D','E','E','Z','Z','I','I','K','L','M','N','O',
            'P','R','S','T','U','F','H','C','C','S','S','\'','Y','\'','E','U',
            'A','B','V','G','D','E','e','z','z','i','i','k','l','m','n','o',
            'p','r','s','t','u','f','h','c','c','s','s','\'','y','\'','e','u'
        };
        unsigned int index = codepoint - 0x0410;
        if (index < sizeof(cyrillic_map)) {
            return cyrillic_map[index];
        }
    }

    return '?';
}
