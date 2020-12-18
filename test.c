#include <stdio.h>
#include <unistd.h>

#include "sound_playback.h"

char *file_list[3] = {"/home/zhang_xiaode/ChillingMusic.wav"
    , "/home/zhang_xiaode/tts_sample.wav"
        , "/home/zhang_xiaode/wav/4.wav"
};
void music_call(int cm){

    printf("current music :%d\n", cm);
}


void callback(){
    printf("call back\n");
}
void audio_call(){
    audio_play(file_list[2], callback);
}

void test_audio_play(){


    audio_init();
    audio_play(file_list[1], audio_call);
    while(1) sleep(1);
}

int main(int argc, char **argv) {

#if 0
    test_audio_play();
    return 0;
#endif
#if 0
    volume_init(100);
    toggle_volume(-20);
    return 0;
#endif

#if 1

    Music music;
    music.list = malloc(sizeof(char*)*3);
    music.list[0] = malloc(strlen(file_list[0])+1);
    strcpy(music.list[0], file_list[0]);
    music.list[1] = malloc(strlen(file_list[1])+1);
    strcpy(music.list[1], file_list[1]);
    music.list[2] = malloc(strlen(file_list[2])+1);
    strcpy(music.list[2], file_list[2]);
    music.num = 3;
    music.current = 1;
    /*music.type = PLAY_TYPE_SINGLE;*/
    /*music.type = PLAY_TYPE_SEQUENCE;*/
    music.type = PLAY_TYPE_RANDOM;
    music.call = music_call;

    music_init(&music);
    /*music_play();*/
    int i= 0;
    while(++i){
        if(i == 30){
            printf("202020202020\n");
            music_pause();
        }
#if 1
        if(i == 32){
            printf("202020202020\n");
            music_play();
        }

        if(i == 35){
            printf("next\n");
            music_next();
        }
        if(i == 40){
            printf("next\n");
            music_next();
        }
#endif
#if 0
        if(i == 3 || i == 10){

            music_next();
        }
#endif

#if 0
       if(i == 7){
           printf("3333333\n");
           music_pause();
       }
       if(i == 20){
           printf("7777777777777\n");
           music_play();
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
