/*
 * Simple sound playback using ALSA API and libasound.
 *
 * Compile:
 * $ cc -o play sound_playback.c -lasound
 */

#include <alsa/asoundlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "sound_playback.h"

#define PCM_DEVICE "default"

int g_init_flag = 0;
Music g_music;

pthread_mutex_t lock;
pthread_mutex_t audio_lock;

long g_volume = 100;
pthread_t g_music_pt;
pthread_t g_audio_pt;
int g_current_music;
int g_music_play_type;
int g_music_specific;

typedef enum {
    MUSIC_PAUSED = 0,
    MUSIC_PLAYING,
    MUSIC_NEXT,
    MUSIC_PREVIOUS
} MUSIC_STATE ;
MUSIC_STATE g_music_state = 1;

typedef struct{
    snd_pcm_t *pcm_handle;
	snd_pcm_uframes_t frames;
    short int channels;
    int seconds;
    int avg_bytes_per_sec;
}SoundParam;

typedef struct{
    char filename[256];
    AUDIO_FINISHED_CALLBACK call;
} Audio;

Audio g_audio = {{0}, NULL};

/* wav音频头部格式 */
typedef struct _wave_pcm_hdr
{
	char            riff[4];                // = "RIFF"
	int				size_8;                 // = FileSize - 8
	char            wave[4];                // = "WAVE"
	char            fmt[4];                 // = "fmt "
	int				fmt_size;				// = 下一个结构体的大小 : 16

	short int       format_tag;             // = PCM : 1
	short int       channels;               // = 通道数 : 1
	int				samples_per_sec;        // = 采样率 : 8000 | 6000 | 11025 | 16000
	int				avg_bytes_per_sec;      // = 每秒字节数 : channels * samples_per_sec * bits_per_sample / 8
	short int       block_align;            // 4
	short int       bits_per_sample;        // = 量化比特数: 8 | 16

	char            data[4];                // = "data";
	int				data_size;              // = 纯数据长度 : FileSize - 44 
} wave_pcm_hdr;

typedef int (*NEXT_MUSIC)(MUSIC_STATE, int , int);

static void file_close(FILE **file){
    if(*file){
        fclose(*file);
        *file = NULL;
    }
}

static MUSIC_STATE music_state_check(){

    MUSIC_STATE music_state;

    pthread_mutex_lock(&lock);
    music_state = g_music_state;
    pthread_mutex_unlock(&lock);

    return music_state;
}

static void set_music_specific(int music_specific){

    pthread_mutex_lock(&lock);
    g_music_specific = music_specific;
    pthread_mutex_unlock(&lock);

}

static int get_music_specific(){

    int music_specific;

    pthread_mutex_lock(&lock);
    music_specific = g_music_specific;
    pthread_mutex_unlock(&lock);

    return music_specific;
}


static int set_param(const char *filename, SoundParam* sp){

	unsigned int pcm, tmp, dir;
	int rate, seconds;
    short int channels;
	snd_pcm_t *pcm_handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;

    int wave_pcm_hdr_size = sizeof(wave_pcm_hdr);
    FILE *f = fopen(filename, "rb");
    if(!f)
        return -1;
    char *head = malloc(wave_pcm_hdr_size);
    if(!head)
        return -1;
    size_t n = fread(head, 1, wave_pcm_hdr_size, f);
    if(n != wave_pcm_hdr_size)
        return -1;

    memcpy(&rate, head+24, 4);
    memcpy(&channels, head+22, 2);

    int avg_bytes_per_sec;
    memcpy(&avg_bytes_per_sec, head+28, 4);
#if 0
    /*not setted correctly*/
    int data_size;
    memcpy(&data_size, head+40, 4);
#endif
    short int bits_per_sample;
    memcpy(&bits_per_sample, head+34, 2);
    int chunk_size;

    memcpy(&chunk_size, head+4, 4);

    seconds =  (chunk_size - 36) / avg_bytes_per_sec;

    printf("rate:%d,channedls:%d, avg_bytes_per_sec:%d, seconds:%d\n"
            "bits_per_sample:%d,chunk_size:%d\n"
            , rate, channels, avg_bytes_per_sec, seconds, bits_per_sample, chunk_size);
    free(head);
    fclose(f);

	/* Open the PCM device in playback mode */
	if (pcm = snd_pcm_open(&pcm_handle, PCM_DEVICE,
					SND_PCM_STREAM_PLAYBACK, 0) < 0) 
		printf("ERROR: Can't open \"%s\" PCM device. %s\n",
					PCM_DEVICE, snd_strerror(pcm));

	/* Allocate parameters object and fill it with default values*/
	snd_pcm_hw_params_alloca(&params);

	snd_pcm_hw_params_any(pcm_handle, params);

	/* Set parameters */
	if (pcm = snd_pcm_hw_params_set_access(pcm_handle, params,
					SND_PCM_ACCESS_RW_INTERLEAVED) < 0) 
		printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_format(pcm_handle, params,
						SND_PCM_FORMAT_S16_LE) < 0) 
		printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, channels) < 0) 
		printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0) < 0) 
		printf("ERROR: Can't set rate. %s\n", snd_strerror(pcm));

	/* Write parameters */
	if (pcm = snd_pcm_hw_params(pcm_handle, params) < 0)
		printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(pcm));

	/* Resume information */
	printf("PCM name: '%s'\n", snd_pcm_name(pcm_handle));

	printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));

	snd_pcm_hw_params_get_channels(params, &tmp);
	printf("channels: %i ", tmp);

	if (tmp == 1)
		printf("(mono)\n");
	else if (tmp == 2)
		printf("(stereo)\n");

	snd_pcm_hw_params_get_rate(params, &tmp, 0);
	printf("rate: %d bps\n", tmp);

	printf("seconds: %d\n", seconds);	

	/* Allocate buffer to hold single period */
	snd_pcm_hw_params_get_period_size(params, &frames, 0);


	snd_pcm_hw_params_get_period_time(params, &tmp, NULL);
    sp->frames = frames;
    sp->channels = channels;
    sp->pcm_handle = pcm_handle;
    sp->seconds = seconds;
    sp->avg_bytes_per_sec = avg_bytes_per_sec;
    printf("set param, frames:%d\n", sp->frames);
    printf("can pause:%d\n", snd_pcm_hw_params_can_pause(params));
}

static void music_state_set(MUSIC_STATE music_state){

    pthread_mutex_lock(&lock);
    g_music_state = music_state;
    pthread_mutex_unlock(&lock);
}

static void current_music_set(int cm){

    pthread_mutex_lock(&lock);
    g_current_music = cm;
    pthread_mutex_unlock(&lock);
}

static void random_init(MUSIC_STATE music_state,int max_index, int cm){

    struct timespec ttime = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &ttime);
    unsigned int seed = ttime.tv_sec+ttime.tv_nsec;
    srandom(seed);
    /*return max_index*random()/RAND_MAX;*/
}

static int next_music_random(MUSIC_STATE music_state,int max_index, int cm){

    int rd;
    random_init(music_state, max_index, cm);
    while(1){
        rd = max_index*random()/RAND_MAX;
        if(rd != cm)
            return rd;
    }
}

static int next_music_single(MUSIC_STATE music_state,int max_index, int cm){

    return cm;
}

static int next_music_sequence(MUSIC_STATE music_state,int max_index, int cm){

    if(music_state == MUSIC_NEXT || music_state == MUSIC_PLAYING){
        if(cm == max_index)
            cm = 0;
        else
            ++cm;
    }else if(music_state == MUSIC_PREVIOUS){
        if(cm == 0)
            cm = max_index;
        else
            --cm;
    }
    return cm;
}

static int type_next_music(int type, MUSIC_STATE music_state, int max_index, int cm){

    int next_music;
    int music_specific;

    music_specific = get_music_specific();

    if(music_specific > 0){
        next_music = music_specific;
        set_music_specific(-1);
        goto end;
    }

    switch (type) {
        case PLAY_TYPE_RANDOM:
            next_music = next_music_random(music_state, max_index, cm);
            break;

        case PLAY_TYPE_SINGLE:
            next_music = next_music_single(music_state, max_index, cm);
            break;

        case PLAY_TYPE_SEQUENCE:
            next_music = next_music_sequence(music_state, max_index, cm);
            break;
    }

end:

    return next_music;
}

static int music_play_internal(void *m){

    int cm;  /*current music id*/
    int ret;
    char *filename;
    snd_pcm_sframes_t delayp, delayp_total, pcm;
    SoundParam sp;
    FILE *file;
    int buff_size;
    char *buff;
    MUSIC_STATE music_state;
    int type;
    Music *music;

    music = m;
    cm = music->current;
    type = music->type;

    /*traverse the music list repeatly*/
    while(1){

        current_music_set(cm);
        if(music->call)
            music->call(cm);

        filename = music->list[cm];
        delayp = delayp_total = 0;

        /*printf("filename:%s\n", filename);*/
        set_param(filename, &sp);
        if((file = fopen(filename, "rb")) == NULL){
            printf("open file failed, %s\n", strerror(errno));
        }
        buff_size = sp.frames * sp.channels * 2;
        if((buff = malloc(buff_size)) == NULL){
            printf("memory error:%s\n", strerror(errno));
            file_close(&file);
            exit(-1);
        }
        /*file read loop*/
        while(1){
            music_state = music_state_check();
            if(music_state == MUSIC_NEXT || music_state == MUSIC_PREVIOUS){

                /*drop all data, play the next music*/
                ret = snd_pcm_drop(sp.pcm_handle);
                if(ret != 0)
                    printf("drop faild:%s\n", snd_strerror(ret));
                music_state_set(MUSIC_PLAYING);
                goto next_file;

            } 

            if(music_state == MUSIC_PLAYING
                    && snd_pcm_state(sp.pcm_handle) == SND_PCM_STATE_PAUSED){

                if((ret = snd_pcm_pause(sp.pcm_handle, 0)) != 0){
                    printf("Pcm continue failed, %s\n", snd_strerror(ret));
                    /*! TODO: Todo description here
                     *  \todo Todo description here
                     */
                }
            } 

            if(music_state == MUSIC_PAUSED){
                if(snd_pcm_state(sp.pcm_handle) != SND_PCM_STATE_PAUSED){
                    if((ret = snd_pcm_pause(sp.pcm_handle, 1)) != 0){
                        printf("Pcm pause failed, %s\n", snd_strerror(ret));
                        /*! TODO: Todo description here
                         *  \todo Todo description here
                         */
                    }
                }
                /*keep sleeping untile the music state changed to MUSIC_PLAYING*/
                sleep(1);
                continue;
            }

            /*we read the file data now*/
            size_t n = fread(buff, 1, buff_size, file);
            if(n != buff_size){
                if(ferror(file) != 0){
                    printf("read file error:%s\n", strerror(errno));
                }
            }

            /*printf("PCM name: %s, state: %s\n", snd_pcm_name(sp.pcm_handle), snd_pcm_state_name(snd_pcm_state(sp.pcm_handle)));*/
            /*write the date to the device*/
            if ((pcm = snd_pcm_writei(sp.pcm_handle, buff, sp.frames)) == -EPIPE) {
                printf("XRUN.\n");
                snd_pcm_prepare(sp.pcm_handle);
            } else if (pcm < 0) {
                printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
            }
            if(feof(file) != 0){
                printf("eof\n");
                break;
            }
        }

        /*we should have some cahce frames on the device to output.*/
        /*Wait until the device drained.*/
        while(1){

            /*if(snd_pcm_state(sp.pcm_handle) == SND_PCM_STATE_SETUP)*/
            /*break;*/
            if(snd_pcm_avail(sp.pcm_handle) < 0)
                break;

            music_state = music_state_check();

            if(music_state == MUSIC_PAUSED &&
                    snd_pcm_state(sp.pcm_handle) != SND_PCM_STATE_PAUSED){
                /*do nothing, enter the reading data loop again.Note that do not open a new file*/
                /*finished = 0;*/
                /*goto next_file;*/
                if((ret = snd_pcm_pause(sp.pcm_handle, 1)) != 0){
                    printf("Pcm pause failed, %s\n", snd_strerror(ret));
                    /*! TODO: Todo description here
                     *  \todo Todo description here
                     */
                }
            }

            if(music_state == MUSIC_PLAYING &&
                    snd_pcm_state(sp.pcm_handle) != SND_PCM_STATE_RUNNING){
                /*do nothing, enter the reading data loop again.Note that do not open a new file*/
                /*finished = 0;*/
                /*goto next_file;*/
                if((ret = snd_pcm_pause(sp.pcm_handle, 0)) != 0){
                    printf("Pcm continue failed, %s\n", snd_strerror(ret));
                    /*! TODO: Todo description here
                     *  \todo Todo description here
                     */
                }
            }
            if(music_state == MUSIC_NEXT || music_state == MUSIC_PREVIOUS){
                /*drop all data, play the next music*/
                ret = snd_pcm_drop(sp.pcm_handle);
                if(ret != 0)
                    printf("drop faild:%s\n", snd_strerror(ret));
                music_state_set(MUSIC_PLAYING);
                goto next_file;
            }
            sleep(1);
        }

        /*snd_pcm_drain(sp.pcm_handle);*/

next_file:
        file_close(&file);
        snd_pcm_close(sp.pcm_handle);
        free(buff);

        pthread_mutex_lock(&lock);
        type = g_music_play_type;
        pthread_mutex_unlock(&lock);
        cm = type_next_music(type, music_state, music->num-1, cm);
    }
}

static void audio_write(void* arg){

    SoundParam sp;
    int buff_size;
    FILE *file;
    char *buff;
    snd_pcm_sframes_t pcm;

    Audio audio;

    while(1){

        pthread_mutex_lock(&audio_lock);
        strcpy(audio.filename, g_audio.filename);
        audio.call = g_audio.call;
        pthread_mutex_unlock(&audio_lock);

        if(audio.filename[0] == 0) {
            sleep(1);
            continue;
        }

        set_param(audio.filename, &sp);

        if((file = fopen(audio.filename, "rb")) == NULL){
            printf("open file failed, %s\n", strerror(errno));
        }
        buff_size = sp.frames * sp.channels *2;
        if((buff = malloc(buff_size)) == NULL){
            printf("memory error:%s\n", strerror(errno));
            file_close(&file);
            exit(-1);
        }

        while(1){

            /*we read the file data now*/
            size_t n = fread(buff, 1, buff_size, file);
            if(n != buff_size){
                if(ferror(file) != 0){
                    printf("read file error:%s\n", strerror(errno));
                }
            }

            /*printf("PCM name: %s, state: %s\n", snd_pcm_name(sp.pcm_handle), snd_pcm_state_name(snd_pcm_state(sp.pcm_handle)));*/
            /*write the date to the device*/
            if ((pcm = snd_pcm_writei(sp.pcm_handle, buff, sp.frames)) == -EPIPE) {
                printf("XRUN.\n");
                snd_pcm_prepare(sp.pcm_handle);
            } else if (pcm < 0) {
                printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
            }
            if(feof(file) != 0){
                printf("eof\n");
                break;
            }
        }

        snd_pcm_drain(sp.pcm_handle);

        file_close(&file);
        snd_pcm_close(sp.pcm_handle);
        free(buff);

        pthread_mutex_lock(&audio_lock);
        g_audio.filename[0] = 0;
        g_audio.call = NULL;
        pthread_mutex_unlock(&audio_lock);

        if(audio.call)
            audio.call();

    }
}

static int music_copy(Music *music_dst, Music *music_src){

    if(!music_dst || !music_src){
        return -1;
    }
    music_dst->num = music_src->num;
    music_dst->list = malloc(sizeof(char*)*music_src->num);
    if(!music_dst->list){
        printf("memory error, %s", strerror(errno));
        exit(-1);
    }
    for (int i = 0; i < music_dst->num; i++) {
        music_dst->list[i] = malloc(strlen(music_src->list[i])+1);
        if(!music_dst->list[i]){
            printf("memory error, %s", strerror(errno));
            exit(-1);
        }
        strcpy(music_dst->list[i], music_src->list[i]);
    }
    music_dst->type = music_src->type;
    music_dst->call = music_src->call;

    return 0;
}

int music_init(Music* music){

    int ret;

    if(g_init_flag)
        return -1;

    if(music_copy(&g_music, music) != 0)
        return -1;

    if(pthread_mutex_init(&lock, NULL) != 0){
        printf("mutex init failed\n");
        return -1;
    }

    if((ret = pthread_create(&g_music_pt, NULL,music_play_internal, &g_music)) != 0){
        printf("create thread error:%s", strerror(errno));
        return -1;
    }
    return 0;
}

static void G_Music_destroy(){

    for (int i = 0; i < g_music.num; i++) {
        if(g_music.list[i]){
            free(g_music.list[i]);
            g_music.list[i] = NULL;
        }
    }
    free(g_music.list);
    g_music.num = 0;
    g_music.call= NULL;
}

int music_destory(){
    /*! TODO: mutex destory and thread exit
     *
     */

    pthread_mutex_destroy(&lock);
    pthread_cancel(g_music_pt);
    g_init_flag = 0;
    G_Music_destroy();
}

int music_play_type(MUSIC_PLAY_TYPE type){

    int type_i = (int)type;

    pthread_mutex_lock(&lock);
    g_music_play_type = type_i;
    pthread_mutex_unlock(&lock);

    return 0;
}

int audio_play(const char *filename, AUDIO_FINISHED_CALLBACK call){


    pthread_mutex_lock(&audio_lock);
    strcpy(&(g_audio.filename), filename);
    g_audio.call = call;
    pthread_mutex_unlock(&audio_lock);

    return 0;
}

int audio_init(){

    int ret;


    if(pthread_mutex_init(&audio_lock, NULL) != 0){
        printf("mutex init failed\n");
        return -1;
    }

    if((ret = pthread_create(&g_audio_pt, NULL, audio_write, NULL)) != 0){
        printf("create thread error:%s", strerror(errno));
        return -1;
    }
    return 0;
}

int audio_destroy(){

    pthread_mutex_destroy(&audio_lock);
    pthread_cancel(g_audio_pt);
    return 0;
}

int music_pause(){
    pthread_mutex_lock(&lock);
    if(g_music_state == MUSIC_PAUSED){
        printf("Music player already paused.\n");
        pthread_mutex_unlock(&lock);
        return -1;
    }
    g_music_state = MUSIC_PAUSED;
    pthread_mutex_unlock(&lock);
    return 0;
}

int music_play(){

    pthread_mutex_lock(&lock);
    if(g_music_state ==MUSIC_PLAYING){
        printf("Music playing.\n");
        pthread_mutex_unlock(&lock);
        return -1;
    }
    g_music_state = MUSIC_PLAYING;
    pthread_mutex_unlock(&lock);
    return 0;
}

int music_next(){
    pthread_mutex_lock(&lock);
    g_music_state = MUSIC_NEXT;
    pthread_mutex_unlock(&lock);
    return 0;
}

int music_previous(){
    pthread_mutex_lock(&lock);
    g_music_state = MUSIC_PREVIOUS;
    pthread_mutex_unlock(&lock);
    return 0;
}

int get_current_music(){

    int c;

    pthread_mutex_lock(&lock);
    c = g_current_music;
    pthread_mutex_unlock(&lock);

    return c;
}

int music_speccify(int id){

    /*check whether id is in the list*/
    set_music_specific(id);
    music_next();
    return 0;
}

int update_music_list(const char** music_list, int num){

    return 0;
}

static void SetAlsaMasterVolume(long volume)
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
    snd_mixer_selem_set_playback_volume_all(elem, volume * (maxl-min) / 100);

    snd_mixer_close(handle);
}

void toggle_volume(int volume){
    
    if(g_volume+volume > 100)
        g_volume = 100;
    else if(g_volume+volume < 0)
        g_volume = 0;
    else
        g_volume = g_volume+volume;
    SetAlsaMasterVolume(g_volume);
}

void volume_init(int volume){
    SetAlsaMasterVolume(volume);
    g_volume = volume;
}
