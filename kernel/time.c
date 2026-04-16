#include "../kernel/net/socket.h"
#include "../kernel/mm/mm.h"
#include <stdint.h>

// NTP (Network Time Protocol) client
typedef struct {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_timestamp_sec;
    uint32_t ref_timestamp_frac;
    uint32_t orig_timestamp_sec;
    uint32_t orig_timestamp_frac;
    uint32_t rx_timestamp_sec;
    uint32_t rx_timestamp_frac;
    uint32_t tx_timestamp_sec;
    uint32_t tx_timestamp_frac;
} __attribute__((packed)) ntp_packet_t;

// Time structure
typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int timezone_offset; // Offset from UTC in minutes
} datetime_t;

static uint64_t system_time_ms = 0; // Milliseconds since boot
static int64_t unix_timestamp = 0;  // Unix timestamp (seconds since 1970-01-01)
static int timezone_offset_minutes = 0; // Timezone offset from UTC

// Convert Unix timestamp to datetime
void unix_to_datetime(int64_t timestamp, datetime_t *dt) {
    // Apply timezone offset
    timestamp += timezone_offset_minutes * 60;

    int64_t days = timestamp / 86400;
    int64_t seconds = timestamp % 86400;

    dt->hour = seconds / 3600;
    dt->minute = (seconds % 3600) / 60;
    dt->second = seconds % 60;

    // Calculate date (simplified algorithm)
    int64_t year = 1970;
    while (1) {
        int days_in_year = 365;
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            days_in_year = 366;
        }

        if (days < days_in_year) break;
        days -= days_in_year;
        year++;
    }

    dt->year = year;

    // Calculate month and day
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        days_in_month[1] = 29;
    }

    int month = 0;
    while (days >= days_in_month[month]) {
        days -= days_in_month[month];
        month++;
    }

    dt->month = month + 1;
    dt->day = days + 1;
    dt->timezone_offset = timezone_offset_minutes;
}

// Get current time
void get_current_time(datetime_t *dt) {
    int64_t current_timestamp = unix_timestamp + (system_time_ms / 1000);
    unix_to_datetime(current_timestamp, dt);
}

// NTP client - sync time from NTP server
int ntp_sync(const char *server) {
    // Resolve NTP server
    uint32_t server_ip = dns_resolve(server);
    if (server_ip == 0) {
        // Try default NTP servers
        server_ip = (129 << 24) | (6 << 16) | (15 << 8) | 28; // time.nist.gov
    }

    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return -1;

    // Prepare NTP packet
    ntp_packet_t packet;
    memset(&packet, 0, sizeof(packet));
    packet.li_vn_mode = 0x1B; // LI=0, VN=3, Mode=3 (client)

    // Send NTP request
    sockaddr_in_t server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(123); // NTP port
    server_addr.sin_addr = server_ip;

    udp_send(server_ip, 123, 12345, (uint8_t*)&packet, sizeof(packet));

    // Wait for response (simplified - should use select/poll)
    // TODO: Receive NTP response

    // For now, extract timezone from DHCP or use UTC
    // Ethernet/DHCP can provide timezone via DHCP option 100 and 101

    // Parse NTP response
    // NTP timestamp is seconds since 1900-01-01
    // Unix timestamp is seconds since 1970-01-01
    // Difference: 2208988800 seconds

    uint32_t ntp_time = ntohl(packet.tx_timestamp_sec);
    unix_timestamp = ntp_time - 2208988800ULL;

    close(sockfd);
    return 0;
}

// Extract timezone from DHCP response
void dhcp_set_timezone(int offset_minutes) {
    timezone_offset_minutes = offset_minutes;
}

// Timer tick - called every millisecond
void timer_tick(void) {
    system_time_ms++;
}

// Get system uptime in milliseconds
uint64_t get_uptime_ms(void) {
    return system_time_ms;
}

// Get Unix timestamp
int64_t get_unix_timestamp(void) {
    return unix_timestamp + (system_time_ms / 1000);
}

// Format time as string
void format_time(datetime_t *dt, char *buffer, int size) {
    // Format: YYYY-MM-DD HH:MM:SS
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
             dt->year, dt->month, dt->day,
             dt->hour, dt->minute, dt->second);
}

// Get timezone name from offset
const char* get_timezone_name(int offset_minutes) {
    // Common timezones
    switch (offset_minutes) {
        case -720: return "UTC-12";
        case -660: return "UTC-11";
        case -600: return "HST"; // Hawaii
        case -540: return "AKST"; // Alaska
        case -480: return "PST"; // Pacific
        case -420: return "MST"; // Mountain
        case -360: return "CST"; // Central
        case -300: return "EST"; // Eastern
        case -240: return "AST"; // Atlantic
        case -180: return "BRT"; // Brazil
        case 0: return "UTC";
        case 60: return "CET"; // Central European
        case 120: return "EET"; // Eastern European
        case 180: return "MSK"; // Moscow
        case 240: return "GST"; // Gulf
        case 300: return "PKT"; // Pakistan
        case 330: return "IST"; // India
        case 360: return "BST"; // Bangladesh
        case 420: return "ICT"; // Indochina
        case 480: return "CST"; // China
        case 540: return "JST"; // Japan
        case 600: return "AEST"; // Australian Eastern
        case 660: return "AEDT"; // Australian Eastern Daylight
        case 720: return "NZST"; // New Zealand
        default: {
            static char tz_buf[16];
            int hours = offset_minutes / 60;
            int mins = offset_minutes % 60;
            if (mins == 0) {
                snprintf(tz_buf, sizeof(tz_buf), "UTC%+d", hours);
            } else {
                snprintf(tz_buf, sizeof(tz_buf), "UTC%+d:%02d", hours, mins);
            }
            return tz_buf;
        }
    }
}

// Initialize time system
void time_init(void) {
    system_time_ms = 0;
    unix_timestamp = 0;
    timezone_offset_minutes = 0;

    // Sync with NTP server
    ntp_sync("pool.ntp.org");
}

// Sleep for milliseconds
void sleep_ms(uint32_t ms) {
    uint64_t target = system_time_ms + ms;
    while (system_time_ms < target) {
        __asm__ volatile("hlt");
    }
}

// Get day of week (0 = Sunday, 6 = Saturday)
int get_day_of_week(datetime_t *dt) {
    // Zeller's congruence
    int y = dt->year;
    int m = dt->month;
    int d = dt->day;

    if (m < 3) {
        m += 12;
        y--;
    }

    int h = (d + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
    return (h + 6) % 7; // Convert to 0=Sunday
}

const char* get_day_name(int day) {
    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    return days[day % 7];
}

const char* get_month_name(int month) {
    const char* months[] = {"", "January", "February", "March", "April", "May", "June",
                           "July", "August", "September", "October", "November", "December"};
    return months[month % 13];
}
