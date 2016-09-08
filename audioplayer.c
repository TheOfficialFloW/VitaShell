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
#include "theme.h"
#include "utils.h"
#include "audioplayer.h"

#define lerp(value, from_max, to_max) ((((value*10) * (to_max*10))/(from_max*10))/10)

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

		int sync = (pInputStream->pBuffer[0] & 0xFF) << 4 | (pInputStream->pBuffer[1] & 0xE0) >> 4;
		if (sync != 0xFFE) {
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

int audioPlayer(char *file, FileList *list, FileListEntry *entry, int *base_pos, int *rel_pos, unsigned int codec_type, int showInterface) {
	int returnCode = 0;
	bool isTouched = false;

	SceTouchData touch;

	if(isRunning()) {
		stopAudioThread();
		returnCode = shutdownAudioPlayer();
	}

	resetAudioPlayer();

	setCodecType(codec_type);
	setInputFileName(file);

	returnCode = startAudioPlayer();
	if(showInterface == 1) {
		// Main loop
		while (isRunning()) {
			readPad();
			sceTouchPeek(0, &touch, 1);

			int checkPrevNexFlag = -1;

			if (touch.reportNum > 0 && !isTouched) {
				int x = lerp(touch.report[0].x, 1920, SCREEN_WIDTH);
				int y = lerp(touch.report[0].y, 1088, SCREEN_HEIGHT);
				if(x >= SCREEN_WIDTH / 2 - 200 && x <= SCREEN_WIDTH / 2 - 80 && y >= SCREEN_HEIGHT - 170 && y <= SCREEN_HEIGHT - 50) {
					checkPrevNexFlag = 1;
				} else if(x >= SCREEN_WIDTH / 2 - 60 && x <= SCREEN_WIDTH / 2 + 60 && y >= SCREEN_HEIGHT - 170 && y <= SCREEN_HEIGHT - 50) {
					checkPrevNexFlag = 0;
				} else if(x >= SCREEN_WIDTH / 2 + 80 && x <= SCREEN_WIDTH / 2 + 200 && y >= SCREEN_HEIGHT - 170 && y <= SCREEN_HEIGHT - 50) {
					togglePause();
				}
				isTouched = true;
			} else if (touch.reportNum <= 0) {
				isTouched = false;
			}

			// Pause or resume if START button pressed
			if (pressed_buttons & SCE_CTRL_START) {
				togglePause();
			}

			// Exit if SELECT button pressed
			if (pressed_buttons & SCE_CTRL_CANCEL) {
				stopAudioThread();
			}

			// Next if R Trigger pressed
			if (pressed_buttons & SCE_CTRL_RTRIGGER) {
				checkPrevNexFlag = 0;
			}

			// Previous if L Trigger pressed
			if (pressed_buttons & SCE_CTRL_LTRIGGER) {
				checkPrevNexFlag = 1;
			}

			if(checkPrevNexFlag >= 0) {
				int available = 0;

				int old_base_pos = *base_pos;
				int old_rel_pos = *rel_pos;
				FileListEntry *old_entry = entry;
				while (checkPrevNexFlag ? entry->previous : entry->next) {
					entry = checkPrevNexFlag ? entry->previous : entry->next;
					if (checkPrevNexFlag) {
						if (*rel_pos > 0) {
							(*rel_pos)--;
						} else {
							if (*base_pos > 0) {
								(*base_pos)--;
							}
						}
					} else {
						if ((*rel_pos + 1) < list->length) {
							if ((*rel_pos + 1) < MAX_POSITION) {
								(*rel_pos)++;
							} else {
								if ((*base_pos + *rel_pos + 1) < list->length) {
									(*base_pos)++;
								}
							}
						}
					}

					if (!entry->is_folder) {
						char path[MAX_PATH_LENGTH];
						snprintf(path, MAX_PATH_LENGTH, "%s%s", list->path, entry->name);
						int type = getFileType(path);
						if (type == FILE_TYPE_MP3) {
							stopAudioThread();
							int returnCode = shutdownAudioPlayer();
							resetAudioPlayer();
							setCodecType(SCE_AUDIODEC_TYPE_MP3);
							setInputFileName(path);
							startAudioPlayer();
							available = 1;
							break;
						}
					}
				}

				if (!available) {
					*base_pos = old_base_pos;
					*rel_pos = old_rel_pos;
					entry = old_entry;
				}
			}

			// Start drawing
			startDrawing(NULL);
			int nameLength = strlen(entry->name);
			vita2d_pgf_draw_text(font, SCREEN_WIDTH / 2 - nameLength * 5.5, SCREEN_HEIGHT - 190, 0xFFFFFFFF, 1.0f, entry->name);
			
			vita2d_draw_texture(headphone_image, SCREEN_WIDTH / 2 - 150, 20);
			vita2d_draw_texture(audio_previous_image, SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 170);
			vita2d_draw_texture(audio_next_image, SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT - 170);
			if(isPausing()) {
				vita2d_draw_texture(audio_play_image, SCREEN_WIDTH / 2 + 80, SCREEN_HEIGHT - 170);
			} else {
				vita2d_draw_texture(audio_pause_image, SCREEN_WIDTH / 2 + 80, SCREEN_HEIGHT - 170);
			}

			// End drawing
			endDrawing();

			sceKernelDelayThread(1000);
		}

		returnCode = shutdownAudioPlayer();
		resetAudioPlayer();
	}
	return returnCode;
}

