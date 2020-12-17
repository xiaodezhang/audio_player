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

pthread_mutex_t lock;
long g_volume = 100;

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

static int next_music_random(MUSIC_STATE music_state,int max_index, int cm){

    struct timespec ttime = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &ttime);
    unsigned int seed = ttime.tv_sec+ttime.tv_nsec;
    srandom(seed);
    return max_index*random()/RAND_MAX;
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


static int music_play_internal(Music *music,NEXT_MUSIC next_music){

    int cm;  /*current music id*/
    int ret;
    char *filename;
    snd_pcm_sframes_t delayp = 0, pcm;
    SoundParam sp;
    FILE *file;
    int buff_size;
    char *buff;
    MUSIC_STATE music_state;
    int finished = 1;

    cm = music->current;

    /*traverse the music list repeatly*/
    while(1){
        if(finished){
            /*the previous file written finished */
            filename = music->list[cm];

            set_param(filename, &sp);
            if((file = fopen(filename, "rb")) == NULL){
                printf("open file failed, %s\n", strerror(errno));
            }
            buff_size = sp.frames * sp.channels *2;
            if((buff = malloc(buff_size)) == NULL){
                printf("memory error:%s\n", strerror(errno));
                fclose(file);
                exit(-1);
            }
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
                        && snd_pcm_state(sp.pcm_handle) == SND_PCM_STATE_PREPARED){

                if(!file){
                    /*remaing paused state, open the file for writing.*/
                    if((file = fopen(filename, "rb")) == NULL){
                        printf("open file failed, %s\n", strerror(errno));
                    }
                    fseek(file, delayp, SEEK_SET);
                } 
            } 

            if(music_state == MUSIC_PAUSED){
                if(snd_pcm_state(sp.pcm_handle) != SND_PCM_STATE_SETUP){

                    /*save the delay, drop all data, we will resume when music state is MUSCI_PLAYING*/
                    if((ret = snd_pcm_delay(sp.pcm_handle, &delayp)) != 0){
                        printf("Pcm delay failed, %s\n", snd_strerror(ret));
                        /*! TODO: Todo description here
                         *  \todo Todo description here
                         */
                    }
                    printf("pause delay:%d\n", delayp);

                    if((ret = snd_pcm_drop(sp.pcm_handle)) != 0){
                        printf("Drop pcm failed, %s\n", snd_strerror(ret));
                        /*! TODO: Todo description here
                         *  \todo Todo description here
                         */
                    }
                    snd_pcm_prepare(sp.pcm_handle);
                    file_close(&file);
                }
                /*keep sleeping untile the music state changed to MUSIC_PLAYING*/
                sleep(1);
                printf("sleep\n");
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
        if((ret = snd_pcm_delay(sp.pcm_handle, &delayp)) != 0){
            printf("Pcm delay failed, %s\n", snd_strerror(ret));
        }

        int delay_sec = delayp / sp.avg_bytes_per_sec;
        printf("delay sec:%d,sec:%d\n", delay_sec, sp.seconds);

        for(int i = sp.seconds-delay_sec; i > 0; --i){

            music_state = music_state_check();

            if(music_state == MUSIC_PAUSED){
                /*do nothing, enter the reading data loop again.Note that do not open a new file*/
                finished = 0;
                goto next_file;
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
        snd_pcm_drain(sp.pcm_handle);

next_file:
        file_close(&file);
        if(music_state != MUSIC_PAUSED){
            snd_pcm_close(sp.pcm_handle);
            cm = next_music(music_state, music->num-1, cm);
            free(buff);
            finished = 1;
        }
    }
    
}

static int music_sequence_play(Music *music){

    music_play_internal(music, next_music_sequence);
}

static int music_random_play(Music *music){

    music_play_internal(music, next_music_random);
}

static int music_signle_play(Music *music){

    music_play_internal(music, next_music_single);
}

static void music_write(void* m){
    
    Music* music = m;
    switch (music->type) {
        case PLAY_TYPE_RANDOM:
            music_random_play(music);
            break;

        case PLAY_TYPE_SINGLE:
            music_signle_play(music);
            break;

        case PLAY_TYPE_SEQUENCE:
            music_sequence_play(music);
            break;
    }
}

static void audio_write(void* filename){

}

int music_init(Music* music){

    int ret;
    pthread_t music_pt;

    if(pthread_mutex_init(&lock, NULL) != 0){
        printf("mutex init failed\n");
        return -1;
    }

    if((ret = pthread_create(&music_pt, NULL, music_write, music)) != 0){
        printf("create thread error:%s", strerror(errno));
        return -1;
    }
    return 0;
}

int music_destory(){
    /*! TODO: mutex destory and thread exit
     *
     */
    pthread_mutex_destroy(&lock);
}

int audio_play_init(const char *filename){

    int ret;

    pthread_t audio_pt;
    if((ret = pthread_create(&audio_pt, NULL, audio_write, filename)) != 0){
        printf("create thread error:%s", strerror(errno));
        return -1;
    }
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

