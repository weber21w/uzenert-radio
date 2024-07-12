#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>

pthread_t station_info_thread;
pthread_t stream_audio_thread;

void *StationInfo();
void *StreamAudio();

char station_info_buf[4096];//this is read by main
char station_info_work[4096];//this is written to by thread
volatile int station_info_updated = 0;

volatile int stream_new_data = 0;

unsigned char buf_out[
/* Processess user commands and audio streaming for 1 client */

int TriggerLocalCommand(char *in_buf, char *out_buf, int max_chars, int max_time, char *stop_cue, char *fail_cue){

	fflush(stdout);
	FILE *fp = popen(in_buf, "r");

	if(fp == NULL){
	
		printf("\e[31;3mERROR TriggerLocalCommand() \"%s\" failed\n\e[0m", in_buf);
		return -1;
	}else
		fflush(fp);
	fflush(stdout);
	fflush(stdin);
	
	int boff = 0;
	out_buf[boff] = 0;//make sure it is string terminated
	int run_time = 0;
	int progress_time = 0;
	
	while(1){
		char c = fgetc(fp);
		
		if(out_buf != NULL)
			out_buf[boff] = c;
			
		if(out_buf[boff] == EOF || out_buf[boff] == 0){
			
			pclose(fp);
			out_buf[boff] = 0;//terminate the buffer as a string
			putchar('\r');//clean up for the new line we didn't print after the banner(for ... progress bar)
			return 1;
		}else{
		//	putchar(out_buf[boff]);
			out_buf[++boff] = 0;
		}
		if(max_chars){//infinite chars if 0
		
			if(--max_chars == 0){//chars ran out
			
				pclose(fp);
				putchar('\r');
				return 2;
			}
		}

		if(stop_cue != NULL && strstr(out_buf, stop_cue) != NULL){//did we find the stop keyword?
			
			pclose(fp);
			putchar('\r');
			return 4;
		}
		
		if(fail_cue != NULL && strstr(out_buf, fail_cue) != NULL){//did we find the fail keyword?
			//this can be abused for something else. the intent is just to have a "success" or "fail" return
			//for common items
			pclose(fp);
			putchar('\r');
			return 5;
		}
		fflush(fp);
		fflush(stdout);
		fflush(stdin);
	}
	
	pclose(fp);
	putchar('\r');
	return 0;
}


int main(){
	pthread_create(&stream_audio_thread, NULL, StreamAudio, NULL);
	
	while(1){
		if(station__updated){//info thread updated their work buffer, we copy it over and signal that we are done
			memcpy(station_info_buf, station_info_work, sizeof(station_info_buf));
asm volatile("mfence":::"memory");
			station_info_updated = 0;//signal to thread it can continue
asm volatile("mfence":::"memory");

		}

		if(stream_new_data){
			
		}
	}	
	return 0;
}

/*
ffprobe -i 'http://hyperadio.ru:8000/live' -hide_banner
[mp3 @ 0x55bc2079bec0] Skipping 402 bytes of junk at 0.
Input #0, mp3, from 'http://hyperadio.ru:8000/live':
  Metadata:
    icy-br          : 128
    icy-description : demoscene radio
    icy-genre       : demoscene, 8bit, vgm, chiptune, tracked
    icy-name        : HYPERADIO
    icy-pub         : 0
    icy-url         : https://hyperadio.retroscene.org
    icy-aim         : https://t.me/hyperadio
    StreamTitle     : Trauma Child Genesis - Traumatique | 05:36
  Duration: N/A, start: 0.000000, bitrate: 128 kb/s
  Stream #0:0: Audio: mp3, 44100 Hz, stereo, fltp, 128 kb/s
*/

void *StationInfo(){//periodically retrieve the streaming station's info
	char station_info_cmd[2048];
	while(1){
		if(station_info_updated)//waiting for main to clear this(signaling that it copied the new data over)
			continue;

		if(station_info_wait){//main wants us to wait(changing target buffer)
			station_info_is_waiting = 1;//let the main thread know we see this
			continue;
		}
asm volatile("mfence":::"memory");

		fflush(stdout);
		fflush(stdin);
		snprintf(station_info_cmd, sizeof(station_info_cmd)-1, "ffprobe -i '%s' -hide_banner 2>&1 | grep -e icy-name -e icy-description -e icy-genre -e StreamTitle | awk -F: '{ print $2 }' | sed -e 's/^[ \t]*//'", target_station);

		FILE *fp = popen(station_info_cmd);

		if(fp == NULL){
			sleep(10);
			continue;
		}
		
		int info_off = 0;
		while(!feof(fp)){
			station_info_work[info_off++] = fgetc(fp);
			if(info_off == sizeof(station_info_work)-1)
				break;
		}
		close(fp);
asm volatile("mfence":::"memory");

		station_info_updated = 1;//let main know we updated the buffer(we wait for aknowledgement)

		sleep(10);
	}
}
