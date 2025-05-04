#!/usr/bin/env php
<?php

foreach (scandir('./') as $f){
	if (strpos($f, '.mp3') !== false){
		if (file_exists(str_replace('.mp3', '.wav', $f))){
			unlink(str_replace('.mp3', '.wav', $f));
		}
		exec('ffmpeg -i "'.$f.'" -vn -ac 2 -ar 44100 -acodec pcm_s16le -f wav "'.str_replace('.mp3', '.wav', $f).'"');
	}
}


