#include "weather.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

    /* ┌──────────────────┐ **
    ** │  Instant météo   │ **
    ** │   Aujourd'hui    │ **
    ** └──────────────────┘ */ 

/* Default coordinates (Tours, France) */
#define WEATHER_LAT         47.3941
#define WEATHER_LON         0.6848
#define WEATHER_CACHE_SEC   900  /* Refresh every 15 minutes */
#define WEATHER_DISPLAY_SEC 5    /* Show on LCD for 5 seconds */

typedef struct {
    bool valid;
    float temp;
    int humidity;
    int weather_code;
    float t_min;
    float t_max;
} weather_data;

static pthread_mutex_t weather_lock = PTHREAD_MUTEX_INITIALIZER;
static weather_data cached = { .valid = false };
static bool fetch_in_progress = false;
static time_t last_fetch = 0;

static const char *wmo_to_text(int code) {
    switch (code) {
        case 0:           return "Clear sky";
        case 1:           return "Mainly clear";
        case 2:           return "Partly cloudy";
        case 3:           return "Overcast";
        case 45: case 48: return "Fog";
        case 51: case 53:
        case 55:          return "Drizzle";
        case 56: case 57: return "Freezing drizzle";
        case 61:          return "Light rain";
        case 63:          return "Rain";
        case 65:          return "Heavy rain";
        case 66: case 67: return "Freezing rain";
        case 71: case 73:
        case 75: case 77: return "Snow";
        case 80: case 81:
        case 82:          return "Rain showers";
        case 85: case 86: return "Snow showers";
        case 95: case 96:
        case 99:          return "Thunderstorm";
        default:          return "Unknown";
    }
}

static bool parse_open_meteo(const char *json, weather_data *out) {
    const char *cur = strstr(json, "\"current\":{");
    const char *daily = strstr(json, "\"daily\":{");
    if (!cur || !daily) {
        return false;
    }

    const char *p_temp = strstr(cur, "\"temperature_2m\":");
    const char *p_hum  = strstr(cur, "\"relative_humidity_2m\":");
    const char *p_code = strstr(cur, "\"weather_code\":");
    const char *p_min  = strstr(daily, "\"temperature_2m_min\":[");
    const char *p_max  = strstr(daily, "\"temperature_2m_max\":[");

    if (!p_temp || !p_hum || !p_code || !p_min || !p_max) {
        return false;
    }

    weather_data parsed = { .valid = true };
    if (sscanf(p_temp, "\"temperature_2m\":%f", &parsed.temp) != 1 ||
        sscanf(p_hum,  "\"relative_humidity_2m\":%d", &parsed.humidity) != 1 ||
        sscanf(p_code, "\"weather_code\":%d", &parsed.weather_code) != 1 ||
        sscanf(p_min,  "\"temperature_2m_min\":[%f", &parsed.t_min) != 1 ||
        sscanf(p_max,  "\"temperature_2m_max\":[%f", &parsed.t_max) != 1) {
        return false;
    }

    *out = parsed;
    return true;
}

static void *fetch_weather_thread(void *arg) {
    (void)arg;

    char cmd[512];
    snprintf(
        cmd, sizeof(cmd),
        "curl -fsS --max-time 8 "
        "\"https://api.open-meteo.com/v1/forecast"
        "?latitude=%.4f&longitude=%.4f"
        "&current=temperature_2m,relative_humidity_2m,weather_code"
        "&daily=temperature_2m_min,temperature_2m_max"
        "&timezone=auto&forecast_days=1\"",
        WEATHER_LAT, WEATHER_LON
    );

    char buf[2048] = { 0 };
    FILE *fp = popen(cmd, "r");
    if (fp != NULL) {
        size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
        buf[n] = '\0';
        pclose(fp);
    }

    weather_data fresh = { .valid = false };
    bool ok = (buf[0] != '\0') && parse_open_meteo(buf, &fresh);

    pthread_mutex_lock(&weather_lock);
    if (ok) {
        cached = fresh;
        last_fetch = time(NULL);
    }
    fetch_in_progress = false;
    pthread_mutex_unlock(&weather_lock);

    return NULL;
}

static void trigger_refresh_if_needed(void) {
    time_t now = time(NULL);

    pthread_mutex_lock(&weather_lock);
    bool stale = (!cached.valid) || (now - last_fetch >= WEATHER_CACHE_SEC);
    if (stale && !fetch_in_progress) {
        fetch_in_progress = true;
        pthread_t tid;
        if (pthread_create(&tid, NULL, fetch_weather_thread, NULL) == 0) {
            pthread_detach(tid);
        } else {
            fetch_in_progress = false;
        }
    }
    pthread_mutex_unlock(&weather_lock);
}

display_string weather_display(int *done) {
    static int ticks = 0;

    if (ticks == 0) {
        trigger_refresh_if_needed();
    }

    pthread_mutex_lock(&weather_lock);
    weather_data snap = cached;
    bool fetching = fetch_in_progress;
    pthread_mutex_unlock(&weather_lock);

    display_string out = { 0 };

    if (!snap.valid) {
        snprintf(out.line1, sizeof(out.line1), "Weather");
        snprintf(out.line2, sizeof(out.line2), "%s",
                 fetching ? "Fetching..." : "Unavailable");
        ticks++;
        if (ticks >= 3) {
            ticks = 0;
            *done = 1;
        }
        return out;
    }

    snprintf(out.line1, sizeof(out.line1), "%.1fC %d%% %.0f/%.0fC",
             snap.temp, snap.humidity, snap.t_min, snap.t_max);
    snprintf(out.line2, sizeof(out.line2), "%s", wmo_to_text(snap.weather_code));

    ticks++;
    if (ticks >= WEATHER_DISPLAY_SEC) {
        ticks = 0;
        *done = 1;
    }

    return out;
}