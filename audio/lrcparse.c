/*
	VitaShell
	Copyright (C) 2015-2017, TheFloW

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
#include "lrcparse.h"

static char* getStringFromRegmatch(char* source,size_t so,size_t eo)
{
    size_t wordlength = eo - so;
    char* word;

    if(wordlength < 2){
        word = malloc(sizeof(char));
        word[0] = '\0';
    }else{
        word = malloc(sizeof(char) * (wordlength + 1));
        memcpy((void*)word,(void*)(source + so),wordlength);//copy string
        word[sizeof(char) * (wordlength)] = '\0';
    }
    return word;
}

Lyrics* lrcParseLoadWithFile(char* lrcfilepath)
{
    int filesize = getFileSize(lrcfilepath);

    if(filesize < 0)
        return NULL;

    char lrcbuffer[filesize];

    if(ReadFile(lrcfilepath,(void*)lrcbuffer,filesize) < 0)
        return NULL;

    return lrcParseLoadWithBuffer(lrcbuffer);
}

Lyrics* lrcParseLoadWithBuffer(char* buffer)
{
    regmatch_t pm[5];
    regex_t preg;

    char* pattern = "\\[([0-9]{2}):([0-5][0-9])\\.([0-9]{2})\\](.*)";

    if (regcomp(&preg, pattern, REG_EXTENDED |REG_NEWLINE) != 0)
        return NULL;

    uint32_t lines = 0;//lyrics line count

    Lyricsline* lrcline = malloc(sizeof(Lyricsline) * MAX_LYRICSLINE);

    while(1){
        if(regexec(&preg,buffer, 5, pm, REG_NOTEOL) != REG_NOMATCH){

            char* m = getStringFromRegmatch(buffer,pm[1].rm_so,pm[1].rm_eo);
            char* s = getStringFromRegmatch(buffer,pm[2].rm_so,pm[2].rm_eo);
            char* ms = getStringFromRegmatch(buffer,pm[3].rm_so,pm[3].rm_eo);
            char* word = getStringFromRegmatch(buffer,pm[4].rm_so,pm[4].rm_eo);


            lrcline[lines].m = atol(m);
            lrcline[lines].s = atoi(s);
            lrcline[lines].ms = atoi(ms);
            lrcline[lines].word = word;
            lrcline[lines].totalms = (atol(m) * 60 + atoi(s)) * 1000 + atoi(ms) * 10;

            free(m);
            free(s);
            free(ms);

            lines++;
            buffer+=pm[0].rm_eo;
        }else{break;}
    }

    regfree(&preg);

    Lyrics* lyrics = malloc(sizeof(Lyrics));
    lyrics->lrclines = lrcline;
    lyrics->lyricscount = lines;

    return lyrics;
}

void lrcParseClose(Lyrics* lyrics)
{
    if(!lyrics)
        return;

    lyrics->lyricscount = 0;
    int i;
    for(i = 0;i < lyrics->lyricscount ; ++i){
        if(lyrics->lrclines[i].word){
            free(lyrics->lrclines[i].word);//free lyrics words
            lyrics->lrclines[i].word = NULL;
        }
    }

    if(lyrics->lrclines){
        free(lyrics->lrclines);//free lrclines struct
        lyrics->lrclines = NULL;
    }

    free(lyrics);//free lyrics
    lyrics = NULL;
}
