// read dontreadme.md for explanation on how things works

#include "lcd.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>


#define I2C_ADDR      0x27  // Screen's I2C address (change urs if different :p)

#define LCD_BACKLIGHT 0x08  // 0000 1000
#define LCD_ENABLE    0x04  // 0000 0100

#define MODE_CMD      0x00  // 0000 0000
#define MODE_CHAR     0x01  // 0000 0001

#define LCD_LINE_1    0x80  // Command: Jump cursor to start of Row 1
#define LCD_LINE_2    0xC0  // Command: Jump cursor to start of Row 2

// RAM Disk File Paths
#define PID_FILE      "/dev/shm/lcd.pid"
#define MSG_FILE      "/dev/shm/lcd.msg"
#define SCREEN_FILE   "/dev/shm/lcd.screen"

int i2c_fd = -1; // connection ID; -1 = not connected yet
volatile sig_atomic_t wake_up = 0;

static main_screen_slot *main_schedule = NULL;
static int main_schedule_count = 0;

static side_screen_slot *side_schedule = NULL;
static int side_schedule_count = 0;

const uint8_t degree_celcius[8] = {
    0b11000, // ████░░░░░░
    0b11000, // ████░░░░░░
    0b00000, // ░░░░░░░░░░
    0b00111, // ░░░░██████
    0b01000, // ░░██░░░░░░
    0b01000, // ░░██░░░░░░
    0b01000, // ░░██░░░░░░
    0b00111  // ░░░░██████
};

/* 
** Note: const char* const thing[] to make things truly const. 
** Only characters are const here, but strings slots are writable. 
** Whatever ┐(￣ヘ￣)┌ edit them if u want lol
*/
const char* jours[] = {
    "Dimanche", "Lundi", "Mardi", "Mercredi",
    "Jeudi", "Vendredi", "Samedi"
};
const char* mois[] = {
    "Janvier", "Février", "Mars", "Avril", "Mai",
    "Juin", "Juillet", "Août", "Septembre",
    "Octobre", "Novembre", "Décembre"
};
const char* days[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};
const char* const months[] = {
    "January", "February", "March", "April", "May",
    "June", "July", "August", "September",
    "October", "November", "December"
};

void lcd_send(uint8_t val, uint8_t mode) {
    uint8_t pt1 = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t pt2 = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

    uint8_t seq[4] = {
        pt1 | LCD_ENABLE,
        pt1,
        pt2 | LCD_ENABLE,
        pt2
    };

    for (int i = 0; i < 4; i++) {
        if (write(i2c_fd, &seq[i], 1) != 1) {} // avoid compilation warning
        usleep(500);
    }
}

void lcd_init(void) {
    // explaination in dontreadme
    uint8_t init_cmds[6] = {0x33, 0x32, 0x06, 0x0C, 0x28, 0x01};
    for (int i = 0; i < 6; i++) {
        lcd_send(init_cmds[i], MODE_CMD);
        usleep(2000);
    }
}

void lcd_write_line(uint8_t row_cmd, const char *text) {
    char buf[17];
    snprintf(buf, sizeof(buf), "%-16.16s", text);

    lcd_send(row_cmd, MODE_CMD);
    for (int i = 0; i < 16; i++) {
        lcd_send((uint8_t)buf[i], MODE_CHAR);
    }
}

void handle_signal(int sig) {
    (void)sig; // avoid unused parameter compilation warning
    wake_up = 1;
}

void notify_daemon(void) {
    FILE *f = fopen(PID_FILE, "r");
    if (f) {
        int pid = 0;
        if (fscanf(f, "%d", &pid) == 1 && pid > 0) {
            kill(pid, SIGUSR1);
        }
        fclose(f);
    }
}

float get_cpu_temp(void) {
    FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!f) return -1.0f;
    
    int read_temp = 0;
    float temp = -1.0f;
    if (fscanf(f, "%d", &read_temp) == 1) {
        temp = read_temp / 1000.0f;
    }
    fclose(f);

    // usual temps readings for me are 39.9, 40.4 and 40.9 which just triggers me sm
    // feel free to remove the +0.1f if you want "accurate" reading 
    // tho they aren't quite accurate to begin with tbh (at least on raspi 4B)
    return temp +0.1f; 
}

int get_cpu_usage(void) {
    static unsigned long long prev_total = 0, prev_idle = 0;

    FILE *f = fopen("/proc/stat", "r");
    if (!f) return 0;

    unsigned long long user = 0, nice = 0, system = 0, idle = 0;
    unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0;

    if (fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) != 8) {
        fclose(f);
        return 0;
    }
    fclose(f);

    unsigned long long idle_all = idle + iowait;
    unsigned long long total = user + nice + system + idle_all + irq + softirq + steal;

    unsigned long long diff_total = total - prev_total;
    unsigned long long diff_idle  = idle_all - prev_idle;

    prev_total = total;
    prev_idle  = idle_all;

    if (diff_total == 0) return 0;
    return (int)((100 * (diff_total - diff_idle)) / diff_total);
}

int get_ram_usage(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0;
    long total = 0, avail = 0;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %ld kB", &total) == 1) continue;
        if (sscanf(line, "MemAvailable: %ld kB", &avail) == 1) break;
    }
    fclose(f);
    if (total <= 0) return 0;
    return (int)(100 - ((avail * 100) / total));
}

display_string main_display(void) {
    /* ┌──────────────────┐ **
    ** │ 18/05  12h03 45s │ **
    ** │ 41.5c  __0% _31% │ **
    ** └──────────────────┘ */ 
    display_string display;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    snprintf(
        display.line1, 
        sizeof(display.line1), 
        "%02d/%02d  %02dh%02d %02ds",
        t->tm_mday, 
        t->tm_mon + 1, 
        t->tm_hour, 
        t->tm_min, 
        t->tm_sec
    );
    snprintf(
        display.line2,
        sizeof(display.line2), 
        "%4.1fC  %3d%% %3d%%",
        get_cpu_temp(), 
        get_cpu_usage(), 
        get_ram_usage()
    );

    return display;
}

display_string sub_display_fr(void) {
    /* ┌──────────────────┐ **
    ** │   Dimanche  26   │ **
    ** │  Septembre 2026  │ **
    ** └──────────────────┘ */ 

    /* ┌──────────────────┐ **
    ** │    Lundi    06   │ **
    ** │     Mars   2026  │ **
    ** └──────────────────┘ */ 
    display_string display;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    const char *m = mois[t->tm_mon];
    int lenm = (int) strlen(m);
    int spacesm = (9 - lenm + 1) / 2;
    const char *j = jours[t->tm_wday];
    int lenj = (int) strlen(j);
    int spacesj = (9 - lenj + 1) / 2;

    snprintf(
        display.line1,
        sizeof(display.line1),
        " %*s%-*s  %02d  ",
        spacesj, "",
        9 - spacesj, j,
        t->tm_mday
    );
    snprintf(
        display.line2,
        sizeof(display.line2),
        " %*s%-*s %4d ",
        spacesm, "",
        9 - spacesm, m,
        t->tm_year + 1900
    );

    return display;
}

display_string sub_display_en(void) {
    /* ┌──────────────────┐ **
    ** │  Wednesday  26   │ **
    ** │  September 2026  │ **
    ** └──────────────────┘ */ 

    /* ┌──────────────────┐ **
    ** │    Monday   06   │ **
    ** │     Mars   2026  │ **
    ** └──────────────────┘ */ 
    display_string display;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    const char *m = months[t->tm_mon];
    int lenm = (int) strlen(m);
    int spacesm = (9 - lenm + 1) / 2;
    const char *j = days[t->tm_wday];
    int lenj = (int) strlen(j);
    int spacesj = (9 - lenj + 1) / 2;

    snprintf(
        display.line1,
        sizeof(display.line1),
        " %*s%-*s  %02d  ",
        spacesj, "",
        9 - spacesj, j,
        t->tm_mday
    );
    snprintf(
        display.line2,
        sizeof(display.line2),
        " %*s%-*s %4d ",
        spacesm, "",
        9 - spacesm, m,
        t->tm_year + 1900
    );

    return display;
}

int register_main_screen(display_string (*render)(void), int duration_sec) {
    if (!render || duration_sec <= 0) return -1;

    main_screen_slot *tmp = realloc(
        main_schedule,
        (size_t)(main_schedule_count + 1) * sizeof(*main_schedule)
    );
    if (!tmp) return -1;

    main_schedule = tmp;
    main_schedule[main_schedule_count++] = (main_screen_slot){
        .render = render,
        .duration_sec = duration_sec
    };
    return 0;
}

int register_side_screen(display_string (*render)(int *done), int times_per_hour) {
    if (!render || times_per_hour <= 0 || times_per_hour > 3600) return -1;

    side_screen_slot *tmp = realloc(
        side_schedule,
        (size_t)(side_schedule_count + 1) * sizeof(*side_schedule)
    );
    if (!tmp) return -1;

    side_schedule = tmp;
    side_schedule[side_schedule_count++] = (side_screen_slot){
        .render = render,
        .times_per_hour = times_per_hour,
        .last_slot = -1
    };
    return 0;
}

void lcd_update(display_string display) {
    lcd_write_line(LCD_LINE_1, display.line1);
    lcd_write_line(LCD_LINE_2, display.line2);

    FILE *sf = fopen(SCREEN_FILE, "w");
    if (sf) {
        fprintf(sf, ""
            "┌──────────────────┐ \n"
            "│ %-16.16s │\n" // │
            "│ %-16.16s │\n" // │
            "└──────────────────┘ \n",
            display.line1, display.line2
        );
        fclose(sf);
    }
}

display_string run_scheduled_display(void) {
    /* 
    * Main schedule rotation resets to 0 at :00 every minute,
    * so plan it accordingly - have a total loop duration divisor of 60
    * (ex: main_display_1 for 7s, main_display_2 for 3, total 10 which divides 60)
    * A full main schedule is forced between two side schedules if two or more happens to trigger
    * at the same moment (mainly so it still displays time between two side displays)
    * Side schedule also resets to 0 at :00 every hour
    * so also use divisors of 3600 (ex: 2 for every 30 mins, 3 for every 20 mins, 12 for every 5 mins)
    * don't use >60 you dummy it'll break (haven't tried tho)
    * yes i NEED it to be PERFECTLY synced to the clock
    * because idk it just pisses me off if it's not (i might genuinely be autistic help) 
    */
    static int active_screen_slot = -1;   // -1 = running main_schedule
    static int main_cooldown = 0;  // Remaining seconds of forced main rotation

    int cycle_len = 0;
    for (int i = 0; i < main_schedule_count; i++) {
        cycle_len += main_schedule[i].duration_sec;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int hourly_time_now_sec = t->tm_min * 60 + t->tm_sec;

    // if a side display is currently running, keep feeding it ticks until done
    if (active_screen_slot != -1) {
        int done = 0;
        display_string out = side_schedule[active_screen_slot].render(&done);
        if (done) {
            active_screen_slot = -1;
            main_cooldown = cycle_len; // Force one full main rotation next
        }
        return out;
    }

    // only check for due side displays once the forced main rotation finishes
    if (main_cooldown > 0) {
        main_cooldown--;
    } else {
        for (int i = 0; i < side_schedule_count; i++) {
            if (side_schedule[i].times_per_hour <= 0) continue;

            int interval = 3600 / side_schedule[i].times_per_hour;
            time_t current_slot = now / interval;

            // On first startup, wait for the next exact mark (unless we somehow booted right on it)
            if (side_schedule[i].last_slot == -1 && (hourly_time_now_sec % interval != 0)) {
                side_schedule[i].last_slot = current_slot;
                continue;
            }

            if (side_schedule[i].last_slot != current_slot) {
                side_schedule[i].last_slot = current_slot;
                active_screen_slot = i;

                int done = 0;
                display_string out = side_schedule[i].render(&done);
                if (done) {
                    active_screen_slot = -1;
                    main_cooldown = cycle_len;
                }
                return out;
            }
        }
    }

    // main rotation
    if (cycle_len <= 0) {
        display_string empty = { "No main screen", "configured" };
        return empty;
    }

    int pos = t->tm_sec % cycle_len;
    int elapsed = 0;

    for (int i = 0; i < main_schedule_count; i++) {
        elapsed += main_schedule[i].duration_sec;
        if (pos < elapsed) {
            return main_schedule[i].render();
        }
    }

    return main_schedule[0].render();
}

int run_daemon(void) {
    if ((i2c_fd = open("/dev/i2c-1", O_RDWR)) < 0) {
        perror("Failed to open /dev/i2c-1");
        return 1;
    }
    if (ioctl(i2c_fd, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("Failed to connect to LCD");
        close(i2c_fd);
        return 1;
    }

    umask(0);
    FILE *pf = fopen(PID_FILE, "w");
    if (pf) {
        fprintf(pf, "%d\n", getpid());
        fclose(pf);
    }

    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sigaction(SIGUSR1, &sa, NULL);
    signal(SIGHUP, SIG_IGN); // Prevent terminal disconnects from stopping the daemon

    lcd_init();
    // Can be improved later: read from a regular file to allow hot editing of schedules instead of baking them when compiling
    setup_screens();

    while (1) {
        wake_up = 0;
        time_t now = time(NULL);
        int showing_custom = 0;

        FILE *mf = fopen(MSG_FILE, "r");
        if (mf) {
            long expiry = 0;
            display_string d;
            if (fscanf(mf, "%ld\n", &expiry) == 1) {
                if (fgets(d.line1, sizeof(d.line1), mf)) d.line1[strcspn(d.line1, "\r\n")] = 0;
                if (fgets(d.line2, sizeof(d.line2), mf)) d.line2[strcspn(d.line2, "\r\n")] = 0;
            }
            fclose(mf);

            if (expiry == 0 || now < expiry) {
                lcd_update(d);
                showing_custom = 1;
            } else {
                unlink(MSG_FILE);
            }
        }

        if (!showing_custom) {
            lcd_update(run_scheduled_display());
        }
        
        if (!wake_up) {
            // I swear having the display randomly lag a second behind or skip another one pissed me off sm omg
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);

            // Remaining nanoseconds until 50 ms past the next whole second (to make sure the actual second ticked and was updated properly)
            long target_ns = 1000000000L - ts.tv_nsec + 50000000L;

            struct timespec req;
            req.tv_sec  = target_ns / 1000000000L;
            req.tv_nsec = target_ns % 1000000000L;

            nanosleep(&req, NULL);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        return run_daemon();
    }
    if (argc == 2 && strcmp(argv[1], "-c") == 0) {
        unlink(MSG_FILE);
        notify_daemon();
        return 0;
    }
    if (argc >= 3) {
        int duration = (argc >= 4) ? atoi(argv[3]) : 10;
        time_t expiry = (duration > 0) ? (time(NULL) + duration) : 0;
        umask(0);
        FILE *mf = fopen(MSG_FILE, "w");
        if (!mf) {
            perror("Failed to write message");
            return 1;
        }
        fprintf(mf, "%ld\n%s\n%s\n", (long)expiry, argv[1], argv[2]);
        fclose(mf);
        notify_daemon();
        return 0;
    }

    printf("Usage:\n"
           "  %s -d                             Start background daemon\n"
           "  %s \"Line 1\" \"Line 2\" [secs]   Show custom message (0 = hold)\n"
           "  %s -c                             Clear custom message\n",
           argv[0], argv[0], argv[0]);
    return 1;
}