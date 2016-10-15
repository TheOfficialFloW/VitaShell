#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "vita_audio.h"

static vitaWav vitaWavInfo[VITA_WAV_MAX_SLOTS];
static int vitaWavPlaying[VITA_WAV_MAX_SLOTS];
static int vitaWavId[VITA_WAV_MAX_SLOTS];

static short *vitaWavSamples;
static unsigned long vitaWavReq;
static int vitaWavIdFlag = 0;

static int vitaWavInitFlag = 0;

typedef struct
{
	int threadHandle;
	int handle;
	int volumeLeft;
	int volumeRight;
	vitaAudioCallback callback;
	void *data;

} vitaAudioChannelInfo;

static int vitaAudioReady = 0;
static short vitaAudioSoundBuffer[VITA_NUM_AUDIO_CHANNELS][2][VITA_NUM_AUDIO_SAMPLES][2];

static vitaAudioChannelInfo vitaAudioStatus[VITA_NUM_AUDIO_CHANNELS];

static volatile int vitaAudioTerminate = 0;

void vitaAudioSetVolume(int channel, int left, int right) {
	vitaAudioStatus[channel].volumeLeft = left;
	vitaAudioStatus[channel].volumeRight = right;
}

int vitaAudioSetFrequency(int channel, unsigned short freq) {
	return sceAudioOutSetConfig(vitaAudioStatus[channel].handle, VITA_NUM_AUDIO_SAMPLES, freq, SCE_AUDIO_OUT_MODE_STEREO);
}

void vitaAudioSetChannelCallback(int channel, vitaAudioCallback callback, void *data)
{
	volatile vitaAudioChannelInfo *pci = &vitaAudioStatus[channel];

	if (callback == 0)
		pci->callback = 0;
	else
	{
		pci->callback = callback;
	}
}

static int vitaAudioOutBlocking(unsigned int channel, unsigned int left, unsigned int right, void *data)
{
	if (!vitaAudioReady)
		return(-1);

	if (channel >= VITA_NUM_AUDIO_CHANNELS)
		return(-1);

	if (left > VITA_VOLUME_MAX)
		left = VITA_VOLUME_MAX;

	if (right > VITA_VOLUME_MAX)
		right = VITA_VOLUME_MAX;

	int vols2[2] = { left, right };
	sceAudioOutSetVolume(vitaAudioStatus[channel].handle, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vols2);
	return sceAudioOutOutput(vitaAudioStatus[channel].handle, data);
}

static int vitaAudioChannelThread(int args, void *argp)
{
	volatile int bufidx = 0;

	int channel = *(int *) argp;

	while (vitaAudioTerminate == 0)
	{
		void *bufptr = &vitaAudioSoundBuffer[channel][bufidx];
		vitaAudioCallback callback;
		callback = vitaAudioStatus[channel].callback;

		if (callback)
		{
			callback(bufptr, VITA_NUM_AUDIO_SAMPLES, vitaAudioStatus[channel].data);
		} else {
			unsigned int *ptr=bufptr;
			int i;
			for (i=0; i<VITA_NUM_AUDIO_SAMPLES; ++i) *(ptr++)=0;
		}

		vitaAudioOutBlocking(channel, vitaAudioStatus[channel].volumeLeft, vitaAudioStatus[channel].volumeRight, bufptr);

		bufidx = (bufidx ? 0:1);
	}

	sceKernelExitThread(0);

	return(0);
}

int vitaAudioInit(int priority)
{
	int i, ret;
	int failed = 0;
	char str[32];

	vitaAudioTerminate = 0;
	vitaAudioReady = 0;

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++)
	{
		vitaAudioStatus[i].handle = -1;
		vitaAudioStatus[i].threadHandle = -1;
		vitaAudioStatus[i].volumeRight = VITA_VOLUME_MAX;
		vitaAudioStatus[i].volumeLeft  = VITA_VOLUME_MAX;
		vitaAudioStatus[i].callback = 0;
		vitaAudioStatus[i].data = 0;
	}

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++)
	{
		if ((vitaAudioStatus[i].handle = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, VITA_NUM_AUDIO_SAMPLES, 44100, SCE_AUDIO_OUT_MODE_STEREO)) < 0)
			failed = 1;
	}

	if (failed)
	{
		for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++)
		{
			if (vitaAudioStatus[i].handle != -1)
				sceAudioOutReleasePort(vitaAudioStatus[i].handle);

			vitaAudioStatus[i].handle = -1;
		}

		return 0;
	}

	vitaAudioReady = 1;

	strcpy(str, "PgeAudioThread0");

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++)
	{
		str[14] = '0' + i;
		vitaAudioStatus[i].threadHandle = sceKernelCreateThread(str, (void*)&vitaAudioChannelThread, priority, 0x10000, 0, 0, NULL);

		if (vitaAudioStatus[i].threadHandle < 0)
		{
			vitaAudioStatus[i].threadHandle = -1;
			failed = 1;
			break;
		}

		ret = sceKernelStartThread(vitaAudioStatus[i].threadHandle, sizeof(i), &i);

		if (ret != 0)
		{
			failed = 1;
			break;
		}
	}

	if (failed)
	{
		vitaAudioTerminate = 1;

		for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++)
		{
			if (vitaAudioStatus[i].threadHandle != -1)
			{
				sceKernelDeleteThread(vitaAudioStatus[i].threadHandle);
			}

			vitaAudioStatus[i].threadHandle = -1;
		}

		vitaAudioReady = 0;

		return 0;
	}

	return 1;
}

void vitaAudioShutdown(void)
{
	int i;
	vitaAudioReady = 0;
	vitaAudioTerminate = 1;

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++)
	{
		if (vitaAudioStatus[i].threadHandle != -1)
		{
			sceKernelDeleteThread(vitaAudioStatus[i].threadHandle);
		}

		vitaAudioStatus[i].threadHandle = -1;
	}

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++)
	{
		if (vitaAudioStatus[i].handle != -1)
		{
			sceAudioOutReleasePort(vitaAudioStatus[i].handle);
			vitaAudioStatus[i].handle = -1;
		}
	}
}

static void wavout_snd_callback(void *_buf, unsigned int _reqn, void *pdata)
{
	int i,slot;
	vitaWav *wi;
	unsigned long ptr, frac;
	short *buf = _buf;

	vitaWavSamples = _buf;
	vitaWavReq = _reqn;

	for(i = 0; i < _reqn; i++)
	{
		int outr = 0, outl = 0;

		for(slot = 0; slot < VITA_WAV_MAX_SLOTS; slot++)
		{
			if(!vitaWavPlaying[slot]) continue;

			wi = &vitaWavInfo[slot];
			frac = wi->playPtr_frac + wi->rateRatio;
			wi->playPtr = ptr = wi->playPtr + (frac>>16);
			wi->playPtr_frac = (frac & 0xffff);

			if(ptr >= wi->sampleCount)
			{
				if(wi->loop)
				{
					wi->playPtr = 0;
					wi->playPtr_frac = 0;
					ptr = 0;
				}
				else
				{
					vitaWavPlaying[slot] = 0;
					break;
				}
			}

			short *src16 = (short *)wi->data;
			unsigned char *src8 = (unsigned char *)wi->data;

			if(wi->channels == 1)
			{
				if(wi->bitPerSample == 8)
				{
					outl += (src8[ptr] * 256) - 32768;
					outr += (src8[ptr] * 256) - 32768;
				}
				else
				{
					outl += src16[ptr];
					outr += src16[ptr];
				}
			}
			else
			{
				if(wi->bitPerSample == 8)
				{
					outl += (src8[ptr*2] * 256) - 32768;
					outr += (src8[ptr*2+1] * 256) - 32768;
				}
				else
				{
					outl += src16[ptr*2];
					outr += src16[ptr*2+1];
				}
			}
		}

		if(outl < -32768)
			outl = -32768;
		else if (outl > 32767)
			outl = 32767;

		if(outr < -32768)
			outr = -32768;
		else if (outr > 32767)
			outr = 32767;

		*(buf++) = outl;
		*(buf++) = outr;
	}
}

int vitaWavInit(void)
{
	int i;

	vitaAudioInit(0x40);

	vitaAudioSetChannelCallback(0, wavout_snd_callback, 0);

	for(i = 0; i < VITA_WAV_MAX_SLOTS; i++)
		vitaWavPlaying[i] = 0;

	vitaWavInitFlag = 1;

	return(1);
}

void vitaWavShutdown(void)
{
	if(vitaWavInitFlag)
		vitaAudioShutdown();
}

void vitaWavStop(vitaWav *wav)
{
	int i;

	for(i = 0; i < VITA_WAV_MAX_SLOTS; i++)
	{
		if(wav->id == vitaWavId[i])
			vitaWavPlaying[i] = 0;
	}
}

void vitaWavStopAll(void)
{
	int i;

	for(i = 0; i < VITA_WAV_MAX_SLOTS; i++)
		vitaWavPlaying[i] = 0;
}

void vitaWavLoop(vitaWav *wav, unsigned int loop)
{
	wav->loop = loop;
}

int vitaWavPlay(vitaWav *wav)
{
	if(!vitaWavInitFlag)
		return(0);

	int i;

	vitaWav *wid;

	for(i = 0;i < VITA_WAV_MAX_SLOTS;i++)
	{
		if(vitaWavPlaying[i] == 0)
			break;
	}

	if(i == VITA_WAV_MAX_SLOTS)
		return(0);

	wid = &vitaWavInfo[i];
	wid->channels = wav->channels;
	wid->sampleRate = wav->sampleRate;
	wid->sampleCount = wav->sampleCount;
	wid->dataLength = wav->dataLength;
	wid->data = wav->data;
	wid->rateRatio = wav->rateRatio;
	wid->playPtr = 0;
	wid->playPtr_frac = 0;
	wid->loop = wav->loop;
	wid->id = wav->id;
	vitaWavPlaying[i] = 1;
	vitaWavId[i] = wav->id;
	wid->bitPerSample = wav->bitPerSample;

	return(1);
}

static vitaWav *vitaWavLoadInternal(vitaWav *wav, unsigned char *wavfile, int size)
{
	unsigned long channels;
	unsigned long samplerate;
	unsigned long blocksize;
	unsigned long bitpersample;
	unsigned long datalength;
	unsigned long samplecount;

	if(memcmp(wavfile, "RIFF", 4) != 0)
	{
		free(wav);
		return NULL;
	}

	channels = *(short *)(wavfile+0x16);
	samplerate = *(long *)(wavfile+0x18);
	blocksize = *(short *)(wavfile+0x20);
	bitpersample = *(short *)(wavfile+0x22);

	int i;

	for(i = 0; memcmp(wavfile + 0x24 + i, "data", 4) != 0; i++)
	{
		if(i == 0xFF)
		{
			free(wav);
			return NULL;
		}
	}

	datalength = *(unsigned long *)(wavfile + 0x28 + i);

	if(datalength + 0x2c > size)
	{
		free(wav);
		return NULL;
	}

	if(channels != 2 && channels != 1)
	{
		free(wav);
		return NULL;
	}

	if(samplerate > 100000 || samplerate < 2000)
	{
		free(wav);
		return NULL;
	}

	if(channels == 2)
	{
		samplecount = datalength/(bitpersample>>2);
	}
	else
	{
		samplecount = datalength/((bitpersample>>2)>>1);
	}

	if(samplecount <= 0)
	{
		free(wav);
		return NULL;
	}

	wav->channels = channels;
	wav->sampleRate = samplerate;
	wav->sampleCount = samplecount;
	wav->dataLength = datalength;
	wav->data = wavfile + 0x2c;
	wav->rateRatio = (samplerate*0x4000)/11025;
	wav->playPtr = 0;
	wav->playPtr_frac= 0;
	wav->loop = 0;
	vitaWavIdFlag++;
	wav->id = vitaWavIdFlag;
	wav->bitPerSample = bitpersample;

	return wav;
}

vitaWav *vitaWavLoad(const char *filename)
{
	unsigned long filelen;

	unsigned char *wavfile;
	vitaWav *wav;

	int fd = sceIoOpen(filename, SCE_O_RDONLY, 0777);

	if(fd < 0)
		return NULL;

	long lSize;

	lSize = sceIoLseek32(fd, 0, SCE_SEEK_END);
	sceIoLseek32(fd, 0, SCE_SEEK_SET);

	wav = malloc(lSize + sizeof(vitaWav));
	wavfile = (unsigned char*)(wav) + sizeof(vitaWav);

	filelen = sceIoRead(fd, wavfile, lSize);

	sceIoClose(fd);

	return(vitaWavLoadInternal(wav, wavfile, filelen));
}

vitaWav *vitaWavLoadMemory(const unsigned char *buffer, int size)
{
	unsigned char *wavfile;
	vitaWav *wav;

	wav = malloc(size + sizeof(wav));
	wavfile = (unsigned char*)(wav) + sizeof(wav);

	memcpy(wavfile, (unsigned char*)buffer, size);

	return(vitaWavLoadInternal(wav, wavfile, size));
}

void vitaWavUnload(vitaWav *wav)
{
	if(wav != NULL)
		free(wav);
}
