//    LightMP3
//    Copyright (C) 2007 Sakya
//    sakya_tg@yahoo.it
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#include <psp2/io/fcntl.h>
#include <psp2/kernel/threadmgr.h>
#include <string.h>

//#include "tremor/ivorbiscodec.h" //libtremor
//#include "tremor/ivorbisfile.h"  //libtremor
#include <vorbis/codec.h>      //ogg-vorbis
#include <vorbis/vorbisfile.h> //ogg-vorbis
#include "player.h"
#include "oggplayer.h"

/////////////////////////////////////////////////////////////////////////////////////////
//Globals
/////////////////////////////////////////////////////////////////////////////////////////
static int OGG_audio_channel;
static int OGG_channels = 0;
static char OGG_fileName[264];
static int OGG_file = -1;
static OggVorbis_File OGG_VorbisFile;
static int OGG_eos = 0;
static struct fileInfo OGG_info;
static int OGG_isPlaying = 0;
static unsigned int OGG_volume_boost = 0.0;
static double OGG_milliSeconds = 0.0;
static int OGG_playingSpeed = 0; // 0 = normal
static int OGG_playingDelta = 0;
static int outputInProgress = 0;
static long OGG_suspendPosition = -1;
static long OGG_suspendIsPlaying = 0;
int OGG_defaultCPUClock = 50;
static short OGG_mixBuffer[VITA_NUM_AUDIO_SAMPLES * 2 * 2]__attribute__ ((aligned(64)));
static unsigned long OGG_tempmixleft = 0;
static double OGG_newFilePos = -1;
static int OGG_tagRead = 0;

/////////////////////////////////////////////////////////////////////////////////////////
//Audio callback
/////////////////////////////////////////////////////////////////////////////////////////
static void oggDecodeThread(void *_buf2, unsigned int numSamples, void *pdata){
    short *_buf = (short *)_buf2;
    //static short OGG_mixBuffer[VITA_NUM_AUDIO_SAMPLES * 2 * 2]__attribute__ ((aligned(64)));
    //static unsigned long OGG_tempmixleft = 0;
	int current_section;

	if (OGG_isPlaying) {	// Playing , so mix up a buffer
        outputInProgress = 1;
		while (OGG_tempmixleft < numSamples) {	//  Not enough in buffer, so we must mix more
			unsigned long bytesRequired = (numSamples - OGG_tempmixleft) * 4;	// 2channels, 16bit = 4 bytes per sample
			//unsigned long ret = ov_read(&OGG_VorbisFile, (char *) &OGG_mixBuffer[OGG_tempmixleft * 2], bytesRequired, &current_section); //libtremor
            unsigned long ret = ov_read(&OGG_VorbisFile, (char *) &OGG_mixBuffer[OGG_tempmixleft * 2], bytesRequired, 0, 2, 1, &current_section); //ogg-vorbis
			if (!ret) {	//EOF
                OGG_isPlaying = 0;
				OGG_eos = 1;
                outputInProgress = 0;
				return;
			} else if (ret < 0) {
				if (ret == OV_HOLE)
					continue;
                OGG_isPlaying = 0;
				OGG_eos = 1;
                outputInProgress = 0;
				return;
			}
			OGG_tempmixleft += ret / 4;	// back down to sample num
		}
        OGG_info.instantBitrate = ov_bitrate_instant(&OGG_VorbisFile);
		OGG_milliSeconds = ov_time_tell(&OGG_VorbisFile);

        if (OGG_newFilePos >= 0)
        {
            ov_raw_seek(&OGG_VorbisFile, (ogg_int64_t)OGG_newFilePos);
            OGG_newFilePos = -1;
        }

        //Check for playing speed:
        if (OGG_playingSpeed){
            if (ov_raw_seek(&OGG_VorbisFile, ov_raw_tell(&OGG_VorbisFile) + OGG_playingDelta) != 0)
                OGG_setPlayingSpeed(0);
        }

		if (OGG_tempmixleft >= numSamples) {	//  Buffer has enough, so copy across
			int count, count2;
			short *_buf2;
            //Volume boost:
            if (!OGG_volume_boost){
                for (count = 0; count < VITA_NUM_AUDIO_SAMPLES; count++) {
                    count2 = count + count;
                    _buf2 = _buf + count2;
                    *(_buf2) = OGG_mixBuffer[count2];
                    *(_buf2 + 1) = OGG_mixBuffer[count2 + 1];
                }
            }else{
                for (count = 0; count < VITA_NUM_AUDIO_SAMPLES; count++) {
                    count2 = count + count;
                    _buf2 = _buf + count2;
                    *(_buf2) = volume_boost(&OGG_mixBuffer[count2], &OGG_volume_boost);
                    *(_buf2 + 1) = volume_boost(&OGG_mixBuffer[count2 + 1], &OGG_volume_boost);
                }
            }
			//  Move the pointers
			OGG_tempmixleft -= numSamples;
			//  Now shuffle the buffer along
			for (count = 0; count < OGG_tempmixleft * 2; count++)
			    OGG_mixBuffer[count] = OGG_mixBuffer[numSamples * 2 + count];
		}
        outputInProgress = 0;
    } else {			//  Not Playing , so clear buffer
        int count;
        for (count = 0; count < numSamples * 2; count++)
            *(_buf + count) = 0;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
//Callback for vorbis
/////////////////////////////////////////////////////////////////////////////////////////
size_t ogg_callback_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    int res = sceIoRead(*(int *) datasource, ptr, size * nmemb);
	if (res == 0x80010013) {
		OGG_file = sceIoOpen(OGG_fileName, SCE_O_RDONLY, 0777);
		if (OGG_file >= 0) {
			sceIoLseek32(OGG_file, (uint32_t)OGG_getFilePosition(), SCE_SEEK_SET);
		}

		res = sceIoRead(*(int *) datasource, ptr, size * nmemb);
	}
	return res;
}
int ogg_callback_seek(void *datasource, ogg_int64_t offset, int whence)
{
    int res = sceIoLseek32(*(int *) datasource, (unsigned int) offset, whence);
	if (res == 0x80010013) {
		OGG_file = sceIoOpen(OGG_fileName, SCE_O_RDONLY, 0777);
		if (OGG_file >= 0) {
			sceIoLseek32(OGG_file, (uint32_t)OGG_getFilePosition(), SCE_SEEK_SET);
		}

	    res = sceIoLseek32(*(int *) datasource, (unsigned int) offset, whence);
	}
	return res;
}
long ogg_callback_tell(void *datasource)
{
    int res = sceIoLseek32(*(int *) datasource, 0, SEEK_CUR);
	if (res == 0x80010013) {
		OGG_file = sceIoOpen(OGG_fileName, SCE_O_RDONLY, 0777);
		if (OGG_file >= 0) {
			sceIoLseek32(OGG_file, (uint32_t)OGG_getFilePosition(), SCE_SEEK_SET);
		}

	    res = sceIoLseek32(*(int *) datasource, 0, SEEK_CUR);
	}
	return (long)res;
}
int ogg_callback_close(void *datasource)
{
    int res = sceIoClose(*(int *) datasource);
	if (res == 0x80010013) {
		OGG_file = sceIoOpen(OGG_fileName, SCE_O_RDONLY, 0777);
		if (OGG_file >= 0) {
			sceIoLseek32(OGG_file, (uint32_t)OGG_getFilePosition(), SCE_SEEK_SET);
		}

		res = sceIoClose(*(int *) datasource);
	}
	return res;
}

void readOggTagData(char *source, char *dest){
    int count = 0;
    int i = 0;

    strcpy(dest, "");
    for (i=0; i<strlen(source); i++){
        if ((unsigned char)source[i] >= 0x20 && (unsigned char)source[i] <= 0xfd){
            dest[count] = source[i];
            if (++count >= 256)
                break;
        }
    }
    dest[count] = '\0';
}

void splitComment(char *comment, char *name, char *value){
	char *result = NULL;
	result = strtok(comment, "=");
	int count = 0;

	while(result != NULL && count < 2){
		if (strlen(result) > 0){
			switch (count){
				case 0:
					strncpy(name, result, 30);
					name[30] = '\0';
					break;
				case 1:
					readOggTagData(result, value);
					value[256] = '\0';
					break;
			}
			count++;
		}
		result = strtok(NULL, "=");
	}
}

void getOGGTagInfo(OggVorbis_File *inVorbisFile, struct fileInfo *targetInfo){
	int i;
	char name[31];
	char value[257];

	vorbis_comment *comment = ov_comment(inVorbisFile, -1);
	for (i=0;i<comment->comments; i++){
		splitComment(comment->user_comments[i], name, value);
		if (!strcasecmp(name, "TITLE"))
			strcpy(targetInfo->title, value);
		else if(!strcasecmp(name, "ALBUM"))
			strcpy(targetInfo->album, value);
		else if(!strcasecmp(name, "ARTIST"))
			strcpy(targetInfo->artist, value);
		else if(!strcasecmp(name, "GENRE"))
			strcpy(targetInfo->genre, value);
		else if(!strcasecmp(name, "DATE") || !strcasecmp(name, "YEAR")){
            strncpy(targetInfo->year, value, 4);
            targetInfo->year[4] = '\0';
		}else if(!strcasecmp(name, "TRACKNUMBER")){
            strncpy(targetInfo->trackNumber, value, 7);
			targetInfo->trackNumber[7] = '\0';
		}
		/*else if(!strcmp(name, "COVERART_UUENCODED")){
            FILE *out = fopen("ms0:/coverart.jpg", "wb");
            FILE *outEnc = fopen("ms0:/coverart.txt", "wb");
            unsigned char base64Buffer[MAX_IMAGE_DIMENSION];
            fwrite(&comment->user_comments[i][19], 1, comment->comment_lengths[i] - 19, outEnc);
            int outChars = base64Decode(comment->comment_lengths[i], comment->user_comments[i], MAX_IMAGE_DIMENSION, base64Buffer);
            fwrite(base64Buffer, 1, outChars, out);
            fclose(outEnc);
            fclose(out);
        }*/
	}

    OGG_info = *targetInfo;
    OGG_tagRead = 1;
}

void OGGgetInfo(){
    //Estraggo le informazioni:
    OGG_info.fileType = OGG_TYPE;
    OGG_info.defaultCPUClock = OGG_defaultCPUClock;
    OGG_info.needsME = 0;

    vorbis_info *vi = ov_info(&OGG_VorbisFile, -1);
	OGG_info.kbit = vi->bitrate_nominal/1000;
    OGG_info.instantBitrate = vi->bitrate_nominal;
	OGG_info.hz = vi->rate;
	OGG_info.length = (long)ov_time_total(&OGG_VorbisFile, -1)/1000;
    if (vi->channels == 1){
        strcpy(OGG_info.mode, "single channel");
		OGG_channels = 1;
    }else if (vi->channels == 2){
        strcpy(OGG_info.mode, "normal LR stereo");
		OGG_channels = 2;
	}
    strcpy(OGG_info.emphasis, "no");

	int h = 0;
	int m = 0;
	int s = 0;
	long secs = OGG_info.length;
	h = secs / 3600;
	m = (secs - h * 3600) / 60;
	s = secs - h * 3600 - m * 60;
	snprintf(OGG_info.strLength, sizeof(OGG_info.strLength), "%2.2i:%2.2i:%2.2i", h, m, s);

    if (!OGG_tagRead)
        getOGGTagInfo(&OGG_VorbisFile, &OGG_info);
}


void OGG_Init(int channel){
    initAudioLib();
    MIN_PLAYING_SPEED=-119;
    MAX_PLAYING_SPEED=119;
    initFileInfo(&OGG_info);
    OGG_tagRead = 0;
    OGG_audio_channel = channel;
    OGG_milliSeconds = 0.0;
    OGG_tempmixleft = 0;
    memset(OGG_mixBuffer, 0, sizeof(OGG_mixBuffer));
    vitaAudioSetChannelCallback(OGG_audio_channel, oggDecodeThread, NULL);
}


int OGG_Load(char *filename){
	outputInProgress = 0;
	OGG_isPlaying = 0;
	OGG_milliSeconds = 0;
	OGG_eos = 0;
    OGG_playingSpeed = 0;
    OGG_playingDelta = 0;
	strcpy(OGG_fileName, filename);
	//Apro il file OGG:
    OGG_file = sceIoOpen(OGG_fileName, SCE_O_RDONLY, 0777);
	if (OGG_file >= 0) {
        OGG_info.fileSize = sceIoLseek(OGG_file, 0, SCE_SEEK_END);
        sceIoLseek(OGG_file, 0, SCE_SEEK_SET);
        ov_callbacks ogg_callbacks;

        ogg_callbacks.read_func = ogg_callback_read;
        ogg_callbacks.seek_func = ogg_callback_seek;
        ogg_callbacks.close_func = ogg_callback_close;
        ogg_callbacks.tell_func = ogg_callback_tell;
		if (ov_open_callbacks(&OGG_file, &OGG_VorbisFile, NULL, 0, ogg_callbacks) < 0){
            sceIoClose(OGG_file);
            OGG_file = -1;
            return ERROR_OPENING;
        }
	}else{
		return ERROR_OPENING;
	}

	OGGgetInfo();
    //Controllo il sample rate:
    if (vitaAudioSetFrequency(OGG_audio_channel, OGG_info.hz) < 0){
        OGG_FreeTune();
        return ERROR_INVALID_SAMPLE_RATE;
    }
	return OPENING_OK;
}

int OGG_IsPlaying() {
	return OGG_isPlaying;
}

int OGG_Play(){
	OGG_isPlaying = 1;
	return 0;
}

void OGG_Pause(){
	OGG_isPlaying = !OGG_isPlaying;
}

int OGG_Stop(){
	OGG_isPlaying = 0;
    //This is to be sure that oggDecodeThread isn't messing with &OGG_VorbisFile
    while (outputInProgress == 1)
        sceKernelDelayThread(100000);
	return 0;
}

void OGG_FreeTune(){
	ov_clear(&OGG_VorbisFile);
    if (OGG_file >= 0)
        sceIoClose(OGG_file);
    OGG_file = -1;
    OGG_tempmixleft = 0;
    memset(OGG_mixBuffer, 0, sizeof(OGG_mixBuffer));
}

void OGG_GetTimeString(char *dest){
	char timeString[9];
	long secs = (long)OGG_milliSeconds/1000;
	int h = secs / 3600;
	int m = (secs - h * 3600) / 60;
	int s = secs - h * 3600 - m * 60;
	snprintf(timeString, sizeof(timeString), "%2.2i:%2.2i:%2.2i", h, m, s);
	strcpy(dest, timeString);
}


int OGG_EndOfStream(){
	return OGG_eos;
}

struct fileInfo *OGG_GetInfo(){
	return &OGG_info;
}


struct fileInfo OGG_GetTagInfoOnly(char *filename){
    int tempFile = -1;
    OggVorbis_File vf;
    struct fileInfo tempInfo;

    strcpy(OGG_fileName, filename);
    initFileInfo(&tempInfo);
	//Apro il file OGG:
	tempFile = sceIoOpen(filename, SCE_O_RDONLY, 0777);
	if (tempFile >= 0) {
        //sceIoLseek(tempFile, 0, SCE_SEEK_SET);
        ov_callbacks ogg_callbacks;

        ogg_callbacks.read_func = ogg_callback_read;
        ogg_callbacks.seek_func = ogg_callback_seek;
        ogg_callbacks.close_func = ogg_callback_close;
        ogg_callbacks.tell_func = ogg_callback_tell;

		if (ov_open_callbacks(&tempFile, &vf, NULL, 0, ogg_callbacks) < 0){
            sceIoClose(tempFile);
            return tempInfo;
        }
        getOGGTagInfo(&vf, &tempInfo);
        ov_clear(&vf);
        if (tempFile >= 0)
            sceIoClose(tempFile);
	}

    return tempInfo;
}


float OGG_GetPercentage(){
    float perc = 0.0f;
    if (OGG_info.length){
        perc = (float)(OGG_milliSeconds/1000.0/(double)OGG_info.length*100.0);
        if (perc > 100)
            perc = 100;
    }
	return perc;
}

void OGG_End(){
    OGG_Stop();
	vitaAudioSetChannelCallback(OGG_audio_channel, 0,0);
	OGG_FreeTune();
	endAudioLib();
}

int OGG_setMute(int onOff){
    return setMute(OGG_audio_channel, onOff);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Fade out:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OGG_fadeOut(float seconds){
    fadeOut(OGG_audio_channel, seconds);
}


void OGG_setVolumeBoost(int boost){
    OGG_volume_boost = boost;
}

int OGG_getVolumeBoost(){
	return OGG_volume_boost;
}


int OGG_setPlayingSpeed(int playingSpeed){
	if (playingSpeed >= MIN_PLAYING_SPEED && playingSpeed <= MAX_PLAYING_SPEED){
		OGG_playingSpeed = playingSpeed;
		if (playingSpeed == 0)
			setVolume(OGG_audio_channel, 0x8000);
		else
			setVolume(OGG_audio_channel, FASTFORWARD_VOLUME);
        OGG_playingDelta = VITA_NUM_AUDIO_SAMPLES * (int)(OGG_playingSpeed/2);
		return 0;
	}else{
		return -1;
	}
}

int OGG_getPlayingSpeed(){
	return OGG_playingSpeed;
}

int OGG_GetStatus(){
	return 0;
}

void OGG_setVolumeBoostType(char *boostType){
    //Only old method supported
    MAX_VOLUME_BOOST = 4;
    MIN_VOLUME_BOOST = 0;
}


//Functions for filter (equalizer):
int OGG_setFilter(double tFilter[32], int copyFilter){
	return 0;
}

void OGG_enableFilter(){}

void OGG_disableFilter(){}

int OGG_isFilterSupported(){
	return 0;
}

int OGG_isFilterEnabled(){
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Manage suspend:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int OGG_suspend(){
    OGG_suspendPosition = ov_raw_tell(&OGG_VorbisFile);
    OGG_suspendIsPlaying = OGG_isPlaying;
    //OGG_Stop();
    //OGG_FreeTune();
    OGG_End();
    return 0;
}

int OGG_resume(){
    OGG_Init(OGG_audio_channel);
    if (OGG_suspendPosition >= 0){
       if (OGG_Load(OGG_fileName) == OPENING_OK){
           if (ov_raw_seek(&OGG_VorbisFile, OGG_suspendPosition))
              OGG_isPlaying = OGG_suspendIsPlaying;
       }
       OGG_suspendPosition = -1;
    }
    return 0;
}

double OGG_getFilePosition()
{
    return (double)ov_raw_tell(&OGG_VorbisFile);
}

void OGG_setFilePosition(double position)
{
    OGG_newFilePos = position;
}
