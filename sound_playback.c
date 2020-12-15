/*
 * Simple sound playback using ALSA API and libasound.
 *
 * Compile:
 * $ cc -o play sound_playback.c -lasound
 * 
 * Usage:
 * $ ./play <sample_rate> <channels> <seconds> < <file>
 * 
 * Examples:
 * $ ./play 44100 2 5 < /dev/urandom
 * $ ./play 22050 1 8 < /path/to/file.wav
 *
 * Copyright (C) 2009 Alessandro Ghedini <alessandro@ghedini.me>
 * --------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Alessandro Ghedini wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we
 * meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * --------------------------------------------------------------
 */

#include <alsa/asoundlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#define PCM_DEVICE "default"

pthread_mutex_t lock;

/*! \enum MUSIC_PLAY_TYPE
 *
 *  Detailed description
 */
typedef enum{ 
    PLAY_TYPE_RANDOM = 0,
    PLAY_TYPE_SEQUENCE,
    PLAY_TYPE_SINGLE
} MUSIC_PLAY_TYPE ;

typedef enum {
    MUSIC_PAUSED = 0,
    MUSIC_PLAYING,
} MUSIC_STATE ;

MUSIC_STATE g_music_state = 1;
typedef struct{
    char **list;
    int num;
    int current;
    MUSIC_PLAY_TYPE type;
}Music;

Music g_music;

typedef struct{
    snd_pcm_t *pcm_handle;
	snd_pcm_uframes_t frames;
    short int channels;
    int seconds;
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

/*0:music player 1:audio player*/
int audio_play(int playerid, const char *filename){

    if(playerid == 0){
        /*new player*/
        ;
    }
    return 0;
}
#if 0
static void check_pause(){

    MUSIC_STATE music_state;
    int ret;

    while(1){
        pthread_mutex_lock(&lock);
        music_state = g_music_state;
        pthread_mutex_unlock(&lock);
        if(music_state == MUSIC_PAUSED){
            if(snd_pcm_state(pcm_handle) != SND_PCM_STATE_SETUP){
                ret = snd_pcm_delay(&delay);
                if(ret != 0){
                    printf("pcm delay failed, %s\n", snd_strerror(ret));
                }
                ret = snd_pcm_drop(pcm_handle);
                if(ret != 0){
                    printf("drop failed:%s\n", snd_strerror(ret));
                }
                if(!)
            }
        }
    }
}
#endif

static void music_write(void* music){

    int buff_size;
    char *buff;
    SoundParam sp;
    unsigned int pcm;
    MUSIC_STATE music_state;

	sigset_t mask, oldmask;
    Music *m = music;
    FILE *f = NULL;
    char *filename;


    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, &oldmask);

    while(1){
        if(m->type == PLAY_TYPE_SINGLE){
            while(1){
                filename = m->list[m->current];
                set_param((const char*)filename, &sp);
                f = fopen(filename, "rb");
                if(!f)
                    return -1;
                buff_size = sp.frames * sp.channels * 2 /* 2 -> sample size */;
                printf("buff size:%d, frames:%d, channels:%d\n", buff_size, sp.frames, sp.channels);
                buff = (char *) malloc(buff_size);
                if(!buff){
                    printf("memory error\n");
                    fclose(f);
                    return;
                }
                int read_num = 0;
                int play_sec = 0;
                snd_pcm_sframes_t delayp;
                struct timespec tstart = {0,0}, tend = {0,0};
                clock_gettime(CLOCK_MONOTONIC, &tstart);
                while(1){
                    pthread_mutex_lock(&lock);
                    music_state = g_music_state;
                    pthread_mutex_unlock(&lock);

                    if(music_state == MUSIC_PLAYING){
                        if(snd_pcm_state(sp.pcm_handle) == SND_PCM_STATE_SETUP){
                            if(!f){
                                f = fopen(filename, "rb");
                                if(!f){
                                    printf("open file failed\n");
                                }
                                fseek(f, delayp, SEEK_SET);
                            }
                        }
                    }
                    if(music_state == MUSIC_PAUSED){
                        if(snd_pcm_state(sp.pcm_handle) != SND_PCM_STATE_SETUP){
                            int ret;
                            ret = snd_pcm_delay(sp.pcm_handle, &delayp);
                            printf("delay:%d\n", delayp);
                            if(ret != 0){
                                printf("pcm delay failed, %s\n", snd_strerror(ret));
                            }
                            ret = snd_pcm_drop(sp.pcm_handle);
                            if(ret != 0){
                                printf("drop faild:%s\n", snd_strerror(ret));
                            }
                            if(f){
                                fclose(f);
                                f = NULL;
                            }
                         sleep(1);
                         continue;
                        }
                    }

                    size_t n = fread(buff, 1, buff_size, f);
                    if(n != buff_size){
                        if(feof(f) != 0){
                            printf("file eof\n");
                            break;
                        }else{
                            printf("error:%s\n", strerror(errno));
                            break;
                        }
                    }

                    if (pcm = snd_pcm_writei(sp.pcm_handle, buff, sp.frames) == -EPIPE) {
                        printf("XRUN.\n");
                        snd_pcm_prepare(sp.pcm_handle);
                    } else if (pcm < 0) {
                        printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
                    }
                }

            clock_gettime(CLOCK_MONOTONIC, &tend);
            printf("time differ:%d\n",tend.tv_sec-tstart.tv_sec);
            int time_diff = tend.tv_sec-tstart.tv_sec;

            printf("finished\n");
            /*snd_pcm_drain(sp.pcm_handle);*/
            for(int i = sp.seconds-time_diff; i > 0; --i){
                pthread_mutex_lock(&lock);
                music_state = g_music_state;
                pthread_mutex_unlock (&lock);

                if(music_state == MUSIC_PAUSED && snd_pcm_state(sp.pcm_handle) != SND_PCM_STATE_SETUP){
                    int ret;
                    ret = snd_pcm_delay(sp.pcm_handle, &delayp);
                    printf("delay:%d\n", delayp);
                    if(ret != 0){
                        printf("pcm delay failed, %s\n", snd_strerror(ret));
                    }
                    ret = snd_pcm_drop(sp.pcm_handle);
                    if(ret != 0){
                        printf("drop faild:%s\n", snd_strerror(ret));
                    }
                    if(f){
                        fclose(f);
                        f = NULL;
                    }
                }
                if(music_state == MUSIC_PLAYING && snd_pcm_state(sp.pcm_handle) == SND_PCM_STATE_SETUP)
                    break;
                sleep(1);
            }
#if 0
            while(1){
                if(music_state == MUSIC_PLAYING)
                    break;
                sleep(1);
            }
#endif
            /*while(1){*/
                /*if(snd_pcm_state(sp.pcm_handle) == SND_PCM_STATE_SETUP)*/
                    /*break;*/
                /*printf("state3:%d\n",snd_pcm_state(sp.pcm_handle));*/
                /*sleep(1);*/
            /*}*/
            printf("state:%d\n", snd_pcm_state(sp.pcm_handle));
            printf("drain\n");
            snd_pcm_close(sp.pcm_handle);
            free(buff);
            if(f){
                fclose(f);
                f = NULL;
            }
          }

        }


        if(m->type == PLAY_TYPE_RANDOM){
            for (int i = 0; i < m->num; i++) {
                
            }
        }
        if(m->type == PLAY_TYPE_SEQUENCE){
        }

    }
}

static void audio_write(void* filename){

    int buff_size;
    char *buff;
    SoundParam sp;
    unsigned int pcm;
    printf("filename:%s\n", filename);

	sigset_t mask, oldmask;


	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &mask, &oldmask);

    set_param((const char*)filename, &sp);
    printf("param set\n");


    FILE *f = fopen(filename, "rb");
    if(!f)
        return -1;
	buff_size = sp.frames * sp.channels * 2 /* 2 -> sample size */;
    printf("buff size:%d, frames:%d, channels:%d\n", buff_size, sp.frames, sp.channels);
	buff = (char *) malloc(buff_size);
    if(!buff){
        printf("memory error\n");
        fclose(f);
        return;
    }
    while(1){
        size_t n = fread(buff, 1, buff_size, f);
        if(n != buff_size){
            if(feof(f) != 0){
                printf("file eof\n");
                break;
            }else{
                printf("error:%s\n", strerror(errno));
                break;
            }
        }

		if (pcm = snd_pcm_writei(sp.pcm_handle, buff, sp.frames) == -EPIPE) {
			printf("XRUN.\n");
			snd_pcm_prepare(sp.pcm_handle);
		} else if (pcm < 0) {
			printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
		}

	}
    printf("finished\n");

	snd_pcm_drain(sp.pcm_handle);
    printf("drain\n");
	snd_pcm_close(sp.pcm_handle);
	free(buff);
    fclose(f);
}

int music_init(Music* music){

    int ret;

    pthread_t music_pt;
#if 0
    Filelist fl;
    fl.list = malloc(num*sizeof(char*));
    if(!fl.list){
        printf("memory error\n");
        return -1;
    }
    for(int i = 0;i < num; ++i){
        fl.list[i] = malloc(strlen(file_list[i])+1);
        strcpy(fl.list[i], file_list[i]);
        printf("%s\n",*(fl.list+i));
    }
    fl.num = num;
    exit(0);
#endif
    if((ret = pthread_create(&music_pt, NULL, music_write, music)) != 0){
        printf("create thread error:%s", strerror(errno));
        return -1;
    }
    return 0;
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



int set_param(const char *filename, SoundParam* sp){

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
    printf("set param, frames:%d\n", sp->frames);
    printf("can pause:%d\n", snd_pcm_hw_params_can_pause(params));
}

int music_pause(){
    printf("00000000000000000000\n");
    pthread_mutex_lock(&lock);
    printf("111111111111111111111111\n");
    if(g_music_state == MUSIC_PAUSED){
        printf("Music player already paused.\n");
        pthread_mutex_unlock(&lock);
        return -1;
    }
    g_music_state = MUSIC_PAUSED;
    pthread_mutex_unlock(&lock);
    printf("music paused\n");
}


int music_play(){

    printf("88888888888888888\n");
    pthread_mutex_lock(&lock);
    printf("9999999999999999999999\n");
    if(g_music_state ==MUSIC_PLAYING){
        printf("Music playing.\n");
        pthread_mutex_unlock(&lock);
        return -1;
    }
    g_music_state = MUSIC_PLAYING;
    printf("music play continue\n");
    pthread_mutex_unlock(&lock);

}

int main(int argc, char **argv) {

    if(pthread_mutex_init(&lock, NULL) != 0){
        printf("mutex init failed\n");
        return -1;
    }
#if 1
    char *file_list[2] = {"/home/zhang_xiaode/ChillingMusic.wav", "/home/zhang_xiaode/tts_sample.wav"};
    /*audio_play_init(argv[1]);*/

    Music music;
    music.list = malloc(sizeof(char*)*2);
    music.list[0] = malloc(strlen(file_list[0])+1);
    strcpy(music.list[0], file_list[0]);
    music.list[1] = malloc(strlen(file_list[1])+1);
    strcpy(music.list[1], file_list[1]);
    music.num = 2;
    music.current = 0;
    music.type = PLAY_TYPE_SINGLE;

    music_init(&music);
    /*music_play();*/
    int i= 0;
    while(++i){
#if 1
       if(i == 7){
           music_play();
           printf("7777777777777\n");
       }
       if(i == 3){
           printf("3333333\n");
           music_pause();
       }
#if 1
       if(i  == 5){
           printf("555555\n");
           music_play();
       }
       if(i == 10){
           printf("10101010101010\n");
           music_pause();
       }
#endif
#endif
       sleep(1);
    }
    pthread_mutex_destroy(&lock);
#else
    audio_write(argv[1]);

#endif
	return 0;
}
