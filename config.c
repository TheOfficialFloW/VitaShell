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
#include "config.h"

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
	uint8_t ch = 0;
	int n = 0;
	int i = 0;
	uint8_t *s = (uint8_t *)str;

	while (1) {
		if (i >= size)
			break;

		ch = ((uint8_t *)buf)[i];

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

int getDecimal(char *str) {
	return strtol(str, NULL, 0);
}

int getHexdecimal(char *str) {
	return strtoul(str, NULL, 16);
}

int getBoolean(char *str) {
	if (strcasecmp(str, "false") == 0 || strcasecmp(str, "off") == 0 || strcasecmp(str, "no") == 0)
		return 0;

	if (strcasecmp(str, "true") == 0 || strcasecmp(str, "on") == 0 || strcasecmp(str, "yes") == 0)
		return 1;

	return -1;
}

char *getString(char *str) {
	if (str[0] != '"')
		return NULL;

	char *p = strchr(str + 1, '"');
	if (!p)
		return NULL;

	int len = p - (str + 1);

	char *out = malloc(len + 1);
	strncpy(out, str + 1, len);
	out[len] = '\0';

	int i;
	for (i = 0; i < strlen(out); i++) {
		if (out[i] == '\\')
			out[i] = '\n';
	}

	return out;
}

int readEntry(char *line, ConfigEntry *entries, int n_entries) {
	// Trim at beginning
	while (*line == ' ' || *line == '\t')
		line++;

	// Ignore comments #1
	if (line[0] == '#') {
		// debugPrintf("IGNORE %s\n", line);
		return 0;
	}

	// Ignore comments #2
	char *p = strchr(line, '#');
	if (p) {
		// debugPrintf("IGNORE %s\n", p);
		*p = '\0';
	}

	// Get token
	p = strchr(line, '=');
	if (!p)
		return -1;

	// Name of entry
	char name[MAX_CONFIG_NAME_LENGTH];
	strncpy(name, line, p - line);
	name[p - line] = '\0';

	trim(name);

	// debugPrintf("NAME: %s\n", name);

	// String of entry
	char *string = p + 1;

	// Trim at beginning
	while (*string == ' ' || *string == '\t')
		string++;

	// Trim at end
	trim(string);

	// debugPrintf("STRING: %s\n", string);

	int i;
	for (i = 0; i < n_entries; i++) {
		if (strcasecmp(name, entries[i].name) == 0) {
			switch (entries[i].type) {
				case CONFIG_TYPE_DECIMAL:
					*(uint32_t *)entries[i].value = getDecimal(string);
					// debugPrintf("VALUE DECIMAL: %d\n", *(uint32_t *)entries[i].value);
					break;
					
				case CONFIG_TYPE_HEXDECIMAL:
					*(uint32_t *)entries[i].value = getHexdecimal(string);
					// debugPrintf("VALUE HEXDECIMAL: 0x%X\n", *(uint32_t *)entries[i].value);
					break;
					
				case CONFIG_TYPE_BOOLEAN:
					*(uint32_t *)entries[i].value = getBoolean(string);
					// debugPrintf("VALUE BOOLEAN: %d\n", *(uint32_t *)entries[i].value);
					break;
					
				case CONFIG_TYPE_STRING:
					*(uint32_t *)entries[i].value = (uint32_t)getString(string);
					// debugPrintf("VALUE STRING: %s\n", *(uint32_t *)entries[i].value);
					break;
			}

			break;
		}
	}

	return 1;
}

int readConfigBuffer(void *buffer, int size, ConfigEntry *entries, int n_entries) {
	int res = 0;
	char line[MAX_CONFIG_LINE_LENGTH];
	char *p = buffer;

	// Skip UTF-8 bom
	uint32_t bom = 0xBFBBEF;
	if (memcmp(p, &bom, 3) == 0) {
		p += 3;
		size -= 3;
	}

	do {
		memset(line, 0, sizeof(line));
		res = GetLine(p, size, line);

		if (res > 0) {
			readEntry(line, entries, n_entries);
			size -= res;
			p += res;
		}
	} while (res > 0);

	return 0;	
}

int readConfig(char *path, ConfigEntry *entries, int n_entries) {
	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;

	int size = sceIoLseek32(fd, 0, SCE_SEEK_END);
	sceIoLseek32(fd, 0, SCE_SEEK_SET);

	void *buffer = malloc(size);
	sceIoRead(fd, buffer, size);
	sceIoClose(fd);

	readConfigBuffer(buffer, size, entries, n_entries);

	free(buffer);

	return 0;
}