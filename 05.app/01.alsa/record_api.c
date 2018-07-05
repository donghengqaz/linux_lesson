/**
 * @file record.c
 * @brief For the operation of record API
 */

#include "record_api.h"

#define DEBUG(x,y...)   {printf("[ %s : %s : %d] ",__FILE__, __func__, __LINE__); printf(x,##y); printf("\n");}
#define ERROR(x,y...)   {printf("[ %s : %s : %d] ",__FILE__, __func__, __LINE__); printf(x,##y); printf("\n");}

static int record_set_params(record_handle_t * record_handle, record_params_t *record_params)
{
    snd_pcm_hw_params_t *hwparams;
    uint32_t exact_rate;
    uint32_t buffer_time, period_time;

    /* Allocate the snd_pcm_hw_params_t structure on the stack. */
    snd_pcm_hw_params_alloca(&hwparams);

    /* Init hwparams with full configuration space */
    if (snd_pcm_hw_params_any(record_handle->handle, hwparams) < 0) {
        ERROR("Error snd_pcm_hw_params_any");
        goto ERR_SET_PARAMS;
    }

    if (snd_pcm_hw_params_set_access(record_handle->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        ERROR("Error snd_pcm_hw_params_set_access");
        goto ERR_SET_PARAMS;
    }

    if (snd_pcm_hw_params_set_format(record_handle->handle, hwparams, record_params->format) < 0) {
        ERROR("Error snd_pcm_hw_params_set_format");
        goto ERR_SET_PARAMS;
    }
    record_handle->format = record_params->format;

    /* Set number of channels */
    if (snd_pcm_hw_params_set_channels(record_handle->handle, hwparams, record_params->channels) < 0) {
        ERROR("Error snd_pcm_hw_params_set_channels");
        goto ERR_SET_PARAMS;
    }
    record_handle->channels = record_params->channels;

    /* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.       */
    exact_rate = record_params->sample_rate;
    if (snd_pcm_hw_params_set_rate_near(record_handle->handle, hwparams, &exact_rate, 0) < 0) {
        ERROR("Error snd_pcm_hw_params_set_rate_near");
        goto ERR_SET_PARAMS;
    }
    if (record_params->sample_rate != exact_rate) {
        ERROR("The rate %d Hz is not supported by your hardware. ==> Using %d Hz instead.",
            record_params->sample_rate, exact_rate);
    }

    if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) {
        ERROR("Error snd_pcm_hw_params_get_buffer_time_max");
        goto ERR_SET_PARAMS;
    }
    if (buffer_time > 500000) buffer_time = 500000;
    period_time = buffer_time / 4;

    if (snd_pcm_hw_params_set_buffer_time_near(record_handle->handle, hwparams, &buffer_time, 0) < 0) {
        ERROR("Error snd_pcm_hw_params_set_buffer_time_near");
        goto ERR_SET_PARAMS;
    }

    if (snd_pcm_hw_params_set_period_time_near(record_handle->handle, hwparams, &period_time, 0) < 0) {
        ERROR("Error snd_pcm_hw_params_set_period_time_near");
        goto ERR_SET_PARAMS;
    }

    /* Set hw params */
    if (snd_pcm_hw_params(record_handle->handle, hwparams) < 0) {
        ERROR("Error snd_pcm_hw_params(handle, params)");
        goto ERR_SET_PARAMS;
    }

    snd_pcm_hw_params_get_period_size(hwparams, &record_handle->chunk_size, 0);
    snd_pcm_hw_params_get_buffer_size(hwparams, &record_handle->buffer_size);
    if (record_handle->chunk_size == record_handle->buffer_size) {
        ERROR(("Can't use period equal to buffer size (%lu == %lu)"), record_handle->chunk_size, record_handle->buffer_size);
        goto ERR_SET_PARAMS;
    }

    //通过fomart获取每sample的bit数
    record_handle->bits_per_sample = snd_pcm_format_physical_width(record_params->format);

    //把frame定义为一个sample的数据，包括每个channel
    record_handle->bits_per_frame = record_handle->bits_per_sample * record_params->channels;

    //计算录chunk_size个frame需要的byte数
    record_handle->chunk_bytes = record_handle->chunk_size * record_handle->bits_per_frame / 8;

    //开辟录音buffer
    record_handle->data_buf = (uint8_t *)malloc(record_handle->chunk_bytes);
    if (!record_handle->data_buf) {
        ERROR("Error malloc: [data_buf]");
        goto ERR_SET_PARAMS;
    }

    return 0;

ERR_SET_PARAMS:
    return -1;
}

record_handle_t *mozart_get_record_handle(record_params_t *record_params)
{
    char *devicename = "default";
    record_handle_t *record_handle;
    record_handle = (record_handle_t *)malloc(sizeof(record_handle_t));
    memset(record_handle, 0x0, sizeof(record_handle_t));

    if (snd_output_stdio_attach(&record_handle->log, stderr, 0) < 0) {
        ERROR("Error snd_output_stdio_attach");
        free(record_handle);
        return NULL;
    }

    if (snd_pcm_open(&(record_handle->handle), devicename, SND_PCM_STREAM_CAPTURE, 0) < 0) {
        ERROR("Error snd_pcm_open [ %s]", devicename);
        free(record_handle);
        return NULL;
    }

    if (record_set_params(record_handle, record_params) < 0) {
        ERROR("Error set_snd_pcm_params");
        free(record_handle);
        return NULL;
    }
    //snd_pcm_dump(record_handle->handle, record_handle->log);
    return record_handle;
}

static ssize_t read_pcm(record_handle_t *record_handle)
{
    ssize_t r;
    size_t result = 0;
    size_t count = record_handle->chunk_size;
    uint8_t *data = record_handle->data_buf;

    while (count > 0) {
        //录count个frame到data中
        r = snd_pcm_readi(record_handle->handle, data, count);

        if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
            snd_pcm_wait(record_handle->handle, 1000);
        } else if (r == -EPIPE) {
            snd_pcm_prepare(record_handle->handle);
            fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
        } else if (r == -ESTRPIPE) {
            fprintf(stderr, "<<<<<<<<<<<<<<< Need suspend >>>>>>>>>>>>>>>\n");
        } else if (r < 0) {
            fprintf(stderr, "Error snd_pcm_readi: [%s]", snd_strerror(r));
            return -1;
        }

        if (r > 0) {
            result += r;
            count -= r;
            data += r * record_handle->bits_per_frame / 8;
        }
    }
    return result;
}

void *mozart_record_get_data(record_handle_t * record_handle)
{
    if (read_pcm(record_handle) != record_handle->chunk_size)
        return NULL;
    return record_handle->data_buf;
}

void mozart_release_record_handle(record_handle_t * record_handle)
{
    snd_pcm_drain(record_handle->handle);
    free(record_handle->data_buf);
    snd_output_close(record_handle->log);
    snd_pcm_close(record_handle->handle);
}
