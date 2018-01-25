/*
  VitaShell
  Copyright (C) 2015-2018, TheFloW

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
#include "audioplayer.h"
#include "file.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "audio/lrcparse.h"

#include "audio/player.h"

static struct fileInfo *fileinfo = NULL;
static vita2d_texture *tex = NULL;


/**
* Calculate the x-axis position if draw text in center
* @param[in] sx start x-axis
* @param[in] ex end x-axis
* @param[in] text
* @return x-axis position
*/
float getCenteroffset(float sx,float ex,char* string){
  if(!string||(string[0] == '\0'))
    return sx;

  float drawWidthSpace = ex - sx;
  uint16_t stringWidth = pgf_text_width(string);
  return stringWidth > drawWidthSpace ? sx : sx + (drawWidthSpace - stringWidth) / 2 ;
}

/**
* Try to load lrc file from audio path
* @param[in] path audio path
* @param[out] totalms set 0
* @param[out] lyricsIndex set 0
* @return Lyrics pointer , NULL is fail
*/
Lyrics* loadLyricsFile(const char *path, uint64_t *totalms, uint32_t *lyricsIndex){
  size_t pathlength = strlen(path);
  *totalms = *lyricsIndex = 0;

  while(pathlength > 0){
    if(path[pathlength] == '.'){
      break;
    }
    pathlength--;
  }

  if(pathlength < 0)
    return NULL;

  char lrcPath[pathlength + 5 * sizeof(char)];
  memccpy(lrcPath,path,sizeof(char),pathlength);//copy path string except filename extension
  strcpy(lrcPath+pathlength,".lrc");

  return lrcParseLoadWithFile(lrcPath);
}

/**
* Draw the lyrics from the designated area
* @param[in] lyrics Lyrics pointer
* @param[in] cur_time_string Playing time string
* @param[out] totalms Playing time (millisecond)
* @param[out] lyricsIndex Index of lyrics
* @param[in] lrcSpaceX Designated area starting point x
* @param[in] lrcSpaceX Designated area starting point y
*/
void drawLyrics(Lyrics* lyrics, const char *cur_time_string, uint64_t* totalms, uint32_t* lyricsIndex, float lrcSpaceX, float lrcSpaceY){
  if(!lyrics)
    return;

  char hourString[3];
  char minuteString[3];
  char secondString[3];

  strncpy(hourString,cur_time_string,sizeof(hourString));
  strncpy(minuteString,cur_time_string + 3,sizeof(minuteString));
  strncpy(secondString,cur_time_string + 6,sizeof(secondString));

  *totalms = (((atoi(hourString) * 60) + atoi(minuteString)) * 60 + atoi(secondString)) * 1000;

  uint32_t m_index = *lyricsIndex >= 1 ? (*lyricsIndex - 1) : *lyricsIndex;
  float right_max_x = SCREEN_WIDTH - SHELL_MARGIN_X;
  //draw current lyrics
  pgf_draw_textf(getCenteroffset(lrcSpaceX,right_max_x,lyrics->lrclines[m_index].word),lrcSpaceY, AUDIO_INFO_ASSIGN, "%s",lyrics->lrclines[m_index].word);

  int i;
  for (i = 1;i < 7; i++){//draw 6 line lyrics for preview
    int n_index = m_index + i;
    if(n_index + 1 > lyrics->lyricscount)
      break;
    pgf_draw_textf(getCenteroffset(lrcSpaceX,right_max_x,lyrics->lrclines[n_index].word),lrcSpaceY + FONT_Y_SPACE * i, AUDIO_INFO, "%s",lyrics->lrclines[n_index].word);
  }

  if (*totalms >= (lyrics->lrclines[*lyricsIndex].totalms) ){
    *lyricsIndex = *lyricsIndex + 1;
  } else if (( *lyricsIndex >= 1 ) & ( *totalms < (lyrics->lrclines[*lyricsIndex - 1].totalms ))) {
    *lyricsIndex = *lyricsIndex - 1;
  }
}

void shortenString(char *out, const char *in, int width) {
  strcpy(out, in);

  int i;
  for (i = strlen(out)-1; i > 0; i--) {
    if (pgf_text_width(out) < width)
      break;

    out[i] = '\0';
  }
}

vita2d_texture *getAlternativeCoverImage(const char *file) {
  char path[MAX_PATH_LENGTH];

  char *p = strrchr(file, '/');
  if (p) {
    *p = '\0';

    snprintf(path, MAX_PATH_LENGTH - 1, "%s/cover.jpg", file);
    if (checkFileExist(path)) {
      *p = '/';
      return vita2d_load_JPEG_file(path);
    }

    snprintf(path, MAX_PATH_LENGTH - 1, "%s/folder.jpg", file);
    if (checkFileExist(path)) {
      *p = '/';
      return vita2d_load_JPEG_file(path);
    }

    *p = '/';
  }

  return NULL;
}

void getAudioInfo(const char *file) {
  char *buffer = NULL;

  fileinfo = getInfoFunct();

  if (tex) {
    vita2d_wait_rendering_done();
    vita2d_free_texture(tex);
    tex = NULL;
  }

  switch (fileinfo->encapsulatedPictureType) {
    case JPEG_IMAGE:
    case PNG_IMAGE:
    {
      SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
      if (fd >= 0) {
        char *buffer = malloc(fileinfo->encapsulatedPictureLength);
        if (buffer) {
          sceIoLseek32(fd, fileinfo->encapsulatedPictureOffset, SCE_SEEK_SET);
          sceIoRead(fd, buffer, fileinfo->encapsulatedPictureLength);
          sceIoClose(fd);

          if (fileinfo->encapsulatedPictureType == JPEG_IMAGE)
            tex = vita2d_load_JPEG_buffer(buffer, fileinfo->encapsulatedPictureLength);

          if (fileinfo->encapsulatedPictureType == PNG_IMAGE)
            tex = vita2d_load_PNG_buffer(buffer);

          if (tex)
            vita2d_texture_set_filters(tex, SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR);

          free(buffer);
        }

        break;
      }
    }
  }

  if (!tex)
    tex = getAlternativeCoverImage(file);
}

int audioPlayer(const char *file, int type, FileList *list, FileListEntry *entry, int *base_pos, int *rel_pos) {
  static int speed_list[] = { -7, -3, -1, 0, 1, 3, 7 };
  #define N_SPEED (sizeof(speed_list) / sizeof(int))

  sceAppMgrAcquireBgmPort();

  powerLock();

  setAudioFunctions(type);

  initFunct(0);
  loadFunct((char *)file);
  playFunct();

  getAudioInfo(file);

  uint64_t totalms = 0;
  uint32_t lyricsIndex = 0;
  Lyrics* lyrics = loadLyricsFile(file,&totalms,&lyricsIndex);

  int scroll_count = 0;
  float scroll_x = 0.0f;

  while (1) {
    char cur_time_string[12];
    getTimeStringFunct(cur_time_string);

    readPad();

    // Cancel
    if (pressed_pad[PAD_CANCEL]) {
      break;
    }

    // Display off
    if (pressed_pad[PAD_TRIANGLE]) {
      scePowerRequestDisplayOff();
    }

    // Toggle play/pause
    if (pressed_pad[PAD_ENTER]) {
      if (isPlayingFunct() && getPlayingSpeedFunct() == 0) {
        pauseFunct();
      } else {
        setPlayingSpeedFunct(0);
        playFunct();
      }
    }

    if (pressed_pad[PAD_LEFT] || pressed_pad[PAD_RIGHT]) {
      int speed = getPlayingSpeedFunct();

      if (pressed_pad[PAD_LEFT]) {
        int i;
        for (i = 0; i < N_SPEED; i++) {
          if (speed_list[i] == speed) {
            if (i > 0)
              speed = speed_list[i-1];
            break;
          }
        }
      }

      if (pressed_pad[PAD_RIGHT]) {
        int i;
        for (i = 0; i < N_SPEED; i++) {
          if (speed_list[i] == speed) {
            if (i < N_SPEED - 1)
              speed = speed_list[i + 1];
            break;
          }
        }
      }

      setPlayingSpeedFunct(speed);

      playFunct();
    }

    // Previous/next song.
    if (getPercentageFunct() == 100.0f || endOfStreamFunct() ||
      pressed_pad[PAD_LTRIGGER] || pressed_pad[PAD_RTRIGGER]) {
      int previous = pressed_pad[PAD_LTRIGGER];
      if (previous && strcmp(cur_time_string, "00:00:00") != 0) {
        lrcParseClose(lyrics);
        endFunct();
        initFunct(0);
        loadFunct((char *)file);
        playFunct();

        getAudioInfo(file);

        lyrics = loadLyricsFile(file,&totalms,&lyricsIndex);

      } else {
        int available = 0;

        int old_base_pos = *base_pos;
        int old_rel_pos = *rel_pos;
        FileListEntry *old_entry = entry;

        if (getPercentageFunct() == 100.0f && !endOfStreamFunct())
          previous = 1;

        if (endOfStreamFunct())
          previous = 0;

        while (previous ? entry->previous : entry->next) {
          entry = previous ? entry->previous : entry->next;

          if (previous) {
            if (*rel_pos > 0) {
              (*rel_pos)--;
            } else if (*base_pos > 0) {
              (*base_pos)--;
            }
          } else {
            if ((*rel_pos + 1) < list->length) {
              if ((*rel_pos + 1) < MAX_POSITION) {
                (*rel_pos)++;
              } else if ((*base_pos+*rel_pos + 1) < list->length) {
                (*base_pos)++;
              }
            }
          }

          if (!entry->is_folder) {
            char path[MAX_PATH_LENGTH];
            snprintf(path, MAX_PATH_LENGTH - 1, "%s%s", list->path, entry->name);
            int type = getFileType(path);
            if (type == FILE_TYPE_MP3 || type == FILE_TYPE_OGG) {
              file = path;

              lrcParseClose(lyrics);
              endFunct();

              setAudioFunctions(type);

              initFunct(0);
              loadFunct((char *)file);
              playFunct();

              getAudioInfo(file);

              lyrics = loadLyricsFile(file,&totalms,&lyricsIndex);

              available = 1;
              break;
            }
          }
        }

        if (!available) {
          *base_pos = old_base_pos;
          *rel_pos = old_rel_pos;
          entry = old_entry;
          break;
        }
      }
    }

    // Start drawing
    startDrawing(bg_audio_image);

    // Draw shell info
    drawShellInfo(file);

    float cover_size = MAX_ENTRIES * FONT_Y_SPACE;

    // Cover
    if (tex) {
      vita2d_draw_texture_scale(tex, SHELL_MARGIN_X, START_Y, cover_size / vita2d_texture_get_width(tex), cover_size / vita2d_texture_get_height(tex));
    } else {
      vita2d_draw_texture(cover_image, SHELL_MARGIN_X, START_Y);
    }

    // Info
    float x = 2.0f * SHELL_MARGIN_X + cover_size;

    pgf_draw_text(x, START_Y + (0 * FONT_Y_SPACE), AUDIO_INFO_ASSIGN, language_container[TITLE]);
    pgf_draw_text(x, START_Y + (1 * FONT_Y_SPACE), AUDIO_INFO_ASSIGN, language_container[ALBUM]);
    pgf_draw_text(x, START_Y + (2 * FONT_Y_SPACE), AUDIO_INFO_ASSIGN, language_container[ARTIST]);
    pgf_draw_text(x, START_Y + (3 * FONT_Y_SPACE), AUDIO_INFO_ASSIGN, language_container[GENRE]);
    pgf_draw_text(x, START_Y + (4 * FONT_Y_SPACE), AUDIO_INFO_ASSIGN, language_container[YEAR]);

    x += 120.0f;

    vita2d_enable_clipping();
    vita2d_set_clip_rectangle(x + 1.0f, START_Y, x + 1.0f + 390.0f, START_Y + (5 * FONT_Y_SPACE));

    float title_x = x;
    uint32_t color = AUDIO_INFO;

    int width = (int)pgf_text_width(fileinfo->title);
    if (width >= 390.0f) {
      if (scroll_count < 60) {
        scroll_x = title_x;
      } else if (scroll_count < width + 90) {
        scroll_x--;
      } else if (scroll_count < width + 120) {
        color = (color & 0x00FFFFFF) | ((((color >> 24) * (scroll_count - width - 90)) / 30) << 24); // fade-in in 0.5s
        scroll_x = title_x;
      } else {
        scroll_count = 0;
      }
      
      scroll_count++;
      
      title_x = scroll_x;
    }

    pgf_draw_text(title_x, START_Y + (0 * FONT_Y_SPACE), color, fileinfo->title[0] == '\0' ? "-" : fileinfo->title);    
    pgf_draw_text(x, START_Y + (1 * FONT_Y_SPACE), AUDIO_INFO, fileinfo->album[0] == '\0' ? "-" : fileinfo->album);
    pgf_draw_text(x, START_Y + (2 * FONT_Y_SPACE), AUDIO_INFO, fileinfo->artist[0] == '\0' ? "-" : fileinfo->artist);
    pgf_draw_text(x, START_Y + (3 * FONT_Y_SPACE), AUDIO_INFO, fileinfo->genre[0] == '\0' ? "-" : fileinfo->genre);
    pgf_draw_text(x, START_Y + (4 * FONT_Y_SPACE), AUDIO_INFO, fileinfo->year[0] == '\0' ? "-" : fileinfo->year);

    vita2d_disable_clipping();

    x -= 120.0f;

    drawLyrics(lyrics, cur_time_string, &totalms, &lyricsIndex, x, START_Y + (6 * FONT_Y_SPACE));

    float y = SCREEN_HEIGHT - 6.0f * SHELL_MARGIN_Y;

    // Icon
    vita2d_texture *icon = NULL;

    if (getPlayingSpeedFunct() != 0) {
      if (getPlayingSpeedFunct() < 0) {
        icon = fastrewind_image;
      } else {
        icon = fastforward_image;
      }

      pgf_draw_textf(x + 45.0f, y, AUDIO_SPEED, "%dx", abs(getPlayingSpeedFunct() + (getPlayingSpeedFunct() < 0 ? -1 : 1)));
    } else {
      if (isPlayingFunct()) {
        icon = pause_image;
      } else {
        icon = play_image;
      }
    }

    vita2d_draw_texture(icon, x, y + 3.0f);

    // Time
    char string[32];
    sprintf(string, "%s / %s", cur_time_string, fileinfo->strLength);
    float time_x = ALIGN_RIGHT(SCREEN_WIDTH-SHELL_MARGIN_X, pgf_text_width(string));

    int w = pgf_draw_text(time_x, y, AUDIO_TIME_CURRENT, cur_time_string);
    pgf_draw_text(time_x + (float)w, y, AUDIO_TIME_SLASH, " /");
    pgf_draw_text(ALIGN_RIGHT(SCREEN_WIDTH-SHELL_MARGIN_X, pgf_text_width(fileinfo->strLength)), y, AUDIO_TIME_TOTAL, fileinfo->strLength);

    float length = SCREEN_WIDTH - 3.0f * SHELL_MARGIN_X - cover_size;
    vita2d_draw_rectangle(x, (y) + FONT_Y_SPACE + 10.0f, length, 8, AUDIO_TIME_BAR_BG);
    vita2d_draw_rectangle(x, (y) + FONT_Y_SPACE + 10.0f, getPercentageFunct() * length / 100.0f, 8, AUDIO_TIME_BAR);

    // End drawing
    endDrawing();
  }

  if (tex) {
    vita2d_wait_rendering_done();
    vita2d_free_texture(tex);
    tex = NULL;
  }

  lrcParseClose(lyrics);
  endFunct();

  powerUnlock();

  sceAppMgrReleaseBgmPort();

  return 0;
}
