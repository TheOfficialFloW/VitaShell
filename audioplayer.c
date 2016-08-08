/*
	VitaShell
	Copyright (C) 2015-2016

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "main.h"
#include "archive.h"
#include "photo.h"
#include "file.h"
#include "utils.h"
#include "audioplayer.h"

static SceUID sEventAudioHolder = -1;
static SceUID sThreadAudioHolder = -1;
static AudioOutConfig sAudioOutConfigHolder;
static bool sIsPausing = false;
static bool sIsRunning = false;

static unsigned int sCodecType;
static char *sInputFileName;

void setCodecType(unsigned int codecType) {
	sCodecType = codecType;
}

void setInputFileName(char *file) {
	sInputFileName = file;
}

void togglePause() {
	sIsPausing = !sIsPausing;
}

bool isPausing() {
	return sIsPausing;
}

void stopAudioThread() {
	sIsRunning = false;
}

bool isRunning() {
	return sIsRunning;
}

void resetAudioPlayer() {
	sIsRunning = true;
	sIsPausing = false;
}

int initAudioDecoderByType(SceAudiodecCtrl *pAudiodecCtrl, AudioFileStream *pInputStream, AudioOutConfig *pAudioOutConfig, int codecType)
{
	int returnValue = 0;
	pInputStream->offsetRead = 0;

	pAudiodecCtrl->wordLength = SCE_AUDIODEC_WORD_LENGTH_16BITS;
	pAudiodecCtrl->size = sizeof(SceAudiodecCtrl);
	if (codecType == SCE_AUDIODEC_TYPE_MP3) {
		int sampleRateMpeg[4][3] = {
			{11025, 12000, 8000}, {0, 0, 0}, {22050, 24000, 16000}, {44100, 48000, 32000}
		};
		int audioChannelsMpeg[4] = {2, 2, 2, 1};

		int sync = (pInputStream->pBuffer[0] & 0xFF) << 3 | (pInputStream->pBuffer[1] & 0xE0) >> 3;
		if (sync != 0x7FF) {
			return -1;
		}

		int version = (pInputStream->pBuffer[1] & 0x18) >> 3;
		pAudiodecCtrl->pInfo->mp3.version = version;
		int chMode = audioChannelsMpeg[(pInputStream->pBuffer[3] & 0xC0) >> 6];
		pAudiodecCtrl->pInfo->mp3.ch = chMode;
		pAudiodecCtrl->pInfo->size = sizeof(pAudiodecCtrl->pInfo->mp3);

		returnValue = sceAudiodecCreateDecoder(pAudiodecCtrl, SCE_AUDIODEC_TYPE_MP3);
		if (returnValue >= 0 && pAudioOutConfig != NULL) {
			int indexBitRate = sampleRateMpeg[(pInputStream->pBuffer[1] & 0x18) >> 3][(pInputStream->pBuffer[2] & 0x06) >> 2];
			pAudioOutConfig->samplingRate = indexBitRate;
			pAudioOutConfig->ch = pAudiodecCtrl->pInfo->mp3.ch;
			pAudioOutConfig->param = SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO;
			pAudioOutConfig->grain = pAudiodecCtrl->maxPcmSize / pAudiodecCtrl->pInfo->mp3.ch / sizeof(int16_t);
		}
	} else if (codecType == SCE_AUDIODEC_TYPE_AT9) {
		//TODO
	} else if (codecType == SCE_AUDIODEC_TYPE_AAC) {
		//TODO
	} else {
		return -1;
	}
	return returnValue;
}

int decodeCurrentAudio(SceAudiodecCtrl *pAudiodecCtrl, AudioFileStream *pInputStream, AudioFileStream *pOutputBuffer)
{
	int returnValue = 0;

	pAudiodecCtrl->pPcm = pOutputBuffer->pBuffer + pOutputBuffer->offsetWrite;
	pAudiodecCtrl->pEs = pInputStream->pBuffer + pInputStream->offsetRead;

	returnValue = sceAudiodecDecode(pAudiodecCtrl);
	if (returnValue >= 0) {
		pInputStream->offsetRead += pAudiodecCtrl->inputEsSize;
		pOutputBuffer->offsetWrite = (pOutputBuffer->offsetWrite + pOutputBuffer->sizeBuffer / 2) % pOutputBuffer->sizeBuffer;
	}

	return returnValue;
}

int initAudioOut(AudioOutConfig *pAudioOutConfig) {
	int returnValue = 0;
	if (pAudioOutConfig->portId < 0) {
		returnValue = sceAudioOutOpenPort(pAudioOutConfig->portType, pAudioOutConfig->grain, pAudioOutConfig->samplingRate, pAudioOutConfig->param);
		if (returnValue >= 0) {
			pAudioOutConfig->portId = returnValue;
			returnValue = sceAudioOutSetVolume(pAudioOutConfig->portId, (SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), pAudioOutConfig->volume);
		}
	}
	return returnValue;
}

int outputCurrentAudio(AudioOutConfig *pAudioOutConfig, AudioFileStream *pOutputStream) {
	int returnValue = 0;
	if (pOutputStream != NULL) {
		returnValue = sceAudioOutOutput(pAudioOutConfig->portId, pOutputStream->pBuffer + pOutputStream->offsetRead);
		if (returnValue >= 0) {
			pOutputStream->offsetRead = (pOutputStream->offsetRead + pOutputStream->sizeBuffer / 2) % pOutputStream->sizeBuffer;
		}
	} else {
		returnValue = sceAudioOutOutput(pAudioOutConfig->portId, NULL);
	}
	return returnValue;
}

int audioOutThread(int argSize, void *pArgBlock) {
	int returnValue = 0;
	int pcmSize = 0;

	AudioOutConfig *pAudioOutConfig;
	pAudioOutConfig = &sAudioOutConfigHolder;

	AudioFileStream input;
	input.pNameFile = sInputFileName;
	input.sizeFile = 0;
	input.pBuffer = NULL;
	input.offsetRead = 0;
	input.offsetWrite = 0;

	AudioFileStream output;
	output.sizeFile = 0;
	output.pBuffer = NULL;
	output.offsetRead = 0;
	output.offsetWrite = 0;

	SceAudiodecInfo audiodecInfo;
	SceAudiodecCtrl audiodecCtrl;
	memset(&audiodecInfo, 0, sizeof(SceAudiodecInfo));
	memset(&audiodecCtrl, 0, sizeof(SceAudiodecCtrl));
	audiodecCtrl.pInfo = &audiodecInfo;

	returnValue = getFileSize(input.pNameFile);
	if (returnValue > 0) {
		input.sizeFile = returnValue;
		int maxEsSize = 0;
		if (sCodecType == SCE_AUDIODEC_TYPE_MP3) {
			maxEsSize = SCE_AUDIODEC_MP3_MAX_ES_SIZE;
		} else if (sCodecType == SCE_AUDIODEC_TYPE_AT9) {
			maxEsSize = SCE_AUDIODEC_AT9_MAX_ES_SIZE;
		} else if (sCodecType == SCE_AUDIODEC_TYPE_AAC) {
			maxEsSize = SCE_AUDIODEC_AAC_MAX_ES_SIZE;
		}
		if(maxEsSize > 0) {
			input.sizeBuffer = SCE_AUDIODEC_ROUND_UP(input.sizeFile + maxEsSize);
			input.pBuffer = memalign(SCE_AUDIODEC_ALIGNMENT_SIZE, input.sizeBuffer);
			returnValue = ReadFile(input.pNameFile, input.pBuffer, input.sizeFile);
			if (returnValue >= 0) {
				input.offsetWrite = input.sizeFile;
				returnValue = initAudioDecoderByType(&audiodecCtrl, &input, pAudioOutConfig, sCodecType);
				if (returnValue >= 0) {
					pcmSize = sizeof(int16_t) * pAudioOutConfig->ch * pAudioOutConfig->grain;
					output.sizeBuffer = SCE_AUDIODEC_ROUND_UP(pcmSize) * 2;
					output.pBuffer = memalign(SCE_AUDIODEC_ALIGNMENT_SIZE, output.sizeBuffer);
					returnValue = initAudioOut(pAudioOutConfig);
					if (returnValue >= 0) {
						sceKernelSetEventFlag(sEventAudioHolder, 0x00000001U);
						//Playing Track :)
						while (isRunning()) {
							if(isPausing()) {
								sceKernelDelayThread(1000);
							} else {
								if (input.sizeFile <= input.offsetRead) {
									break;
								}

								returnValue = decodeCurrentAudio(&audiodecCtrl, &input, &output);
								if (returnValue < 0) {
									break;
								}

								returnValue = outputCurrentAudio(pAudioOutConfig, &output);
								if (returnValue < 0) {
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	outputCurrentAudio(pAudioOutConfig, NULL);

	if (pAudioOutConfig->portId >= 0) {
		returnValue = sceAudioOutReleasePort(pAudioOutConfig->portId);
		pAudioOutConfig->portId = -1;
	}

	returnValue = sceAudiodecDeleteDecoder(&audiodecCtrl);
	if (output.pBuffer != NULL) {
		free(output.pBuffer);
		output.pBuffer = NULL;
	}

	if (input.pBuffer != NULL) {
		free(input.pBuffer);
		input.pBuffer = NULL;
	}

	return returnValue;
}

int shutdownAudioPlayer() {
	int returnValue = 0;

	if (sThreadAudioHolder >= 0) {
		returnValue = sceKernelWaitThreadEnd(sThreadAudioHolder, NULL, NULL);
		returnValue = sceKernelDeleteThread(sThreadAudioHolder);
		sThreadAudioHolder = -1;
	}

	if (sEventAudioHolder >= 0) {
		returnValue = sceKernelDeleteEventFlag(sEventAudioHolder);
		sEventAudioHolder = -1;
	}

	returnValue = sceAudiodecTermLibrary(sCodecType);
	return returnValue;
}

int startAudioPlayer()
{
	int returnValue = 0;

	AudioOutConfig *pAudioOutConfig = &sAudioOutConfigHolder;

	pAudioOutConfig->portId = -1;
	pAudioOutConfig->portType = SCE_AUDIO_OUT_PORT_TYPE_MAIN;
	pAudioOutConfig->samplingRate = 0;
	pAudioOutConfig->volume[0] = pAudioOutConfig->volume[1] = SCE_AUDIO_VOLUME_0DB / 4;
	pAudioOutConfig->ch = 0;
	pAudioOutConfig->param = 0;
	pAudioOutConfig->grain = 0;

	SceAudiodecInitParam audiodecInitParam;
	memset(&audiodecInitParam, 0, sizeof(audiodecInitParam));
	
	if(sCodecType == SCE_AUDIODEC_TYPE_AT9) {
		//TODO
	} else if (sCodecType == SCE_AUDIODEC_TYPE_MP3) {
		audiodecInitParam.size = sizeof(audiodecInitParam.mp3);
		audiodecInitParam.mp3.totalStreams = 1;
	} else if (sCodecType == SCE_AUDIODEC_TYPE_AAC) {
		//TODO
	} else {
		returnValue = -1;
	}

	if (returnValue >= 0) {
		returnValue = sceAudiodecInitLibrary(sCodecType, &audiodecInitParam);
		if (returnValue >= 0) {
			returnValue = sceKernelCreateEventFlag("AudioOutThread", 0x00001000, 0, NULL);
			if (returnValue >= 0) {
				sEventAudioHolder = returnValue;
				returnValue = sceKernelCreateThread("AudioOutThread", (void*)&audioOutThread, 64, 0x1000U, 0, 0, NULL);
				if (returnValue >= 0) {
					sThreadAudioHolder = returnValue;
					returnValue = sceKernelStartThread(sThreadAudioHolder, 0, NULL);
					if (returnValue >= 0) {
						sceKernelWaitEventFlag(sEventAudioHolder, 0x00000001U, 0, NULL, NULL);
					}
				}
			}
		}
	}

	if (returnValue < 0) {
		shutdownAudioPlayer();
	}

	return returnValue;
}

//TODO
void playPrevious() {
	/*
	stopThread();
	returnCode = shutdownAudioPlayer();
	reInit();
	setCodecType();
	setInputFileName(file);
	setOutputFileName();
	startAudioPlayer();
	*/
}

//TODO
void playNext() {
	/*
	stopThread();
	returnCode = shutdownAudioPlayer();
	reInit();
	setCodecType();
	setInputFileName(file);
	setOutputFileName();
	startAudioPlayer();
	*/
}

int audioPlayer(char *file, int type, FileList *list, FileListEntry *entry, int *base_pos, int *rel_pos, unsigned int codec_type, int blooking) {
	int returnCode = 0;

	if(isRunning()) {
		stopAudioThread();
		returnCode = shutdownAudioPlayer();
	}

	resetAudioPlayer();

	setCodecType(codec_type);
	setInputFileName(file);

	returnCode = startAudioPlayer();
	if(blooking == 1) {
		// Main loop
		while (isRunning()) {
			readPad();

			// Pause or resume if START button pressed
			if (pressed_buttons & SCE_CTRL_START) {
				togglePause();
			}

			// Exit if SELECT button pressed
			if (pressed_buttons & SCE_CTRL_CANCEL) {
				stopAudioThread();
			}

			// Pause or resume if START button pressed
			if (pressed_buttons & SCE_CTRL_RTRIGGER) {
				playNext();
			}

			// Exit if SELECT button pressed
			if (pressed_buttons & SCE_CTRL_LTRIGGER) {
				playPrevious();
			}

			sceKernelDelayThread(1000);
		}

		returnCode = shutdownAudioPlayer();
		resetAudioPlayer();
	}
	return returnCode;
}

