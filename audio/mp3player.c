// mp3player.c: MP3 Player Implementation in C for Sony PSP
//
////////////////////////////////////////////////////////////////////////////

#include <psp2/io/fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "mp3player.h"
#include "vita_audio.h"

#define FALSE 0
#define TRUE !FALSE
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#define MadErrorString(x) mad_stream_errorstr(x)
#define INPUT_BUFFER_SIZE	(5*8192)
#define OUTPUT_BUFFER_SIZE	2048	/* Must be an integer multiple of 4. */

int isPlaying;		// Set to true when a mod is being played

/* This table represents the subband-domain filter characteristics. It
* is initialized by the ParseArgs() function and is used as
* coefficients against each subband samples when DoFilter is non-nul.
*/
mad_fixed_t Filter[32];

/* DoFilter is non-nul when the Filter table defines a filter bank to
* be applied to the decoded audio subbands.
*/
int DoFilter = 0;

#define NUMCHANNELS 2
uint8_t *ptr;
long size;
unsigned int samplesInOutput = 0;

//////////////////////////////////////////////////////////////////////
// Global local variables
//////////////////////////////////////////////////////////////////////

//libmad lowlevel stuff

// The following variables contain the music data, ie they don't change value until you load a new file
struct mad_stream Stream;
struct mad_frame Frame;
struct mad_synth Synth;
mad_timer_t Timer;
signed short OutputBuffer[OUTPUT_BUFFER_SIZE];
unsigned char InputBuffer[INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD],
    *OutputPtr = (unsigned char *) OutputBuffer, *GuardPtr = NULL;
const unsigned char *OutputBufferEnd = (unsigned char *) OutputBuffer + OUTPUT_BUFFER_SIZE * 2;
int Status = 0, i;
unsigned long FrameCount = 0;

//////////////////////////////////////////////////////////////////////
// These are the public functions
//////////////////////////////////////////////////////////////////////
static int myChannel;
static int eos;

/*void MP3setStubs(codecStubs * stubs)
{
    stubs->init = MP3_Init;
    stubs->load = MP3_Load;
    stubs->play = MP3_Play;
    stubs->pause = MP3_Pause;
    stubs->stop = MP3_Stop;
    stubs->end = MP3_End;
    stubs->time = MP3_GetTimeString;
    stubs->tick = NULL;
    stubs->eos = MP3_EndOfStream;
    memcpy(stubs->extension, "mp3\0" "\0\0\0\0", 2 * 4);
}*/

static int PrintFrameInfo(struct mad_header *Header)
{
    const char *Layer, *Mode, *Emphasis;

    /* Convert the layer number to it's printed representation. */
    switch (Header->layer) {
    case MAD_LAYER_I:
	Layer = "I";
	break;
    case MAD_LAYER_II:
	Layer = "II";
	break;
    case MAD_LAYER_III:
	Layer = "III";
	break;
    default:
	Layer = "(unexpected layer value)";
	break;
    }

    /* Convert the audio mode to it's printed representation. */
    switch (Header->mode) {
    case MAD_MODE_SINGLE_CHANNEL:
	Mode = "single channel";
	break;
    case MAD_MODE_DUAL_CHANNEL:
	Mode = "dual channel";
	break;
    case MAD_MODE_JOINT_STEREO:
	Mode = "joint (MS/intensity) stereo";
	break;
    case MAD_MODE_STEREO:
	Mode = "normal LR stereo";
	break;
    default:
	Mode = "(unexpected mode value)";
	break;
    }

    /* Convert the emphasis to it's printed representation. Note that
     * the MAD_EMPHASIS_RESERVED enumeration value appeared in libmad
     * version 0.15.0b.
     */
    switch (Header->emphasis) {
    case MAD_EMPHASIS_NONE:
	Emphasis = "no";
	break;
    case MAD_EMPHASIS_50_15_US:
	Emphasis = "50/15 us";
	break;
    case MAD_EMPHASIS_CCITT_J_17:
	Emphasis = "CCITT J.17";
	break;
#if (MAD_VERSION_MAJOR>=1) || \
  ((MAD_VERSION_MAJOR==0) && (MAD_VERSION_MINOR>=15))
    case MAD_EMPHASIS_RESERVED:
	Emphasis = "reserved(!)";
	break;
#endif
    default:
	Emphasis = "(unexpected emphasis value)";
	break;
    }
    sceDisplayWaitVblankStart();
    return (0);
}

/****************************************************************************
* Applies a frequency-domain filter to audio data in the subband-domain.	*
****************************************************************************/
static void ApplyFilter(struct mad_frame *Frame)
{
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

/****************************************************************************
* Converts a sample from libmad's fixed point number format to a signed	*
* short (16 bits).															*
****************************************************************************/
static signed short MadFixedToSshort(mad_fixed_t Fixed)
{
    /* A fixed point number is formed of the following bit pattern:
     *
     * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
     * MSB                          LSB
     * S ==> Sign (0 is positive, 1 is negative)
     * W ==> Whole part bits
     * F ==> Fractional part bits
     *
     * This pattern contains MAD_F_FRACBITS fractional bits, one
     * should alway use this macro when working on the bits of a fixed
     * point number. It is not guaranteed to be constant over the
     * different platforms supported by libmad.
     *
     * The signed short value is formed, after clipping, by the least
     * significant whole part bit, followed by the 15 most significant
     * fractional part bits. Warning: this is a quick and dirty way to
     * compute the 16-bit number, madplay includes much better
     * algorithms.
     */

    /* Clipping */
    if (Fixed >= MAD_F_ONE)
	return (32767);
    if (Fixed <= -MAD_F_ONE)
	return (-32767);

    /* Conversion. */
    Fixed = Fixed >> (MAD_F_FRACBITS - 15);
    return ((signed short) Fixed);
}

static void MP3Callback(void *_buf2, unsigned int numSamples, void *pdata)
{
  short *_buf = (short *)_buf2;
    unsigned long samplesOut = 0;
    //      u8 justStarted = 1;

    if (isPlaying == TRUE) {	//  Playing , so mix up a buffer
	if (samplesInOutput > 0) {
	    //printf("%d samples in buffer\n", samplesInOutput);
	    if (samplesInOutput > numSamples) {
		memcpy((char *) _buf, (char *) OutputBuffer, numSamples * 2 * 2);
		samplesOut = numSamples;
		samplesInOutput -= numSamples;
	    } else {
		memcpy((char *) _buf, (char *) OutputBuffer, samplesInOutput * 2 * 2);
		samplesOut = samplesInOutput;
		samplesInOutput = 0;
	    }
	}
	while (samplesOut < numSamples) {
	    if (Stream.buffer == NULL || Stream.error == MAD_ERROR_BUFLEN) {
		//size_t ReadSize, Remaining;
		//unsigned char *ReadStart;

		/* {2} libmad may not consume all bytes of the input
		 * buffer. If the last frame in the buffer is not wholly
		 * contained by it, then that frame's start is pointed by
		 * the next_frame member of the Stream structure. This
		 * common situation occurs when mad_frame_decode() fails,
		 * sets the stream error code to MAD_ERROR_BUFLEN, and
		 * sets the next_frame pointer to a non NULL value. (See
		 * also the comment marked {4} bellow.)
		 *
		 * When this occurs, the remaining unused bytes must be
		 * put back at the beginning of the buffer and taken in
		 * account before refilling the buffer. This means that
		 * the input buffer must be large enough to hold a whole
		 * frame at the highest observable bit-rate (currently 448
		 * kb/s). XXX=XXX Is 2016 bytes the size of the largest
		 * frame? (448000*(1152/32000))/8
		 */
		/*if(Stream.next_frame!=NULL)
		   {
		   Remaining=Stream.bufend-Stream.next_frame;
		   memmove(InputBuffer,Stream.next_frame,Remaining);
		   ReadStart=InputBuffer+Remaining;
		   ReadSize=INPUT_BUFFER_SIZE-Remaining;
		   }
		   else
		   ReadSize=INPUT_BUFFER_SIZE,
		   ReadStart=InputBuffer,
		   Remaining=0;
		 */
		/* Fill-in the buffer. If an error occurs print a message
		 * and leave the decoding loop. If the end of stream is
		 * reached we also leave the loop but the return status is
		 * left untouched.
		 */
		//ReadSize=BstdRead(ReadStart,1,ReadSize,BstdFile);
		//printf("readsize: %d\n", ReadSize);
		//sceDisplayWaitVblankStart();
		/*if(ReadSize<=0)
		   {
		   //printf("read error on bit-stream (%s - %d)\n", error_to_string(errno), errno);
		   //   Status=1;

		   if(BstdFile->eof) {
		   printf("end of input stream\n");
		   sceDisplayWaitVblankStart();
		   }
		   //break;
		   printf("Readsize was <=0 in player callback\n");
		   sceDisplayWaitVblankStart();
		   } */

		/* {3} When decoding the last frame of a file, it must be
		 * followed by MAD_BUFFER_GUARD zero bytes if one wants to
		 * decode that last frame. When the end of file is
		 * detected we append that quantity of bytes at the end of
		 * the available data. Note that the buffer can't overflow
		 * as the guard size was allocated but not used the the
		 * buffer management code. (See also the comment marked
		 * {1}.)
		 *
		 * In a message to the mad-dev mailing list on May 29th,
		 * 2001, Rob Leslie explains the guard zone as follows:
		 *
		 *    "The reason for MAD_BUFFER_GUARD has to do with the
		 *    way decoding is performed. In Layer III, Huffman
		 *    decoding may inadvertently read a few bytes beyond
		 *    the end of the buffer in the case of certain invalid
		 *    input. This is not detected until after the fact. To
		 *    prevent this from causing problems, and also to
		 *    ensure the next frame's main_data_begin pointer is
		 *    always accessible, MAD requires MAD_BUFFER_GUARD
		 *    (currently 8) bytes to be present in the buffer past
		 *    the end of the current frame in order to decode the
		 *    frame."
		 */
		/*if(BstdFileEofP(BstdFile))
		   {
		   GuardPtr=ReadStart+ReadSize;
		   memset(GuardPtr,0,MAD_BUFFER_GUARD);
		   ReadSize+=MAD_BUFFER_GUARD;
		   } */

		/* Pipe the new buffer content to libmad's stream decoder
		 * facility.
		 */
		mad_stream_buffer(&Stream, ptr, size);
		Stream.error = MAD_ERROR_NONE;
	    }

	    /* Decode the next MPEG frame. The streams is read from the
	     * buffer, its constituents are break down and stored the the
	     * Frame structure, ready for examination/alteration or PCM
	     * synthesis. Decoding options are carried in the Frame
	     * structure from the Stream structure.
	     *
	     * Error handling: mad_frame_decode() returns a non zero value
	     * when an error occurs. The error condition can be checked in
	     * the error member of the Stream structure. A mad error is
	     * recoverable or fatal, the error status is checked with the
	     * MAD_RECOVERABLE macro.
	     *
	     * {4} When a fatal error is encountered all decoding
	     * activities shall be stopped, except when a MAD_ERROR_BUFLEN
	     * is signaled. This condition means that the
	     * mad_frame_decode() function needs more input to complete
	     * its work. One shall refill the buffer and repeat the
	     * mad_frame_decode() call. Some bytes may be left unused at
	     * the end of the buffer if those bytes forms an incomplete
	     * frame. Before refilling, the remaining bytes must be moved
	     * to the beginning of the buffer and used for input for the
	     * next mad_frame_decode() invocation. (See the comments
	     * marked {2} earlier for more details.)
	     *
	     * Recoverable errors are caused by malformed bit-streams, in
	     * this case one can call again mad_frame_decode() in order to
	     * skip the faulty part and re-sync to the next frame.
	     */
	    if (mad_frame_decode(&Frame, &Stream)) {
		if (MAD_RECOVERABLE(Stream.error)) {
		    /* Do not print a message if the error is a loss of
		     * synchronization and this loss is due to the end of
		     * stream guard bytes. (See the comments marked {3}
		     * supra for more informations about guard bytes.)
		     */
		    if (Stream.error != MAD_ERROR_LOSTSYNC || Stream.this_frame != GuardPtr) {
			sceDisplayWaitVblankStart();
		    }
		    return;	//continue;
		} else if (Stream.error == MAD_ERROR_BUFLEN) {
		    eos = 1;
		    return;	//continue;
		} else {
		    sceDisplayWaitVblankStart();
		    Status = 1;
		    MP3_Stop();	//break;
		}
	    }

	    /* The characteristics of the stream's first frame is printed
	     * on stderr. The first frame is representative of the entire
	     * stream.
	     */
	    if (FrameCount == 0)
		if (PrintFrameInfo(&Frame.header)) {
		    Status = 1;
		    //break;
		}

	    /* Accounting. The computed frame duration is in the frame
	     * header structure. It is expressed as a fixed point number
	     * whole data type is mad_timer_t. It is different from the
	     * samples fixed point format and unlike it, it can't directly
	     * be added or subtracted. The timer module provides several
	     * functions to operate on such numbers. Be careful there, as
	     * some functions of libmad's timer module receive some of
	     * their mad_timer_t arguments by value!
	     */
	    FrameCount++;
	    mad_timer_add(&Timer, Frame.header.duration);

	    /* Between the frame decoding and samples synthesis we can
	     * perform some operations on the audio data. We do this only
	     * if some processing was required. Detailed explanations are
	     * given in the ApplyFilter() function.
	     */
	    if (DoFilter)
		ApplyFilter(&Frame);

	    /* Once decoded the frame is synthesized to PCM samples. No errors
	     * are reported by mad_synth_frame();
	     */
	    mad_synth_frame(&Synth, &Frame);

	    /* Synthesized samples must be converted from libmad's fixed
	     * point number to the consumer format. Here we use unsigned
	     * 16 bit big endian integers on two channels. Integer samples
	     * are temporarily stored in a buffer that is flushed when
	     * full.
	     */

	    for (i = 0; i < Synth.pcm.length; i++) {
		signed short Sample;
		//printf("%d < %d\n", samplesOut, numSamples);
		if (samplesOut < numSamples) {
		    //printf("I really get here\n");
		    /* Left channel */
		    Sample = MadFixedToSshort(Synth.pcm.samples[0][i]);
		    // *(OutputPtr++)=Sample>>8;
		    // *(OutputPtr++)=Sample&0xff;
		    _buf[samplesOut * 2] = Sample;

		    /* Right channel. If the decoded stream is monophonic then
		     * the right output channel is the same as the left one.
		     */
		    if (MAD_NCHANNELS(&Frame.header) == 2)
			Sample = MadFixedToSshort(Synth.pcm.samples[1][i]);
		    // *(OutputPtr++)=Sample>>8;
		    // *(OutputPtr++)=Sample&0xff;
		    //_buf[samplesOut*2]=0;//Sample;
		    _buf[samplesOut * 2 + 1] = Sample;
		    samplesOut++;
		} else {
		    //printf("%d < %d of %d\n", samplesOut, numSamples, Synth.pcm.length);
		    Sample = MadFixedToSshort(Synth.pcm.samples[0][i]);
		    OutputBuffer[samplesInOutput * 2] = Sample;
		    //OutputBuffer[samplesInOutput*4+1]=0;//Sample>>8;
		    //OutputBuffer[samplesInOutput*4+2]=0;//Sample&0xff;
		    if (MAD_NCHANNELS(&Frame.header) == 2)
			Sample = MadFixedToSshort(Synth.pcm.samples[1][i]);
		    OutputBuffer[samplesInOutput * 2 + 1] = Sample;
		    //OutputBuffer[samplesInOutput*4+3]=0;//Sample>>8;
		    //OutputBuffer[samplesInOutput*4+4]=0;//Sample&0xff;
		    samplesInOutput++;
		}

	    }
	}
    } else {			//  Not Playing , so clear buffer
	{
	    unsigned int count;
	    for (count = 0; count < numSamples * 2; count++)
		*(_buf + count) = 0;
	}
    }
}

void MP3_Init(int channel)
{
    myChannel = channel;
    isPlaying = FALSE;
    vitaAudioSetChannelCallback(myChannel, MP3Callback,0);
    /* First the structures used by libmad must be initialized. */
    mad_stream_init(&Stream);
    mad_frame_init(&Frame);
    mad_synth_init(&Synth);
    mad_timer_reset(&Timer);
    //ModPlay_Load("",data);
}


void MP3_FreeTune()
{
    /* The input file was completely read; the memory allocated by our
     * reading module must be reclaimed.
     */
    if (ptr)
	free(ptr);
    //sceIoClose(BstdFile->fd);
    //BstdFileDestroy(BstdFile);

    /* Mad is no longer used, the structures that were initialized must
     * now be cleared.
     */
    mad_synth_finish(&Synth);
    mad_frame_finish(&Frame);
    mad_stream_finish(&Stream);

    /* If the output buffer is not empty and no error occurred during
     * the last write, then flush it.
     */
    /*if(OutputPtr!=OutputBuffer && Status!=2)
       {
       size_t       BufferSize=OutputPtr-OutputBuffer;

       if(fwrite(OutputBuffer,1,BufferSize,OutputFp)!=BufferSize)
       {
       fprintf(stderr,"%s: PCM write error (%s).\n",
       ProgName,strerror(errno));
       Status=2;
       }
       } */

    /* Accounting report if no error occurred. */
    if (!Status) {
	char Buffer[80];

	/* The duration timer is converted to a human readable string
	 * with the versatile, but still constrained mad_timer_string()
	 * function, in a fashion not unlike strftime(). The main
	 * difference is that the timer is broken into several
	 * values according some of it's arguments. The units and
	 * fracunits arguments specify the intended conversion to be
	 * executed.
	 *
	 * The conversion unit (MAD_UNIT_MINUTES in our example) also
	 * specify the order and kind of conversion specifications
	 * that can be used in the format string.
	 *
	 * It is best to examine libmad's timer.c source-code for details
	 * of the available units, fraction of units, their meanings,
	 * the format arguments, etc.
	 */
	mad_timer_string(Timer, Buffer, "%lu:%02lu.%03u", MAD_UNITS_MINUTES, MAD_UNITS_MILLISECONDS, 0);
	sceDisplayWaitVblankStart();
	sceKernelDelayThread(500000);
    }
}

void MP3_End()
{
    MP3_Stop();
    vitaAudioSetChannelCallback(myChannel, 0,0);
    MP3_FreeTune();
}

//////////////////////////////////////////////////////////////////////
// Functions - Local and not public
//////////////////////////////////////////////////////////////////////

//  This is the initialiser and module loader
//  This is a general call, which loads the module from the 
//  given address into the modplayer
//
//  It basically loads into an internal format, so once this function
//  has returned the buffer at 'data' will not be needed again.
int MP3_Load(char *filename)
{
    int fd;
    eos = 0;
    //psp_stats pstat;
    //sceIoGetstat(filename, &pstat);
    if ((fd = sceIoOpen(filename, SCE_O_RDONLY, 0777)) > 0) {
		//  opened file, so get size now
		size = sceIoLseek(fd, 0, SCE_SEEK_END);
		sceIoLseek(fd, 0, SCE_SEEK_SET);
		ptr = (unsigned char *) malloc(size + 8);
		memset(ptr, 0, size + 8);
		if (ptr != 0) {		// Read file in
			sceIoRead(fd, ptr, size);
		} else {
			sceIoClose(fd);
			return 0;
		}
		// Close file
		sceIoClose(fd);
    } else {
		return 0;
    }
    //  Set volume to full ready to play
    //SetMasterVolume(64);
    isPlaying = FALSE;
    return 1;
}

// This function initialises for playing, and starts
int MP3_Play()
{
    // See if I'm already playing
    if (isPlaying)
	return FALSE;

    isPlaying = TRUE;
    return TRUE;
}

void MP3_Pause()
{
    isPlaying = !isPlaying;
}

int MP3_Stop()
{
    //stop playing
    isPlaying = FALSE;

    //clear buffer
    memset(OutputBuffer, 0, OUTPUT_BUFFER_SIZE);
    OutputPtr = (unsigned char *) OutputBuffer;

    //seek to beginning of file
    //sceIoLseek(BstdFile->fd, 0, SEEK_SET);

    return TRUE;
}

void MP3_GetTimeString(char *dest)
{
    mad_timer_string(Timer, dest, "%02lu:%02u:%02u", MAD_UNITS_HOURS, MAD_UNITS_MILLISECONDS, 0);
}

int MP3_EndOfStream()
{
    if (eos == 1)
	return 1;
    return 0;
}
