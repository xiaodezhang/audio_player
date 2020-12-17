#include <stdio.h>
#include <unistd.h>

#include "sound_playback.h"

char *file_list[2] = {"/home/zhang_xiaode/ChillingMusic.wav", "/home/zhang_xiaode/tts_sample.wav"};
void music_call(int cm){

    printf("current music :%d\n", cm);
}


void callback(){
    printf("call back\n");
}
void audio_call(){
    audio_play(file_list[0], callback);
}

void test_audio_play(){


    audio_init();
    audio_play(file_list[1], audio_call);
    while(1) sleep(1);
}

int main(int argc, char **argv) {

    test_audio_play();
    return 0;
#if 0
    volume_init(100);
    toggle_volume(-20);
    return 0;
#endif

#if 1
    char *file_list[3] = {"/home/zhang_xiaode/ChillingMusic.wav"
               , "/home/zhang_xiaode/tts_sample.wav"
               , "/home/zhang_xiaode/wav/2.wav"
    };

    Music music;
    music.list = malloc(sizeof(char*)*3);
    music.list[0] = malloc(strlen(file_list[0])+1);
    strcpy(music.list[0], file_list[0]);
    music.list[1] = malloc(strlen(file_list[1])+1);
    strcpy(music.list[1], file_list[1]);
    music.list[2] = malloc(strlen(file_list[1])+1);
    strcpy(music.list[2], file_list[1]);
    music.num = 3;
    music.current = 2;
    /*music.type = PLAY_TYPE_SINGLE;*/
    music.type = PLAY_TYPE_SEQUENCE;
    music.call = music_call;

    music_init(&music);
    /*music_play();*/
    int i= 0;
    while(++i){
#if 0
        if(i == 3 || i == 10){

            music_next();
        }
#endif

#if 1
       if(i == 7){
           printf("3333333\n");
           music_pause();
       }
       if(i == 20){
           music_play();
           printf("7777777777777\n");
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
       if(i == 12){
           printf("12\n");
           music_play();
       }
#endif
#endif
       sleep(1);
    }
#else

#endif

    music_destory();
	return 0;
}
