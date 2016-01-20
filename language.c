/*
	VitaShell
	Copyright (C) 2015-2016, TheFloW

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
#include "language.h"

#include "resources/english_us_translation.h"

static char *lang[] ={
	"japanese",
	"english_us",
	"french",
	"spanish",
	"german",
	"italian",
	"dutch",
	"portuguese",
	"russian",
	"korean",
	"chinese_t",
	"chinese_s",
	"finnish",
	"swedish",
	"danish",
	"norwegian",
	"polish",
	"portuguese_br",
	"english_gb"
};

char *language_container[LANGUAGE_CONTRAINER_SIZE];

void trim(char *str) {
	int len = strlen(str);
	int i;

	for (i = len - 1; i >= 0; i--) {
		if (str[i] == 0x20 || str[i] == '\t') {
			str[i] = 0;
		} else {
			break;
		}
	}
}

int GetLine(char *buf, int size, char *str) {
	unsigned char ch = 0;
	int n = 0;
	int i = 0;
	unsigned char *s = (unsigned char *)str;

	while (1) {
		if (i >= size)
			break;

		ch = ((unsigned char *)buf)[i];

		if (ch < 0x20 && ch != '\t') {
			if (n != 0) {
				i++;
				break;
			}
		} else {
			*str++ = ch;
			n++;
		}

		i++;
	}

	trim((char *)s);

	return i;
}

void freeLanguageContainer() {
	int i;
	for (i = 0; i < LANGUAGE_CONTRAINER_SIZE; i++) {
		if (language_container[i]) {
			free(language_container[i]);
			language_container[i] = NULL;
		}
	}
}

int loadLanguageContainer(void *buffer, int size) {
	int res;
	int i = 0;
	char *p = buffer;
	char line[512];

	freeLanguageContainer();

	do {
		memset(line, 0, sizeof(line));

		if ((res = GetLine(p, size, line)) > 0) {
			int j;
			for (j = 0; j < strlen(line); j++) {
				if (line[j] == '\\')
					line[j] = '\n';
			}

			language_container[i] = malloc(strlen(line) + 1);
			strcpy(language_container[i], line);
			i++;
		}
 
		size -= res;
		p += res;
	} while (res > 0 && i < LANGUAGE_CONTRAINER_SIZE);

	return i == LANGUAGE_CONTRAINER_SIZE;
}

void loadLanguage(int id) {
	int loaded = 0;

	if (id >= 0 && id < (sizeof(lang) / sizeof(char *))) {
		char path[128];
		sprintf(path, "cache0:VitaShell/%s_translation.txt", lang[id]);

		SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
		if (fd >= 0) {
			int size = sceIoLseek(fd, 0, SCE_SEEK_END);
			sceIoLseek(fd, 0, SCE_SEEK_SET);

			void *buffer = malloc(size);
			sceIoRead(fd, buffer, size);

			loaded = loadLanguageContainer(buffer, size);

			free(buffer);
		}
	}

	if (!loaded)
		loadLanguageContainer(english_us_translation, size_english_us_translation);
}