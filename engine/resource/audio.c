#include "audio.h"
#include <string.h>

static AudioResource s_pool[AUDIO_RESOURCE_POOL_SIZE];
static int s_used[AUDIO_RESOURCE_POOL_SIZE];

void audio_resource_pool_init(void)
{
    memset(s_pool, 0, sizeof(s_pool));
    memset(s_used, 0, sizeof(s_used));
}

int audio_resource_load(const char *cdpath,
                        unsigned char xa_file,
                        unsigned char xa_channel,
                        AudioResource *out)
{
    CdlFILE file;
    if (!CdSearchFile(&file, (char *)cdpath))
        return 0;
    out->loc = file.pos;
    out->valid = 1;
    out->xa_file = xa_file;
    out->xa_channel = xa_channel;
    return 1;
}
