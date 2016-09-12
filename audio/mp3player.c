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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

#include "id3.h"
#include "mp3xing.h"
#include "player.h"
#include "mp3player.h"

#define FALSE 0
#define TRUE !FALSE

/* This table represents the subband-domain filter characteristics. It
* is initialized by the ParseArgs() function and is used as
* coefficients against each subband samples when DoFilter is non-nul.
*/
static mad_fixed_t Filter[32];
static double filterDouble[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* DoFilter is non-nul when the Filter table defines a filter bank to
* be applied to the decoded audio subbands.
*/
static int DoFilter = 0;

//////////////////////////////////////////////////////////////////////
// Global local variables
//////////////////////////////////////////////////////////////////////
static struct mad_stream Stream;
static struct mad_header Header;
static struct mad_frame Frame;
static struct mad_synth Synth;
static mad_timer_t Timer;
static int MP3_outputInProgress = 0;
static int MP3_channels = 0;
static int MP3_tagRead = 0;

// The following variables are maintained and updated by the tracker during playback
static int MP3_isPlaying;		// Set to true when a mod is being played
static long MP3_suspendPosition = -1;
static long MP3_suspendIsPlaying = 0;

typedef struct  {
	short left;
	short right;
} Sample;

//////////////////////////////////////////////////////////////////////
// These are the public functions
//////////////////////////////////////////////////////////////////////
static int myChannel;
static int eos;

struct fileInfo MP3_info;

#define BOOST_OLD 0
#define BOOST_NEW 1

static char MP3_fileName[264];
static int MP3_volume_boost_type = BOOST_NEW;
static double MP3_volume_boost = 0.0;
static unsigned int MP3_volume_boost_old = 0;
static double DB_forBoost = 1.0;
static int MP3_playingSpeed = 0; // 0 = normal
int MP3_defaultCPUClock = 70;
static int MP3_fd = -1;

#define INPUT_BUFFER_SIZE 2048
static unsigned char fileBuffer[INPUT_BUFFER_SIZE];
static unsigned int samplesRead;
static unsigned int MP3_filePos;
static double MP3_newFilePos = -1;
static double fileSize = 0;
static double tagsize = 0;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Applies a frequency-domain filter to audio data in the subband-domain.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void ApplyFilter(struct mad_frame *Frame){
    int Channel, Sample, Samples, SubBand;
    /* There is two application loops, each optimized for the number
     * of audio channels to process. The first alternative is for
     * two-channel frames, the second is for mono-audio.
     */
    Samples = MAD_NSBSAMPLES(&Frame->header);
    if (Frame->header.mode != MAD_MODE_SINGLE_CHANNEL)
	for (Channel = 0; Channel < 2; Channel++)
	    for (Sample = 0; Sample < Samples; Sample++)
		for (SubBand = 0; SubBand < 32; SubBand++)
			Frame->sbsample[Channel][Sample][SubBand] =
			mad_f_mul(Frame->sbsample[Channel][Sample][SubBand], Filter[SubBand]);
    else
	for (Sample = 0; Sample < Samples; Sample++)
	    for (SubBand = 0; SubBand < 32; SubBand++)
			Frame->sbsample[0][Sample][SubBand] = mad_f_mul(Frame->sbsample[0][Sample][SubBand], Filter[SubBand]);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Converts a sample from libmad's fixed point number format to a signed
// short (16 bits).
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
short convertSample(mad_fixed_t sample) {
	// round
	sample += (1L << (MAD_F_FRACBITS - 16));

	// clip
	if (sample >= MAD_F_ONE)
    	sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
	    sample = -MAD_F_ONE;

	// quantize
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Streaming functions (adapted from Ghoti's MusiceEngine.c):
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int fillFileBuffer() {
	// Find out how much to keep and how much to fill.
	const unsigned int   bytesToKeep   = Stream.bufend - Stream.next_frame;
	unsigned int         bytesToFill   = sizeof(fileBuffer) - bytesToKeep;

	// Want to keep any bytes?
	if (bytesToKeep)
		memmove(fileBuffer, fileBuffer + sizeof(fileBuffer) - bytesToKeep, bytesToKeep);

	// Read into the rest of the file buffer.
	unsigned char* bufferPos = fileBuffer + bytesToKeep;
	while (bytesToFill > 0){
		int bytesRead = sceIoRead(MP3_fd, bufferPos, bytesToFill);

		if (bytesRead == 0x80010013) {
			MP3_fd = sceIoOpen(MP3_fileName, SCE_O_RDONLY, 0777);
			if (MP3_fd >= 0) {
				sceIoLseek32(MP3_fd, MP3_filePos, SCE_SEEK_SET);
			}

			bytesRead = sceIoRead(MP3_fd, bufferPos, bytesToFill);
		}

		// EOF?
		if (bytesRead <= 0)
			return 2;

		// Adjust where we're writing to.
		bytesToFill -= bytesRead;
		bufferPos += bytesRead;
		MP3_filePos += bytesRead;
	}
	return 0;
}

void decode() {
	while ((mad_frame_decode(&Frame, &Stream) == -1) && ((Stream.error == MAD_ERROR_BUFLEN) || (Stream.error == MAD_ERROR_BUFPTR))){
		int tmp;
		tmp = fillFileBuffer();
		if (tmp==2)
            eos = 1;
		mad_stream_buffer(&Stream, fileBuffer, sizeof(fileBuffer));
	}
    //Equalizers and volume boost (NEW METHOD):
    if (DoFilter || MP3_volume_boost)
        ApplyFilter(&Frame);

    mad_timer_add(&Timer, Frame.header.duration);
	mad_synth_frame(&Synth, &Frame);
}

void convertLeftSamples(Sample* first, Sample* last, const mad_fixed_t* src) {
	Sample* dst;
	for (dst = first; dst != last; ++dst){
		dst->left = convertSample(*src++);
        //Volume Boost (OLD METHOD):
        if (MP3_volume_boost_old)
            dst->left = volume_boost(&dst->left, &MP3_volume_boost_old);
    }
}

void convertRightSamples(Sample* first, Sample* last, const mad_fixed_t* src) {
	Sample* dst;
	for (dst = first; dst != last; ++dst){
		dst->right = convertSample(*src++);
        //Volume Boost (OLD METHOD):
        if (MP3_volume_boost_old)
            dst->right = volume_boost(&dst->right, &MP3_volume_boost_old);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MP3 Callback for audio:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void MP3Callback(void *buffer, unsigned int samplesToWrite, void *pdata){
    Sample *destination = (Sample*)buffer;

    if (MP3_isPlaying == TRUE) {	//  Playing , so mix up a buffer
        MP3_outputInProgress = 1;
    	while (samplesToWrite > 0) 	{
            const unsigned int samplesAvailable = Synth.pcm.length - samplesRead;
            if (samplesAvailable > samplesToWrite) {
                convertLeftSamples(destination, destination + samplesToWrite, &Synth.pcm.samples[0][samplesRead]);
				if (MP3_channels == 2)
	                convertRightSamples(destination, destination + samplesToWrite, &Synth.pcm.samples[1][samplesRead]);
				else
	                convertRightSamples(destination, destination + samplesToWrite, &Synth.pcm.samples[0][samplesRead]);

                samplesRead += samplesToWrite;
                samplesToWrite = 0;

                if (MP3_newFilePos >= 0)
                {
                    if (!MP3_newFilePos)
                        MP3_newFilePos = ID3v2TagSize(MP3_fileName);

					int res = sceIoLseek32(MP3_fd, MP3_newFilePos, SCE_SEEK_SET);
					if (res == 0x80010013) {
						MP3_fd = sceIoOpen(MP3_fileName, SCE_O_RDONLY, 0777);
						if (MP3_fd >= 0) {
							sceIoLseek32(MP3_fd, MP3_filePos, SCE_SEEK_SET);
						}

						res = sceIoLseek32(MP3_fd, MP3_newFilePos, SCE_SEEK_SET);
					}

                    if (res != MP3_filePos){
                        MP3_filePos = MP3_newFilePos;
                        mad_timer_set(&Timer, (int)((float)MP3_info.length / 100.0 * MP3_GetPercentage()), 1, 1);
                    }
                    MP3_newFilePos = -1;
                }

		        //Check for playing speed:
                if (MP3_playingSpeed){
					int res = sceIoLseek32(MP3_fd, 2 * INPUT_BUFFER_SIZE * MP3_playingSpeed, SCE_SEEK_CUR);
					if (res == 0x80010013) {
						MP3_fd = sceIoOpen(MP3_fileName, SCE_O_RDONLY, 0777);
						if (MP3_fd >= 0) {
							sceIoLseek32(MP3_fd, MP3_filePos, SCE_SEEK_SET);
						}

						res = sceIoLseek32(MP3_fd, 2 * INPUT_BUFFER_SIZE * MP3_playingSpeed, SCE_SEEK_CUR);
					}

                    if (res != MP3_filePos){
                        MP3_filePos += 2 * INPUT_BUFFER_SIZE * MP3_playingSpeed;
                        mad_timer_set(&Timer, (int)((float)MP3_info.length / 100.0 * MP3_GetPercentage()), 1, 1);
                    }else
                        MP3_setPlayingSpeed(0);
                }
            }else{
                convertLeftSamples(destination, destination + samplesAvailable, &Synth.pcm.samples[0][samplesRead]);
				if 	(MP3_channels == 2)
	                convertRightSamples(destination, destination + samplesAvailable, &Synth.pcm.samples[1][samplesRead]);
				else
	                convertRightSamples(destination, destination + samplesAvailable, &Synth.pcm.samples[0][samplesRead]);

                samplesRead = 0;
                decode();

                destination += samplesAvailable;
                samplesToWrite -= samplesAvailable;
            }
        }
        MP3_outputInProgress = 0;
    } else {			//  Not Playing , so clear buffer
		int count;
		for (count = 0; count < samplesToWrite; count++){
			destination[count].left	= 0;
			destination[count].right = 0;
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Init:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3_Init(int channel){
    initAudioLib();
    myChannel = channel;
    MP3_isPlaying = FALSE;
	MP3_playingSpeed = 0;
	MP3_volume_boost = 0;
    MP3_volume_boost_old = 0;

    initFileInfo(&MP3_info);
    MP3_tagRead = 0;

    vitaAudioSetChannelCallback(myChannel, MP3Callback,0);

    MIN_PLAYING_SPEED=-119;
    MAX_PLAYING_SPEED=119;

    /* First the structures used by libmad must be initialized. */
    mad_stream_init(&Stream);
	mad_header_init(&Header);
    mad_frame_init(&Frame);
    mad_synth_init(&Synth);
    mad_timer_reset(&Timer);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Free tune
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3_FreeTune(){
    sceIoClose(MP3_fd);
    MP3_fd = -1;
    /* Mad is no longer used, the structures that were initialized must
     * now be cleared.
     */
    mad_synth_finish(&Synth);
    mad_header_finish(&Header);
    mad_frame_finish(&Frame);
    mad_stream_finish(&Stream);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Recupero le informazioni sul file:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void getMP3TagInfo(char *filename, struct fileInfo *targetInfo){
    //ID3:
    struct ID3Tag ID3;
    strcpy(MP3_fileName, filename);
    ParseID3(filename, &ID3);
    strcpy(targetInfo->title, ID3.ID3Title);
    strcpy(targetInfo->artist, ID3.ID3Artist);
    strcpy(targetInfo->album, ID3.ID3Album);
    strcpy(targetInfo->year, ID3.ID3Year);
    strcpy(targetInfo->genre, ID3.ID3GenreText);
    strcpy(targetInfo->trackNumber, ID3.ID3TrackText);
	targetInfo->length = ID3.ID3Length;
    targetInfo->encapsulatedPictureType = ID3.ID3EncapsulatedPictureType;
    targetInfo->encapsulatedPictureOffset = ID3.ID3EncapsulatedPictureOffset;
    targetInfo->encapsulatedPictureLength = ID3.ID3EncapsulatedPictureLength;

    MP3_info = *targetInfo;
    MP3_tagRead = 1;

}

int MP3getInfo(){
	unsigned long FrameCount = 0;
    int fd;
    int bufferSize = 1024*496;
    uint8_t *localBuffer;

    int has_xing = 0;
    struct xing xing;
	memset(&xing, 0, sizeof xing);

    long singleDataRed = 0;
	struct mad_stream stream;
	struct mad_header header;
    int timeFromID3 = 0;
    float mediumBitrate = 0.0f;

    if (!MP3_tagRead)
        getMP3TagInfo(MP3_fileName, &MP3_info);

	mad_stream_init (&stream);
	mad_header_init (&header);

    fd = sceIoOpen(MP3_fileName, SCE_O_RDONLY, 0777);
    if (fd < 0)
        return -1;

	long size = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoLseek(fd, 0, SCE_SEEK_SET);

	double startPos = ID3v2TagSize(MP3_fileName);
    //Check for xing frame:
	unsigned char *xing_buffer;
	xing_buffer = (unsigned char *)malloc(XING_BUFFER_SIZE);
	if (xing_buffer != NULL)
	{
        sceIoRead(fd, xing_buffer, XING_BUFFER_SIZE);
        if(parse_xing(xing_buffer, 0, &xing))
        {
            if (xing.flags & XING_FRAMES && xing.frames){
                has_xing = 1;
                bufferSize = 50 * 1024;
            }
        }
        free(xing_buffer);
    }

	sceIoLseek32(fd, startPos, SCE_SEEK_SET);
    startPos = SeekNextFrameMP3(fd);
    size -= startPos;

    if (size < bufferSize * 3)
        bufferSize = size;
    localBuffer = (unsigned char *) malloc(sizeof(unsigned char) * bufferSize);
    unsigned char *buff = localBuffer;

	MP3_channels = 2;
	MP3_info.fileType = MP3_TYPE;
    MP3_info.defaultCPUClock = MP3_defaultCPUClock;
    MP3_info.needsME = 0;
	MP3_info.fileSize = size;
    MP3_info.framesDecoded = 0;

    double totalBitrate = 0;
    int i = 0;

	for (i=0; i<3; i++){
        memset(localBuffer, 0, bufferSize);
        singleDataRed = sceIoRead(fd, localBuffer, bufferSize);
    	mad_stream_buffer (&stream, localBuffer, singleDataRed);

        while (1){
    		if (mad_header_decode (&header, &stream) == -1){
                if (stream.buffer == NULL || stream.error == MAD_ERROR_BUFLEN)
                    break;
    			else if (MAD_RECOVERABLE(stream.error)){
    				continue;
    			}else{
    				break;
    			}
    		}
    		//Informazioni solo dal primo frame:
    	    if (FrameCount++ == 0){
    			switch (header.layer) {
    			case MAD_LAYER_I:
    				strcpy(MP3_info.layer,"I");
    				break;
    			case MAD_LAYER_II:
    				strcpy(MP3_info.layer,"II");
    				break;
    			case MAD_LAYER_III:
    				strcpy(MP3_info.layer,"III");
    				break;
    			default:
    				strcpy(MP3_info.layer,"unknown");
    				break;
    			}

    			MP3_info.kbit = header.bitrate / 1000;
    			MP3_info.instantBitrate = header.bitrate;
    			MP3_info.hz = header.samplerate;
    			switch (header.mode) {
    			case MAD_MODE_SINGLE_CHANNEL:
    				strcpy(MP3_info.mode, "single channel");
					MP3_channels = 1;
    				break;
    			case MAD_MODE_DUAL_CHANNEL:
    				strcpy(MP3_info.mode, "dual channel");
					MP3_channels = 2;
					break;
    			case MAD_MODE_JOINT_STEREO:
    				strcpy(MP3_info.mode, "joint (MS/intensity) stereo");
					MP3_channels = 2;
    				break;
    			case MAD_MODE_STEREO:
    				strcpy(MP3_info.mode, "normal LR stereo");
					MP3_channels = 2;
    				break;
    			default:
    				strcpy(MP3_info.mode, "unknown");
					MP3_channels = 2;
    				break;
    			}

    			switch (header.emphasis) {
    			case MAD_EMPHASIS_NONE:
    				strcpy(MP3_info.emphasis,"no");
    				break;
    			case MAD_EMPHASIS_50_15_US:
    				strcpy(MP3_info.emphasis,"50/15 us");
    				break;
    			case MAD_EMPHASIS_CCITT_J_17:
    				strcpy(MP3_info.emphasis,"CCITT J.17");
    				break;
    			case MAD_EMPHASIS_RESERVED:
    				strcpy(MP3_info.emphasis,"reserved(!)");
    				break;
    			default:
    				strcpy(MP3_info.emphasis,"unknown");
    				break;
    			}

                //Check if lenght found in tag info:
                if (MP3_info.length > 0){
                    timeFromID3 = 1;
                    break;
                }
                if (has_xing)
                    break;
            }

            totalBitrate += header.bitrate;
		}
        if (size == bufferSize)
            break;
        else if (i==0)
            sceIoLseek(fd, startPos + size/3, SCE_SEEK_SET);
        else if (i==1)
            sceIoLseek(fd, startPos + 2 * size/3, SCE_SEEK_SET);

        if (timeFromID3 || has_xing)
            break;
	}
	mad_header_finish (&header);
	mad_stream_finish (&stream);
    if (buff)
    	free(buff);
    sceIoClose(fd);

    int secs = 0;
    if (has_xing)
    {
        /* modify header.duration since we don't need it anymore */
        mad_timer_multiply(&header.duration, xing.frames);
        secs = mad_timer_count(header.duration, MAD_UNITS_SECONDS);
		MP3_info.length = secs;
	}
    else if (!MP3_info.length){
		mediumBitrate = totalBitrate / (float)FrameCount;
		secs = size * 8 / mediumBitrate;
        MP3_info.length = secs;
    }else{
        secs = MP3_info.length;
    }

	//Formatto in stringa la durata totale:
	int h = secs / 3600;
	int m = (secs - h * 3600) / 60;
	int s = secs - h * 3600 - m * 60;
	snprintf(MP3_info.strLength, sizeof(MP3_info.strLength), "%2.2i:%2.2i:%2.2i", h, m, s);

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MP3_End
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3_End(){
    MP3_Stop();
    vitaAudioSetChannelCallback(myChannel, 0,0);
    MP3_FreeTune();
    endAudioLib();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Load mp3 into memory:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3_Load(char *filename){
    eos = 0;
	MP3_outputInProgress = 0;
    MP3_filePos = 0;
    fileSize = 0;
    samplesRead = 0;
    MP3_fd = sceIoOpen(filename, SCE_O_RDONLY, 0777);
    if (MP3_fd < 0)
        return ERROR_OPENING;
    fileSize = sceIoLseek32(MP3_fd, 0, SCE_SEEK_END);
	sceIoLseek32(MP3_fd, 0, SCE_SEEK_SET);
	tagsize = ID3v2TagSize(filename);
	sceIoLseek32(MP3_fd, tagsize, SCE_SEEK_SET);
    SeekNextFrameMP3(MP3_fd);

    MP3_isPlaying = FALSE;

    strcpy(MP3_fileName, filename);
    if (MP3getInfo() != 0){
        strcpy(MP3_fileName, "");
        sceIoClose(MP3_fd);
        MP3_fd = -1;
        return ERROR_OPENING;
    }

    //Controllo il sample rate:
    if (vitaAudioSetFrequency(myChannel, MP3_info.hz) < 0)
        return ERROR_INVALID_SAMPLE_RATE;
    return OPENING_OK;
}

int MP3_IsPlaying() {
	return MP3_isPlaying;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function initialises for playing, and starts
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3_Play(){
	//Azzero il timer:
    if (eos == 1){
		mad_timer_reset(&Timer);
	}

	// See if I'm already playing
    if (MP3_isPlaying)
		return FALSE;

    MP3_isPlaying = TRUE;

	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pause:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3_Pause(){
    MP3_isPlaying = !MP3_isPlaying;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3_Stop(){
    //stop playing
    MP3_isPlaying = FALSE;
    while (MP3_outputInProgress == 1)
        sceKernelDelayThread(100000);

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get time string
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3_GetTimeString(char *dest){
    mad_timer_string(Timer, dest, "%02lu:%02u:%02u", MAD_UNITS_HOURS, MAD_UNITS_MILLISECONDS, 0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get Percentage
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float MP3_GetPercentage(){
	//Calcolo posizione in %:
	float perc = 0.0f;

    if (fileSize > 0){
        perc = ((float)MP3_filePos) / ((float)fileSize - (float)tagsize) * 100.0;
        if (perc > 100)
            perc = 100;
    }
    return(perc);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Check EOS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3_EndOfStream(){
    if (eos == 1)
		return 1;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get info on file:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct fileInfo *MP3_GetInfo(){
	return &MP3_info;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get only tag info from a file:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct fileInfo MP3_GetTagInfoOnly(char *filename){
    struct fileInfo tempInfo;
    initFileInfo(&tempInfo);
    getMP3TagInfo(filename, &tempInfo);
	return tempInfo;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set volume boost type:
//NOTE: to be launched only once BEFORE setting boost volume or filter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3_setVolumeBoostType(char *boostType){
    if (strcmp(boostType, "OLD") == 0){
        MP3_volume_boost_type = BOOST_OLD;
        MAX_VOLUME_BOOST = 4;
        MIN_VOLUME_BOOST = 0;
    }else{
        MAX_VOLUME_BOOST = 15;
        MIN_VOLUME_BOOST = -MAX_VOLUME_BOOST;
        MP3_volume_boost_type = BOOST_NEW;
    }
    MP3_volume_boost_old = 0;
    MP3_volume_boost = 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set volume boost:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3_setVolumeBoost(int boost){
    if (MP3_volume_boost_type == BOOST_NEW){
        MP3_volume_boost_old = 0;
        MP3_volume_boost = boost;
    }else{
        MP3_volume_boost_old = boost;
        MP3_volume_boost = 0;
    }
    //Reapply the filter:
    MP3_setFilter(filterDouble, 0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get actual volume boost:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3_getVolumeBoost(){
    if (MP3_volume_boost_type == BOOST_NEW){
    	return(MP3_volume_boost);
    }else{
        return(MP3_volume_boost_old);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set Filter:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3_setFilter(double tFilter[32], int copyFilter){
	//Converto i db:
	double AmpFactor;
	int i;

	for (i = 0; i < 32; i++){
		//Check for volume boost:
		if (MP3_volume_boost){
			AmpFactor=pow(10.,(tFilter[i] + MP3_volume_boost * DB_forBoost)/20);
		}else{
			AmpFactor=pow(10.,tFilter[i]/20);
		}
		if(AmpFactor>mad_f_todouble(MAD_F_MAX))
		{
			DoFilter = 0;
			return(0);
		}else{
			Filter[i]=mad_f_tofixed(AmpFactor);
		}
		if (copyFilter){
			filterDouble[i] = tFilter[i];
		}
	}
	return(1);
}


int MP3_isFilterSupported(){
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Check if filter is enabled:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3_isFilterEnabled(){
	return DoFilter;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Enable filter:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3_enableFilter(){
	DoFilter = 1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Disable filter:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3_disableFilter(){
	DoFilter = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get playing speed:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3_getPlayingSpeed(){
	return MP3_playingSpeed;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set playing speed:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3_setPlayingSpeed(int playingSpeed){
	if (playingSpeed >= MIN_PLAYING_SPEED && playingSpeed <= MAX_PLAYING_SPEED){
		MP3_playingSpeed = playingSpeed;
		if (playingSpeed == 0)
            setVolume(myChannel, 0x8000);
		else
    		setVolume(myChannel, FASTFORWARD_VOLUME);
		return 0;
	}else{
		return -1;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set mute:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3_setMute(int onOff){
    return setMute(myChannel, onOff);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Fade out:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3_fadeOut(float seconds){
    fadeOut(myChannel, seconds);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Manage suspend:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3_suspend(){
    MP3_suspendPosition = MP3_filePos;
    MP3_suspendIsPlaying = MP3_isPlaying;
	/*MP3_Stop();
    MP3_FreeTune();*/

	MP3_isPlaying = FALSE;
    mad_synth_finish(&Synth);
    mad_header_finish(&Header);
    mad_frame_finish(&Frame);
    mad_stream_finish(&Stream);

    sceIoClose(MP3_fd);
	MP3_fd = -1;
    return 0;
}

int MP3_resume(){
	if (MP3_suspendPosition >= 0){
		mad_stream_init(&Stream);
		mad_header_init(&Header);
		mad_frame_init(&Frame);
		mad_synth_init(&Synth);
		mad_timer_reset(&Timer);
		MP3_fd = sceIoOpen(MP3_fileName, SCE_O_RDONLY, 0777);
		if (MP3_fd >= 0){
			MP3_filePos = MP3_suspendPosition;
			sceIoLseek32(MP3_fd, MP3_filePos, SCE_SEEK_SET);
			mad_timer_set(&Timer, (int)((float)MP3_info.length / 100.0 * MP3_GetPercentage()), 1, 1);
			MP3_isPlaying = MP3_suspendIsPlaying;
		}
	}
	MP3_suspendPosition = -1;
    return 0;
}

double MP3_getFilePosition()
{
    return MP3_filePos;
}

void MP3_setFilePosition(double position)
{
    MP3_newFilePos = position;
}
