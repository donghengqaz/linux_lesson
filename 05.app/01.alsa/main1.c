#include <alsa/asoundlib.h>
#include "record_api.h"

#define VOL_MAX 16

void output_pcm(int16_t *pdata, int size)
{
    int i;

    for (; size >= 16; size -= 16) {
        for (i = 0; i < VOL_MAX; i++) {
            fprintf(stdout, "%5d ", pdata[i]);
        }
        fprintf(stdout, "\n");
    }

    if (size) {
        for (; size >= 0; size--)
            fprintf(stdout, "%5d ", pdata[i++]);

        fprintf(stdout, "\n");
    }
}

int main(void)
{
    int16_t *pdata;
    record_params_t params = {
        .format = SND_PCM_FORMAT_S16_LE,
        .channels = 1,
        .sample_rate = 8 * 1000
    };

    record_handle_t *hdl = mozart_get_record_handle(&params);
    if (!hdl) {
        fprintf(stderr, "open record error\n");
        return -1;
    }

    while (1) {
        pdata = (int16_t *)mozart_record_get_data(hdl);
        if (!pdata) {
            fprintf(stderr, "get record error\n");
            break;
        }

        output_pcm(pdata, hdl->chunk_size);
    }

    mozart_release_record_handle(hdl);

    return 0;
}
