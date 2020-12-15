#include <alsa/asoundlib.h>
#include <stdio.h>

void SetAlsaMasterVolume(long volume)
{
    long min, maxl;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &maxl);
    printf("min:%d,max:%d\n", min, maxl);
    snd_mixer_selem_set_playback_volume_all(elem, volume * (maxl-min) / 100);
    printf("min:%d,max:%d\n", min, maxl);
    printf("update volume\n");

    snd_mixer_close(handle);
}

int main(int argc, char *argv[])
{
    
    SetAlsaMasterVolume(50);
    return 0;
}