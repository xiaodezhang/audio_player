#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sound_playback.h"

char *file_list[3] = {"/home/zhang_xiaode/ChillingMusic.wav"
    , "/home/zhang_xiaode/tts_sample.wav"
        , "/home/zhang_xiaode/wav/06.城里的月光.wav"
};
void music_call(int cm){

    /*printf("current music :%d\n", cm);*/
}


void callback(){
    printf("call back\n");
}
void audio_call(){

}

void test_audio_play(){

    int i = 0;

    audio_init();
    audio_play(file_list[2], 50);
    while(++i){
        if(i == 3){
            printf("3333333\n");
            audio_play(file_list[0], 49);
            audio_play(file_list[1], 49);
        }
        if(i == 4){
            printf("444444\n");
            audio_play(file_list[1], 49);
        }
#if 1
        if(i == 5){
            printf("55555\n");
            audio_play(file_list[2], 51);
        }
        if(i == 8){
            printf("888888\n");
            audio_destroy();
            audio_init();
            audio_play(file_list[1], 50);
        }
#endif
        sleep(1);
    } 
}

int main(int argc, char **argv) {

#if 1
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
    music.num = 3;
    music.list = malloc(sizeof(char*)*music.num);
    for (int i = 0; i < music.num; i++) {
        music.list[i] = malloc(strlen(file_list[i])+1);
        strcpy(music.list[i], file_list[i]);
    }
    music.current = 2;
    /*music.type = PLAY_TYPE_SINGLE;*/
    music.type = PLAY_TYPE_SEQUENCE;
    /*music.type = PLAY_TYPE_RANDOM;*/
    music.call = music_call;

    music_init(&music);
    /*for(int i = 0;i < music.num; ++i){*/
        /*free(music.list[i]);*/
    /*}*/
    /*music_play();*/
    int i= 0;
    music_play();
    while(++i){
#if 0
        if(i == 10){
            music_specify(2);
            printf("2222222\n");
            printf("current music:%d\n",get_current_music());

            music_specify(0);
            printf("000000\n");
            printf("current music:%d\n",get_current_music());
        }
        if(i == 15){
            music_play_type(PLAY_TYPE_SINGLE);
            printf("1515151515\n");
            printf("current music:%d\n", get_music_play_type());
        }
#endif
#if 0
        if(i == 5){
            printf("555555555555\n");
            music_play();
        }
        if(i == 8){
            printf("8888888888888888888\n");
            music_previous();
        }

        if(i == 9){
            printf("1212121212121212\n");
            music_previous();
        }

        /*if(i == 35){*/
            /*printf("next\n");*/
            /*music_next();*/
        /*}*/
#endif
#if 0
        if(i == 40){
            printf("next\n");
            music_next();
        }
        if(i == 50){
            printf("5050505050\n");
            music_destory();
            free(music.list[1]);
            char *wav = "/home/zhang_xiaode/wav/3.wav";
            music.list[1] = malloc(strlen(wav)+1);
            strcpy(music.list[1], wav);
            music.current = 0;
            printf("music:%s\n", music.list[1]);
            music_init(&music);
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
