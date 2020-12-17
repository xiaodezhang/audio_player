test:
	gcc -o test test.c sound_playback.c -lpthread -lasound
clean:
	rm  test
