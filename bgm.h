/* 音声管理 */

static int playing_bgm = 0;

void stop_bgm(void){
	if (playing_bgm){
		PlaySoundA(NULL, NULL, 0);
		playing_bgm = 0;
	}
}
void play_bgm(const char* wav_file, int loop){
	if (playing_bgm || (read_conf(conf_file_name, "audio") == 0)){
		return;
	}
	FILE* fp = fopen(wav_file, "rb");
	if (fp == NULL){
		return;
	}
	fclose(fp);
	if (loop){
		PlaySoundA(wav_file, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
	} else {
		PlaySoundA(wav_file, NULL, SND_FILENAME | SND_ASYNC);
	}
	playing_bgm = 1;
}


