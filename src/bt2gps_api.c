#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#include "bt2gps_api.h"
#include "buzz_logging.h"

#define BT2_GPS_MAX_LINE 8192
#define BT2_GPS_MAX_PARSE_WORDS 16

#define GLL_STRING "$GPGLL"
#define RMC_STRING "$GPRMC"


typedef struct nmea_i_parser_s {
    const char * type;
    int word_count;
    int lat_ndx;
    int lat_hem_ndx;
    int lon_ndx;
    int lon_hem_ndx;
 } nmea_i_parser_t;


nmea_i_parser_t * g_nmea_sentence_map = NULL;

/*
 * Each entry is a description of where the lattitude/longitude information is in the word list
 * The array is null terminated
 */
 static int initialize_map() {
    static int type_count = 2;

    g_nmea_sentence_map = (nmea_i_parser_t *) calloc(type_count+1, sizeof(nmea_i_parser_t));

    g_nmea_sentence_map[0].type = "$GPRMC";
    g_nmea_sentence_map[0].word_count = 7;
    g_nmea_sentence_map[0].lat_ndx = 3;
    g_nmea_sentence_map[0].lat_hem_ndx = 4;
    g_nmea_sentence_map[0].lon_ndx = 5;
    g_nmea_sentence_map[0].lon_hem_ndx = 7;

    g_nmea_sentence_map[1].type = "$GPGLL";
    g_nmea_sentence_map[1].word_count = 5;
    g_nmea_sentence_map[1].lat_ndx = 1;
    g_nmea_sentence_map[1].lat_hem_ndx = 2;
    g_nmea_sentence_map[1].lon_ndx = 3;
    g_nmea_sentence_map[1].lon_hem_ndx = 4;

    return 0;
 }



static int read_until_char(int fd, char * buf, size_t max_size, char term_char) {
    int done = 0;
    int n;
    size_t ndx = 0;

    while(!done) {
        n = read(fd, &buf[ndx], 1);
        if (n < 0) {
            logger(BUZZ_ERROR, "GPS error returned when reading the serial port: %s", strerror(errno));
            return BT2_GPS_NOT_FOUND;
        }
        if (buf[ndx] == term_char) {
            done = 1;
        }
        ndx = ndx + n;
        if (ndx >= max_size) {
            logger(BUZZ_WARN, "exceeded the max size of %d", max_size);
            return BT2_GPS_NOT_FOUND;
        }
    }
    return ndx;
}

static int daysmins_to_float(const char * daysmin, char hem, float * out_v) {
    static const char * neg_hem = "sSwW";

    float sign = 1.0f;
    int rc;
    float raw;
    int day;
    float min;
    float v;

    logger(BUZZ_DEBUG, "converting %s %c", daysmin, hem);
    if (NULL != strchr(neg_hem, hem)) {
        sign = -1.0f;
    }
    rc = sscanf(daysmin, "%f", &raw);
    if (rc <= 0) {
        return BT2_GPS_ERROR;
    }
    day = raw / 100;
    min = (raw - (day * 100.0)) / 60.0;
    v = min + day;

    *out_v = v * sign;;

    return BT2_GPS_SUCCESS;
}

int bt2_gps_init(
    bt2_gps_handle_t ** out_handle,
    const char * serial_path,
    speed_t baud) {
    bt2_gps_handle_t * new_handle;
    struct termios tty;

    if (g_nmea_sentence_map == NULL) {
        logger(BUZZ_DEBUG, "Setting up the sentence parse in global memory");
        initialize_map();
    }

    logger(BUZZ_DEBUG, "Opening the serial port for bluetooth");
    new_handle = (bt2_gps_handle_t *) calloc(1, sizeof(bt2_gps_handle_t));
    new_handle->serial_port = open(serial_path, O_RDWR);
    if (new_handle->serial_port < 0) {
        logger(BUZZ_ERROR, "Failed to open %s: %s", serial_path, strerror(errno));
        goto error;
    }

    logger(BUZZ_DEBUG, "Set tty options");
    memset(&tty, 0, sizeof(tty));
    if(tcgetattr(new_handle->serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
    }
    cfsetospeed (&tty, baud);

    pthread_cond_init(&new_handle->cond, NULL);
    pthread_mutex_init(&new_handle->mutex, NULL);

    *out_handle = new_handle;

    return BT2_GPS_SUCCESS;
error:
    free(new_handle);
    return BT2_GPS_ERROR;
}

int bt2_gps_destroy(bt2_gps_handle_t * handle) {
    if (handle->running) {
        logger(BUZZ_WARN, "Trying to destroy a running handle");
        return BT2_GPS_ERROR;
    }
    close(handle->serial_port);
    free(handle);
    pthread_cond_destroy(&handle->cond);
    pthread_mutex_destroy(&handle->mutex);
    return BT2_GPS_SUCCESS;
}

static int read_sentence(
    bt2_gps_handle_t * gps_handle,
    char * buffer,
    size_t buf_len) {
    int rc;
    int char_count = 0;
    int n;

    memset(buffer, '\0', buf_len);

    /* read until we find the first character of a sentence */
    while (buffer[0] != '$') {
        n = read(gps_handle->serial_port, buffer, 1);
        if (n < 0) {
            logger(BUZZ_ERROR, "GPS error returned when reading the serial port: %s", strerror(errno));
            return BT2_GPS_NOT_FOUND;
        }
        char_count++;
        if (char_count > BT2_GPS_MAX_LINE) {
            logger(BUZZ_ERROR, "Read %d bytes before finding a $", char_count);
            return BT2_GPS_NOT_FOUND;
        }
    }

    rc = read_until_char(gps_handle->serial_port, &buffer[1], BT2_GPS_MAX_LINE-1, '\n');
    if (rc < 0) {
        logger(BUZZ_ERROR, "Did not find a CR in %d chars", BT2_GPS_MAX_LINE);
        return rc;
    }
 
    return BT2_GPS_SUCCESS;
}

/* We avoid the need for heap memory by spliting the sentence in place by overwriting ',' with '\0' */
static int split_sentence(char * buffer, size_t buf_len, char ** out_words, size_t word_count) {

    int current_word = 0;
    char * ptr;

    ptr = buffer;
    out_words[current_word] = ptr;
    current_word++;
    ptr = strchr(ptr, ',');
    while(ptr != NULL && current_word < word_count) {
        *ptr = '\0';
        ptr++;
        out_words[current_word] = ptr;
        ptr = strchr(ptr, ',');
        current_word++; 
    }

    return current_word;
}

static int parse_out_location(nmea_i_parser_t * parser, char ** words, const int word_count, float * out_lat, float * out_lon) {
    int rc;

    logger(BUZZ_DEBUG, "Parsing sentence type %s", parser->type);
    if (word_count < parser->word_count) {
        logger(BUZZ_DEBUG, "Bad %s sentence, required at least %d words but found %d", parser->type, parser->word_count, word_count);
        return BT2_GPS_ERROR;
    }
    rc = daysmins_to_float(words[parser->lat_ndx], *words[parser->lat_hem_ndx], out_lat);
    if (rc != BT2_GPS_SUCCESS) {
        return BT2_GPS_ERROR;
    }
    rc = daysmins_to_float(words[parser->lon_ndx], *words[parser->lon_hem_ndx], out_lon);
    if (rc != BT2_GPS_SUCCESS) {
        return BT2_GPS_ERROR;
    }
    return BT2_GPS_SUCCESS;
}

/*
 * read until we get a location or an error 
 *
 * must be called locked
 */
static int get_i_location(
    bt2_gps_handle_t * gps_handle,
    float * out_lat,
    float * out_lon) {
    int rc;
    char buffer[BT2_GPS_MAX_LINE];
    char * words[BT2_GPS_MAX_PARSE_WORDS];
    int word_count;
    int done = 0;

    while (!done) {
        logger(BUZZ_DEBUG, "Reading a sentence from the GPS device...");
        rc = read_sentence(gps_handle, buffer, BT2_GPS_MAX_LINE);
        if (rc != BT2_GPS_SUCCESS) {
            return BT2_GPS_ERROR;
        }
        logger(BUZZ_INFO, "Read the sentence: %s", buffer);

        word_count = split_sentence(buffer, BT2_GPS_MAX_LINE, words, BT2_GPS_MAX_PARSE_WORDS);
        if(word_count < 1) {
            return BT2_GPS_ERROR;
        }

        logger(BUZZ_DEBUG, "Parsing sentence type %s of %d words", words[0], word_count);
        for(int i = 0; g_nmea_sentence_map[i].type != NULL; i++) {
            if (strcmp(g_nmea_sentence_map[i].type, words[0]) == 0) {
                rc = parse_out_location(&g_nmea_sentence_map[i], words, word_count, out_lat, out_lon);
                if (word_count < 5) {
                    return BT2_GPS_ERROR;
                }   
                return BT2_GPS_SUCCESS;
            }
        }
        logger(BUZZ_DEBUG, "Sentence type %s in unknown.", words[0]);
    }

    return BT2_GPS_SUCCESS;
}


int bt2_gps_get_location_now(
    bt2_gps_handle_t * gps_handle, float * out_lat, float * out_lon) {
    int rc;

    pthread_mutex_lock(&gps_handle->mutex);
    {
        rc = get_i_location(gps_handle, out_lat, out_lon);
    }
    pthread_mutex_unlock(&gps_handle->mutex);

    return rc;
}


static void * gather_thread(void * arg) {
    bt2_gps_handle_t * gps_handle = (bt2_gps_handle_t*) arg;
    int rc;
    float lat;
    float lon;
    struct timespec waittime;

    pthread_mutex_lock(&gps_handle->mutex);
    {
        while(gps_handle->running) {
            rc = get_i_location(gps_handle, &lat, &lon);
            if (rc != BT2_GPS_SUCCESS) {
                logger(BUZZ_ERROR, "failed to get a sentence");

                waittime.tv_sec = gps_handle->error_interval + time(NULL);
                waittime.tv_nsec = 0;
            }
            else {
                logger(BUZZ_INFO, "Updating location %f %f", lat, lon);

                gps_handle->update_time = time(NULL);
                gps_handle->last_known_lat = lat;
                gps_handle->last_known_lon = lon;

                waittime.tv_sec = gps_handle->interval_time + time(NULL);
                waittime.tv_nsec = 0;
            }

            pthread_cond_timedwait(&gps_handle->cond, &gps_handle->mutex, &waittime);
        }
    }
    pthread_mutex_unlock(&gps_handle->mutex);

    return NULL;
}

int bt2_gps_get_last_known_location(
    bt2_gps_handle_t * gps_handle, float * out_lat, float * out_lon, time_t * when) {
    int rc;

    pthread_mutex_lock(&gps_handle->mutex);
    {
        if (gps_handle->update_time == 0) {
            rc = BT2_GPS_ERROR;
        }
        else {
            *when = gps_handle->update_time;
            *out_lat = gps_handle->last_known_lat;
            *out_lon = gps_handle->last_known_lon;
            rc = BT2_GPS_SUCCESS;
        }
    }
    pthread_mutex_unlock(&gps_handle->mutex);

    return rc;
}

int bt2_gps_start(bt2_gps_handle_t * gps_handle, int interval_time, int error_interval) {
    if (gps_handle->running) {
        logger(BUZZ_WARN, "Attempting to start a runnign handle");
        return BT2_GPS_ERROR;
    }

    pthread_mutex_lock(&gps_handle->mutex);
    {
        gps_handle->interval_time = interval_time;
        gps_handle->error_interval = error_interval;
        gps_handle->running = 1;
        pthread_create(&gps_handle->thread_id, NULL, gather_thread, gps_handle);
    }
    pthread_mutex_unlock(&gps_handle->mutex);

    return BT2_GPS_SUCCESS;
}

int bt2_gps_stop(bt2_gps_handle_t * gps_handle) {

    pthread_mutex_lock(&gps_handle->mutex);
    {
        logger(BUZZ_INFO, "Shutting down gps thread");
        gps_handle->running = 0;
        pthread_cond_broadcast(&gps_handle->cond);
    }
    pthread_mutex_unlock(&gps_handle->mutex);

    logger(BUZZ_INFO, "waiting for the thread to end");
    pthread_join(gps_handle->thread_id, NULL);

    return BT2_GPS_SUCCESS;
}
