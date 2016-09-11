/*
 * vitaWav.h: Header for WAV audio playback
 * This file is part of the "Phoenix Game Engine".
 *
 * Copyright (C) 2007 Phoenix Game Engine
 * Copyright (C) 2007 InsertWittyName <tias_dp@hotmail.com>
 *
 */

#ifndef __VITA_AUDIO_H__
#define __VITA_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup vitaWav WAV Library
 *  @{
 */

/**
 * A WAV file struct
 */
typedef struct
{
	unsigned long channels;		/**<  Number of channels */
	unsigned long sampleRate;	/**<  Sample rate */
	unsigned long sampleCount;	/**<  Sample count */
	unsigned long dataLength;	/**<  Data length */
	unsigned long rateRatio;	/**<  Rate ratio (sampleRate / 44100 * 0x10000) */
	unsigned long playPtr;		/**<  Internal */
	unsigned long playPtr_frac;	/**<  Internal */
	unsigned int loop;			/**<  Loop flag */
	unsigned char *data;		/**< A pointer to the actual WAV data */
	unsigned long id;			/**<  The ID of the WAV */
	unsigned int bitPerSample;	/**<  The bit rate of the WAV */
} vitaWav;

/**
 * Initialise the WAV playback
 *
 * @returns 1 on success.
 */
int vitaWavInit(void);

/**
 * Shutdown WAV playback
 */
void vitaWavShutdown(void);

/**
 * Load a WAV file
 *
 * @param filename - Path of the file to load.
 *
 * @returns A pointer to a ::vitaWav struct or NULL on error.
 */
vitaWav *vitaWavLoad(const char *filename);

/**
 * Load a WAV file from memory
 *
 * @param buffer - Buffer that contains the WAV data.
 *
 * @param size - Size of the buffer.
 *
 * @returns A pointer to a ::vitaWav struct or NULL on error.
 */
vitaWav *vitaWavLoadMemory(const unsigned char *buffer, int size);

/**
 * Unload a previously loaded WAV file
 *
 * @param wav - A valid ::vitaWav
 */
void vitaWavUnload(vitaWav *wav);

/**
 * Start playing a loaded WAV file
 *
 * @param wav A pointer to a valid ::vitaWav struct.
 *
 * @returns 1 on success
 */
int vitaWavPlay(vitaWav *wav);

/**
 * Stop playing a loaded WAV
 *
 * @param wav A pointer to a valid ::vitaWav struct.
 *
 * @returns 1 on success
 */
void vitaWavStop(vitaWav *wav);

/**
 * Stop playing all WAVs
 */
void vitaWavStopAll(void);

/**
 * Set the loop of the WAV playback
 *
 * @param wav - A pointer to a valid ::vitaWav struct.
 *
 * @param loop - Set to 1 to loop, 0 to playback once.
 */
void vitaWavLoop(vitaWav *wav, unsigned int loop);

/** @} */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __PGEWAV_H__