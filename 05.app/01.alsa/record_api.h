/**
 * @file record_interface.h
 * @brief For the operation of record API
 */

#ifndef _RECORD_API_H
#define _RECORD_API_H

#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <locale.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "alsa/asoundlib.h"
#include <assert.h>

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

typedef struct {
    /**
     * @brief Alsa handle.
     */
    snd_pcm_t *handle;
    /**
     * @brief Alsa log Output redirection.
     */
    snd_output_t *log;
    /**
     * @brief Alsa internal audio cache buffer size.
     */
    snd_pcm_uframes_t buffer_size;
    /**
     * @brief Data format.
     */
    snd_pcm_format_t format;
    /**
     * @brief The number of channels.
     */
    uint16_t channels;
    /**
     * @brief The number of bit per sample point.
     */
    size_t bits_per_sample;
    /**
     * @brief The number of bit per frame.
     */
    size_t bits_per_frame;
    /**
     * @brief The amount of frame required to record.
     */
    snd_pcm_uframes_t chunk_size;
    /**
     * @brief The number of bytes required to be recorded is matched with the
     * chunk_size.
     */
    size_t chunk_bytes;
    /**
     * @brief Point to the recording data buffer, the size is chunk_bytes bytes,
     * have chunk_size frame.
     */
    uint8_t *data_buf;
} record_handle_t;

typedef struct {
    /*
     * Specifies data format.
     * SND_PCM_FORMAT_S16_LE;
     * SND_PCM_FORMAT_S8;
     * SND_PCM_FORMAT_UNKNOWN;
     * All references at sound/asound.h
     */
    snd_pcm_format_t format;
    /**
     * @brief Specifies the number of channels.
     */
    uint16_t channels;
    /**
     * @brief frequence of sample
     */
    uint32_t sample_rate;
} record_params_t;


/**
 * @brief Get the handle for the recording.
 *
 * @param record_params Recording parameters.
 *
 * @return Record handle on success otherwise NULL is returned.
 */
record_handle_t *mozart_get_record_handle(record_params_t *record_params);

/**
 * @brief Get the recording data, the size is chunk_bytes bytes.
 *
 * @param record_handle
 *
 * @return Return a pointer to data on success otherwise NULL is returned.
 */
void *mozart_record_get_data(record_handle_t * record_handle);

/**
 * @brief Release record handle after call mozart_get_record_handle.
 *
 * @param record_handle
 */
void mozart_release_record_handle(record_handle_t * record_handle);

#endif /* _RECORD_API_H */
