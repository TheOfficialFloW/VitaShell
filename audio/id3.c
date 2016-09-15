#include <psp2/io/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "id3.h"
struct genre
{
        int code;
        char text[112];
};

struct genre genreList[] =
{
   {0 , "Blues"}, {1 , "Classic Rock"}, {2 , "Country"}, {3 , "Dance"}, {4 , "Disco"}, {5 , "Funk"}, {6 , "Grunge"}, {7 , "Hip-Hop"}, {8 , "Jazz"}, {9 , "Metal"}, {10 , "New Age"},
   {11 , "Oldies"}, {12 , "Other"}, {13 , "Pop"}, {14 , "R&B"}, {15 , "Rap"}, {16 , "Reggae"}, {17 , "Rock"}, {18 , "Techno"}, {19 , "Industrial"}, {20 , "Alternative"},
   {21 , "Ska"}, {22 , "Death Metal"}, {23 , "Pranks"}, {24 , "Soundtrack"}, {25 , "Euro-Techno"}, {26 , "Ambient"}, {27 , "Trip-Hop"}, {28 , "Vocal"}, {29 , "Jazz+Funk"}, {30 , "Fusion"},
   {31 , "Trance"}, {32 , "Classical"}, {33 , "Instrumental"}, {34 , "Acid"}, {35 , "House"}, {36 , "Game"}, {37 , "Sound Clip"}, {38 , "Gospel"}, {39 , "Noise"}, {40 , "Alternative Rock"},
   {41 , "Bass"}, {42 , "Soul"}, {43 , "Punk"}, {44 , "Space"}, {45 , "Meditative"}, {46 , "Instrumental Pop"}, {47 , "Instrumental Rock"}, {48 , "Ethnic"}, {49 , "Gothic"}, {50 , "Darkwave"},
   {51 , "Techno-Industrial"}, {52 , "Electronic"}, {53 , "Pop-Folk"}, {54 , "Eurodance"}, {55 , "Dream"}, {56 , "Southern Rock"}, {57 , "Comedy"}, {58 , "Cult"}, {59 , "Gangsta"}, {60 , "Top 40"},
   {61 , "Christian Rap"}, {62 , "Pop/Funk"}, {63 , "Jungle"}, {64 , "Native US"}, {65 , "Cabaret"}, {66 , "New Wave"}, {67 , "Psychadelic"}, {68 , "Rave"}, {69 , "Showtunes"}, {70 , "Trailer"},
   {71 , "Lo-Fi"}, {72 , "Tribal"}, {73 , "Acid Punk"}, {74 , "Acid Jazz"}, {75 , "Polka"}, {76 , "Retro"}, {77 , "Musical"}, {78 , "Rock & Roll"}, {79 , "Hard Rock"}, {80 , "Folk"},
   {81 , "Folk-Rock"}, {82 , "National Folk"}, {83 , "Swing"}, {84 , "Fast Fusion"}, {85 , "Bebob"}, {86 , "Latin"}, {87 , "Revival"}, {88 , "Celtic"}, {89 , "Bluegrass"}, {90 , "Avantgarde"},
   {91 , "Gothic Rock"}, {92 , "Progressive Rock"}, {93 , "Psychedelic Rock"}, {94 , "Symphonic Rock"}, {95 , "Slow Rock"}, {96 , "Big Band"}, {97 , "Chorus"}, {98 , "Easy Listening"}, {99 , "Acoustic"},
   {100 , "Humour"}, {101 , "Speech"}, {102 , "Chanson"}, {103 , "Opera"}, {104 , "Chamber Music"}, {105 , "Sonata"}, {106 , "Symphony"}, {107 , "Booty Bass"}, {108 , "Primus"}, {109 , "Porn Groove"},
   {110 , "Satire"}, {111 , "Slow Jam"}, {112 , "Club"}, {113 , "Tango"}, {114 , "Samba"}, {115 , "Folklore"}, {116 , "Ballad"}, {117 , "Power Ballad"}, {118 , "Rhytmic Soul"}, {119 , "Freestyle"}, {120 , "Duet"},
   {121 , "Punk Rock"}, {122 , "Drum Solo"}, {123 , "A capella"}, {124 , "Euro-House"}, {125 , "Dance Hall"}, {126 , "Goa"}, {127 , "Drum & Bass"}, {128 , "Club-House"}, {129 , "Hardcore"}, {130 , "Terror"},
   {131 , "Indie"}, {132 , "BritPop"}, {133 , "Negerpunk"}, {134 , "Polsk Punk"}, {135 , "Beat"}, {136 , "Christian Gangsta"}, {137 , "Heavy Metal"}, {138 , "Black Metal"}, {139 , "Crossover"}, {140 , "Contemporary C"},
   {141 , "Christian Rock"}, {142 , "Merengue"}, {143 , "Salsa"}, {144 , "Thrash Metal"}, {145 , "Anime"}, {146 , "JPop"}, {147 , "SynthPop"}
};
int genreNumber = sizeof (genreList) / sizeof (struct genre);


//Search for FF+D8+FF bytes (first bytes of a jpeg image)
//Returns file position:
int searchJPGstart(int fp, int delta){
    int retValue = -1;
    int i = 0;

    unsigned char *tBuffer = malloc(sizeof(unsigned char) * (delta + 2));
    if (tBuffer == NULL)
        return -1;

    int startPos = sceIoLseek(fp, 0, SCE_SEEK_CUR);
    sceIoRead(fp, tBuffer, delta + 2);
    sceIoLseek(fp, startPos, SCE_SEEK_SET);

    unsigned char *buff = tBuffer;
    for (i=0; i<delta; i++){
        if(!memcmp(buff++, ID3_JPEG, 3)){
            retValue = startPos + i;
            break;
        }
    }
    free(tBuffer);
    return retValue;
}

//Search for 89 50 4E 47 0D 0A 1A 0A 00 00 00 0D 49 48 44 52 bytes (first bytes of a PNG image)
//Returns file position:
int searchPNGstart(int fp, int delta){
    int retValue = -1;
    int i = 0;

    unsigned char *tBuffer = malloc(sizeof(unsigned char) * (delta + 15));
    if (tBuffer == NULL)
        return -1;

    int startPos = sceIoLseek(fp, 0, SCE_SEEK_CUR);
    sceIoRead(fp, tBuffer, delta + 15);
    sceIoLseek(fp, startPos, SCE_SEEK_SET);

    unsigned char *buff = tBuffer;
    for (i=0; i<delta; i++){
        if(!memcmp(buff++, ID3_PNG, 16)){
            retValue = startPos + i;
            break;
        }
    }
    free(tBuffer);
    return retValue;
}

// ID3v2 code taken from libID3 by Xart
// http://www.xart.co.uk
short int swapInt16BigToHost(short int arg)
{
   short int i=0;
   int checkEndian = 1;
   if( 1 == *(char *)&checkEndian )
   {
      // Intel (little endian)
      i=arg;
      i=((i&0xFF00)>>8)|((i&0x00FF)<<8);
   }
   else
   {
      // PPC (big endian)
      i=arg;
   }
   return i;
}

int swapInt32BigToHost(int arg)
{
   int i=0;
   int checkEndian = 1;
   if( 1 == *(char *)&checkEndian )
   {
      // Intel (little endian)
      i=arg;
      i=((i&0xFF000000)>>24)|((i&0x00FF0000)>>8)|((i&0x0000FF00)<<8)|((i&0x000000FF)<<24);
   }
   else
   {
      // PPC (big endian)
      i=arg;
   }
   return i;
}

static void utf16_to_utf8(uint16_t *src, uint8_t *dst) {
	int i;
	for (i = 0; src[i]; i++) {
		if ((src[i] & 0xFF80) == 0) {
			*(dst++) = src[i] & 0xFF;
		} else if((src[i] & 0xF800) == 0) {
			*(dst++) = ((src[i] >> 6) & 0xFF) | 0xC0;
			*(dst++) = (src[i] & 0x3F) | 0x80;
		} else if((src[i] & 0xFC00) == 0xD800 && (src[i + 1] & 0xFC00) == 0xDC00) {
			*(dst++) = (((src[i] + 64) >> 8) & 0x3) | 0xF0;
			*(dst++) = (((src[i] >> 2) + 16) & 0x3F) | 0x80;
			*(dst++) = ((src[i] >> 4) & 0x30) | 0x80 | ((src[i + 1] << 2) & 0xF);
			*(dst++) = (src[i + 1] & 0x3F) | 0x80;
			i += 1;
		} else {
			*(dst++) = ((src[i] >> 12) & 0xF) | 0xE0;
			*(dst++) = ((src[i] >> 6) & 0x3F) | 0x80;
			*(dst++) = (src[i] & 0x3F) | 0x80;
		}
	}

	*dst = '\0';
}

//Reads tag data purging invalid characters:
void readTagData(int fp, int tagLength, int maxTagLength, char *tagValue){
	// TheFloW: This check corrupts the seeking offset.
    // if (tagLength > maxTagLength)
    //     tagLength = maxTagLength;

    int i;
    int count = 0;
    unsigned short carattere16[tagLength/2+2];
	unsigned char carattere8[128];
    unsigned char* carattere = (unsigned char*)carattere16;
    char* utf8Tag;

    strcpy(tagValue, "");
    tagValue[0] = '\0';

    sceIoRead(fp, carattere, tagLength);
    carattere[tagLength] = '\0';
    carattere[tagLength+1] = '\0';

    // unicode tag
    if ( carattere16[0] == 0xFEFF ) {
        // utf8Tag = miniConvUTF16LEConv( carattere16 + 1 );
		utf16_to_utf8(carattere16 + 1, carattere8);
		utf8Tag = (char*)carattere8;
    }
    // encode to utf8
    else {
       /* if ( miniConvHaveDefaultSubtitleConv() )
                utf8Tag = miniConvDefaultSubtitleConv( carattere );
        else */
                utf8Tag = (char*)carattere;
    }
    if ( utf8Tag == NULL || !strlen(utf8Tag)) {
        for (i=0; i<tagLength; i++){
            if (carattere[i] >= 0x20 && carattere[i] <= 0xfd) //<= 0x7f
                    tagValue[count++] = carattere[i];
        }
        tagValue[count] = '\0';
    }
    else
    {
            strcpy( tagValue, utf8Tag );
    }
}

int ID3v2TagSize(const char *mp3path)
{
   int fp = 0;
   int size;
   char sig[3];

   fp = sceIoOpen(mp3path, SCE_O_RDONLY, 0777);
   if (fp < 0) return 0;

   sceIoRead(fp, sig, sizeof(sig));
   if (strncmp("ID3",sig,3) != 0) {
      sceIoClose(fp);
      return 0;
   }

   sceIoLseek(fp, 6, SCE_SEEK_SET);
   sceIoRead(fp, &size, sizeof(unsigned int));
   /*
    *  The ID3 tag size is encoded with four bytes where the first bit
    *  (bit 7) is set to zero in every byte, making a total of 28 bits. The zeroed
    *  bits are ignored, so a 257 bytes long tag is represented as $00 00 02 01.
    */

   size = (unsigned int) swapInt32BigToHost((int)size);
   size = ( ( (size & 0x7f000000) >> 3 ) | ( (size & 0x7f0000) >> 2 ) | ( (size & 0x7f00) >> 1 ) | (size & 0x7f) );
   sceIoClose(fp);
   return size;
}

int ID3v2(const char *mp3path)
{
   char sig[3];
   unsigned short int version;


   int fp = sceIoOpen(mp3path, SCE_O_RDONLY, 0777);
   if (fp < 0) return 0;

   sceIoRead(fp, sig, sizeof(sig));
   if (!strncmp("ID3",sig,3)) {
          sceIoRead(fp, &version, sizeof(unsigned short int));
      version = (unsigned short int) swapInt16BigToHost((short int)version);
      version /= 256;
   }
   sceIoClose(fp);

   return (int)version;
}

void ParseID3v2_2(const char *mp3path, struct ID3Tag *id3tag)
{
   int fp = 0;

   int size;
   int tag_length;
   char tag[3];
   char buffer[20];

   //if(ID3v2(mp3path) == 2) {
      size = ID3v2TagSize(mp3path);
          fp = sceIoOpen(mp3path, SCE_O_RDONLY, 0777);
      if (fp < 0) return;
          sceIoLseek(fp, 10, SCE_SEEK_SET);

      while (size != 0) {
                 sceIoRead(fp, tag, 3);
         size -= 3;

         /* read 3 byte big endian tag length */
             sceIoRead(fp, &tag_length, sizeof(unsigned int));
                 sceIoLseek(fp, -1, SCE_SEEK_CUR);

         tag_length = (unsigned int) swapInt32BigToHost((int)tag_length);
         tag_length = (tag_length / 256);
         size -= 3;

         /* Perform checks for end of tags and tag length overflow or zero */
         if(*tag == 0 || tag_length > size || tag_length == 0) break;

         if(!strncmp("TP1",tag,3)) /* Artist */
         {
                        sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Artist);
         }
         else if(!strncmp("TT2",tag,3)) /* Title */
         {
                        sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Title);
         }
         else if(!strncmp("TAL",tag,3)) /* Album */
         {
                    sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Album);
         }
         else if(!strncmp("TRK",tag,3)) /* Track No. */
         {
                        sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 8, id3tag->ID3TrackText);
            id3tag->ID3Track = atoi(id3tag->ID3TrackText);
         }
         else if(!strncmp("TYE",tag,3)) /* Year */
         {
                        sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 12, id3tag->ID3Year);
         }
         else if(!strncmp("TLE",tag,3)) /* Length */
         {
                        sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 264, buffer);
            id3tag->ID3Length = atoi(buffer);
         }
         else if(!strncmp("COM",tag,3)) /* Comment */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Comment);
         }
         else if(!strncmp("TCO",tag,3)) /* Genre */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3GenreText);
            if (id3tag->ID3GenreText[0] == '(' && id3tag->ID3GenreText[strlen(id3tag->ID3GenreText) - 1] == ')')
            {
                id3tag->ID3GenreText[0] = ' ';
                id3tag->ID3GenreText[strlen(id3tag->ID3GenreText) - 1] = '\0';
                int index = atoi(id3tag->ID3GenreText);
                if (index >= 0 && index < genreNumber)
                    strcpy(id3tag->ID3GenreText, genreList[index].text);
            }
         }
         else if(!strncmp("PIC",tag,3)) /* Picture */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            sceIoLseek(fp, 5, SCE_SEEK_CUR);
            id3tag->ID3EncapsulatedPictureType = JPEG_IMAGE;
            id3tag->ID3EncapsulatedPictureOffset = searchJPGstart(fp, tag_length - 1);
            if (id3tag->ID3EncapsulatedPictureOffset < 0){
                id3tag->ID3EncapsulatedPictureType = PNG_IMAGE;
                id3tag->ID3EncapsulatedPictureOffset = searchPNGstart(fp, tag_length - 1);
            }
            tag_length = tag_length - (id3tag->ID3EncapsulatedPictureOffset - sceIoLseek(fp, 0, SCE_SEEK_CUR));
            id3tag->ID3EncapsulatedPictureLength = tag_length-6;
                        sceIoLseek(fp, tag_length-6, SCE_SEEK_CUR);
            if (id3tag->ID3EncapsulatedPictureOffset < 0){
                id3tag->ID3EncapsulatedPictureType = 0;
                id3tag->ID3EncapsulatedPictureOffset = 0;
                id3tag->ID3EncapsulatedPictureLength = 0;
            }
         }
         else
         {
                        sceIoLseek(fp, tag_length, SCE_SEEK_CUR);
         }
         size -= tag_length;
      }
      strcpy(id3tag->versionfound, "2.2");
      sceIoClose(fp);
   //}
}

void ParseID3v2_3(const char *mp3path, struct ID3Tag *id3tag)
{
   int fp = 0;

   int size;
   int tag_length;
   char tag[4];
   char buffer[20];

   //if(ID3v2(mp3path) == 3) {
      size = ID3v2TagSize(mp3path);
      fp = sceIoOpen(mp3path, SCE_O_RDONLY, 0777);
      if (fp < 0) return;
      sceIoLseek(fp, 10, SCE_SEEK_SET);

      while (size != 0) {
         sceIoRead(fp, tag, 4);
         size -= 4;

         /* read 4 byte big endian tag length */
         sceIoRead(fp, &tag_length, sizeof(unsigned int));
         tag_length = (unsigned int) swapInt32BigToHost((int)tag_length);
         size -= 4;

         sceIoLseek(fp, 2, SCE_SEEK_CUR);
         size -= 2;

         /* Perform checks for end of tags and tag length overflow or zero */
         if(*tag == 0 || tag_length > size || tag_length == 0) break;

         if(!strncmp("TPE1",tag,4)) /* Artist */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Artist);
         }
         else if(!strncmp("TIT2",tag,4)) /* Title */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Title);
         }
         else if(!strncmp("TALB",tag,4)) /* Album */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Album);
         }
         else if(!strncmp("TRCK",tag,4)) /* Track No. */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 8, id3tag->ID3TrackText);
            id3tag->ID3Track = atoi(id3tag->ID3TrackText);
         }
         else if(!strncmp("TYER",tag,4)) /* Year */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 12, id3tag->ID3Year);
         }
         else if(!strncmp("TLEN",tag,4)) /* Length in milliseconds */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 264, buffer);
            id3tag->ID3Length = atol(buffer) / 1000;
         }
         else if(!strncmp("TCON",tag,4)) /* Genre */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3GenreText);
            if (id3tag->ID3GenreText[0] == '(' && id3tag->ID3GenreText[strlen(id3tag->ID3GenreText) - 1] == ')')
            {
                id3tag->ID3GenreText[0] = ' ';
                id3tag->ID3GenreText[strlen(id3tag->ID3GenreText) - 1] = '\0';
                int index = atoi(id3tag->ID3GenreText);
                if (index >= 0 && index < genreNumber)
                    strcpy(id3tag->ID3GenreText, genreList[index].text);
            }
         }
         else if(!strncmp("COMM",tag,4)) /* Comment */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Comment);
         }
         else if(!strncmp("APIC",tag,4)) /* Picture */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            sceIoLseek(fp, 12, SCE_SEEK_CUR);
            id3tag->ID3EncapsulatedPictureType = JPEG_IMAGE;
            id3tag->ID3EncapsulatedPictureOffset = searchJPGstart(fp, tag_length - 1);
            if (id3tag->ID3EncapsulatedPictureOffset < 0){
                id3tag->ID3EncapsulatedPictureType = PNG_IMAGE;
                id3tag->ID3EncapsulatedPictureOffset = searchPNGstart(fp, tag_length - 1);
            }
            tag_length = tag_length - (id3tag->ID3EncapsulatedPictureOffset - sceIoLseek(fp, 0, SCE_SEEK_CUR));
            id3tag->ID3EncapsulatedPictureLength = tag_length-13;
            sceIoLseek(fp, tag_length-13, SCE_SEEK_CUR);
            if (id3tag->ID3EncapsulatedPictureOffset < 0){
                id3tag->ID3EncapsulatedPictureType = 0;
                id3tag->ID3EncapsulatedPictureOffset = 0;
                id3tag->ID3EncapsulatedPictureLength = 0;
            }
         }
         else
         {
            sceIoLseek(fp, tag_length, SCE_SEEK_CUR);
         }
         size -= tag_length;
      }
      strcpy(id3tag->versionfound, "2.3");
      sceIoClose(fp);
   //}
}


void ParseID3v2_4(const char *mp3path, struct ID3Tag *id3tag)
{
   int fp = 0;

   int size;
   int tag_length;
   char tag[4];
   char buffer[20];

   //if(ID3v2(mp3path) == 4) {
      size = ID3v2TagSize(mp3path);
      fp = sceIoOpen(mp3path, SCE_O_RDONLY, 0777);
      if (fp < 0) return;
      sceIoLseek(fp, 10, SCE_SEEK_SET);

      while (size != 0) {
         sceIoRead(fp, tag, 4);
         size -= 4;

         /* read 4 byte big endian tag length */
         sceIoRead(fp, &tag_length, sizeof(unsigned int));
         tag_length = (unsigned int) swapInt32BigToHost((int)tag_length);
         size -= 4;

         sceIoLseek(fp, 2, SCE_SEEK_CUR);
         size -= 2;

         /* Perform checks for end of tags and tag length overflow or zero */
         if(*tag == 0 || tag_length > size || tag_length == 0) break;

         if(!strncmp("TPE1",tag,4)) /* Artist */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Artist);
         }
         else if(!strncmp("TIT2",tag,4)) /* Title */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Title);
         }
         else if(!strncmp("TALB",tag,4)) /* Album */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Album);
         }
         else if(!strncmp("TRCK",tag,4)) /* Track No. */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 8, id3tag->ID3TrackText);
            id3tag->ID3Track = atoi(id3tag->ID3TrackText);
         }
         else if(!strncmp("TYER",tag,4)) /* Year */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 12, id3tag->ID3Year);
         }
         else if(!strncmp("TLEN",tag,4)) /* Length in milliseconds */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 264, buffer);
            id3tag->ID3Length = atol(buffer) / 1000;
         }
         else if(!strncmp("TCON",tag,4)) /* Genre */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3GenreText);
         }
         else if(!strncmp("COMM",tag,4)) /* Comment */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, id3tag->ID3Comment);
         }
         else if(!strncmp("APIC",tag,4)) /* Picture */
         {
            sceIoLseek(fp, 1, SCE_SEEK_CUR);
            sceIoLseek(fp, 12, SCE_SEEK_CUR);
            id3tag->ID3EncapsulatedPictureType = JPEG_IMAGE;
            id3tag->ID3EncapsulatedPictureOffset = searchJPGstart(fp, tag_length - 1);
            if (id3tag->ID3EncapsulatedPictureOffset < 0){
                id3tag->ID3EncapsulatedPictureType = PNG_IMAGE;
                id3tag->ID3EncapsulatedPictureOffset = searchPNGstart(fp, tag_length - 1);
            }
            tag_length = tag_length - (id3tag->ID3EncapsulatedPictureOffset - sceIoLseek(fp, 0, SCE_SEEK_CUR));
            id3tag->ID3EncapsulatedPictureLength = tag_length-13;
            sceIoLseek(fp, tag_length-13, SCE_SEEK_CUR);
            if (id3tag->ID3EncapsulatedPictureOffset < 0){
                id3tag->ID3EncapsulatedPictureType = 0;
                id3tag->ID3EncapsulatedPictureOffset = 0;
                id3tag->ID3EncapsulatedPictureLength = 0;
            }
         }
         else
         {
            sceIoLseek(fp, tag_length, SCE_SEEK_CUR);
         }
         size -= tag_length;
      }
      strcpy(id3tag->versionfound, "2.4");
      sceIoClose(fp);
   //}
}

int ParseID3v2(const char *mp3path, struct ID3Tag *id3tag)
{
   switch (ID3v2(mp3path)) {
      case 2:
         ParseID3v2_2(mp3path, id3tag);
         break;
      case 3:
         ParseID3v2_3(mp3path, id3tag);
         break;
      case 4:
         ParseID3v2_4(mp3path, id3tag);
         break;
      default:
         return -1;
   }

   /* If no Title is found, uses filename - extension for Title. */
   /*if(*id3tag->ID3Title == 0) {
      strcpy(id3tag->ID3Title,strrchr(mp3path,'/') + 1);
      if (*strrchr(id3tag->ID3Title,'.') != 0) *strrchr(id3tag->ID3Title,'.') = 0;
   }*/
   return 0;
}

int ParseID3v1(const char *mp3path, struct ID3Tag *id3tag){
    int id3fd; //our local file descriptor
    char id3buffer[512];
    id3fd = sceIoOpen(mp3path, SCE_O_RDONLY, 0777);
    if (id3fd < 0)
        return -1;
    sceIoLseek(id3fd, -128, SEEK_END);
    sceIoRead(id3fd,id3buffer,128);

    if (strstr(id3buffer,"TAG") != NULL){
        sceIoLseek(id3fd, -125, SEEK_END);
        sceIoRead(id3fd,id3tag->ID3Title,30);
        id3tag->ID3Title[30] = '\0';

        sceIoLseek(id3fd, -95, SEEK_END);
        sceIoRead(id3fd,id3tag->ID3Artist,30);
        id3tag->ID3Artist[30] = '\0';

        sceIoLseek(id3fd, -65, SEEK_END);
        sceIoRead(id3fd,id3tag->ID3Album,30);
        id3tag->ID3Album[30] = '\0';

        sceIoLseek(id3fd, -35, SEEK_END);
        sceIoRead(id3fd,id3tag->ID3Year,4);
        id3tag->ID3Year[4] = '\0';

        sceIoLseek(id3fd, -31, SEEK_END);
        sceIoRead(id3fd,id3tag->ID3Comment,30);
        id3tag->ID3Comment[30] = '\0';

        sceIoLseek(id3fd, -1, SEEK_END);
        sceIoRead(id3fd,id3tag->ID3GenreCode,1);
        id3tag->ID3GenreCode[1] = '\0';

        /* Track */
        if (*(id3tag->ID3Comment + 28) == 0 && *(id3tag->ID3Comment + 29) > 0) {
           id3tag->ID3Track = (int)*(id3tag->ID3Comment + 29);
           strcpy(id3tag->versionfound, "1.1");
        } else {
           id3tag->ID3Track = 1;
           strcpy(id3tag->versionfound, "1.0");
        }

        if (((int)id3tag->ID3GenreCode[0] >= 0) & ((int)id3tag->ID3GenreCode[0] < genreNumber)){
                strcpy(id3tag->ID3GenreText, genreList[(int)id3tag->ID3GenreCode[0]].text);
        }
        else{
                strcpy(id3tag->ID3GenreText, "");
        }
        id3tag->ID3GenreText[30] = '\0';
     }else{
        sceIoClose(id3fd);
        return -1;
     }
     sceIoClose(id3fd);
     return 0;
}

// Main function:
int ParseID3(char *mp3path, struct ID3Tag *target)
{
        memset(target, 0, sizeof(struct ID3Tag));

    ParseID3v1(mp3path, target);
    ParseID3v2(mp3path, target);
//    if (!strlen(target->ID3Title))
//        getFileName(mp3path, target->ID3Title);
    return 0;
}
