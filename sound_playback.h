#ifndef SOUND_PLAYBACK_H
#define SOUND_PLAYBACK_H

typedef enum{ 
    PLAY_TYPE_RANDOM = 0,
    PLAY_TYPE_SEQUENCE,
    PLAY_TYPE_SINGLE
} MUSIC_PLAY_TYPE ;


typedef struct{
    char **list;
    int num;
    int current;
    MUSIC_PLAY_TYPE type;
}Music;

int music_init(Music *music);
int music_destory();

int music_play();
int music_pause();
int music_next();
int music_previous();


void volume_init(int volume);
void toggle_volume(int volume);


#endif
