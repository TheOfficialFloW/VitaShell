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
#include "file.h"
#include "module.h"
#include "shaders.h"

static SceShaccCgSourceFile input;

int logPrintf(char *path, char *text, ...) {
	va_list list;
	char string[512];

	va_start(list, text);
	vsprintf(string, text, list);
	va_end(list);

	SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
	if (fd >= 0) {
		sceIoWrite(fd, string, strlen(string));
		sceIoClose(fd);
	}

	return 0;
}

void *getInput() {
	return &input;
}

SceShaccCgCompileOutput *compileShaderType(int type) {
	int (* _sceShaccCgInitializeCompileOptions)(SceShaccCgCompileOptions options);
	SceShaccCgCompileOutput *(* _sceShaccCgCompile)(const SceShaccCgCompileOptions *options, const shader_first *shader_first, void *callbacks);

	_sceShaccCgInitializeCompileOptions = (void *)findModuleExportByName("SceShaccCg", "SceShaccCg", 0x3B58AFA0);
	_sceShaccCgCompile = (void *)findModuleExportByName("SceShaccCg", "SceShaccCg", 0x66814F35);
	if (!_sceShaccCgInitializeCompileOptions || !_sceShaccCgCompile)
		return NULL;

	SceShaccCgCompileOptions second;
	memset(&second, 0, sizeof(SceShaccCgCompileOptions));
	_sceShaccCgInitializeCompileOptions(second);	

	shader_first first;
	memset(&first, 0, sizeof(shader_first));
	first.field_0 = (int)getInput;

	second.mainSourceFile = input.fileName;
	second.targetProfile = type;
	second.entryFunctionName = "main";
	second.macroDefinitionCount = 1;
	const char *str = "SHADER_API_PSM";
	second.macroDefinitions = &str;
	second.useFx = 1;
	second.optimizationLevel = 3;
	second.useFastmath = 1;
	second.useFastint = 0;
	second.performanceWarnings = 0;
	second.warningLevel = 1;
	second.pedantic = 3;

	return _sceShaccCgCompile(&second, &first, 0);
}

int compileShader(char *file, int *infos, int *warnings, int *errors) {
	int type = SCE_SHACCCG_PROFILE_VP;

	// Get type by extension
	int length = strlen(file);
	if (strcasecmp(file + length - 5, "_v.cg") == 0) {
		type = SCE_SHACCCG_PROFILE_VP;
	} else if (strcasecmp(file + length - 5, "_f.cg") == 0) {
		type = SCE_SHACCCG_PROFILE_FP;
	} else {
		return -1;
	}

	char *buffer = malloc(BIG_BUFFER_SIZE);
	if (!buffer)
		return -2;

	int size = ReadFile(file, buffer, BIG_BUFFER_SIZE);
	if (size <= 0) {
		free(buffer);
		return size;
	}

	// Compile
	input.fileName = "<built-in>";
	input.program = buffer;
	input.size = size;
	const SceShaccCgCompileOutput *r = compileShaderType(type);
	if (!r) {
		free(buffer);
		return -3;
	}

	// Print infos/warnings/errors
	if (r->diagnosticCount > 0) {
		char path[MAX_PATH_LENGTH];
		strcpy(path, file);
		strcpy(path + length - 3, ".log");

		sceIoRemove(path);

		logPrintf(path, "%s FILE %s\n", (type == SCE_SHACCCG_PROFILE_VP) ? "VERTEX" : "FRAGMENT", file);

		int i;
		for (i = 0; i < r->diagnosticCount; i++) {
			const SceShaccCgDiagnosticMessage *e = &r->diagnostics[i];

			uint32_t x = 0, y = 0;
			if (e->location) {
				x = e->location->lineNumber;
				y = e->location->columnNumber;
			}

			const char *message = e->message;

			switch (e->level) {
				case SCE_SHACCCG_DIAGNOSTIC_LEVEL_INFO:
					logPrintf(path, "INFO (%lu, %lu): %s", x, y, message);
					if (infos)
						(*infos)++;
					break;
					
				case SCE_SHACCCG_DIAGNOSTIC_LEVEL_WARNING:
					logPrintf(path, "WARN (%lu, %lu): %s", x, y, message);
					if (warnings)
						(*warnings)++;
					break;
					
				case SCE_SHACCCG_DIAGNOSTIC_LEVEL_ERROR:
					logPrintf(path, "ERROR (%lu, %lu): %s", x, y, message);
					if (errors)
						(*errors)++;
					break;
			}
		}
	}

	// Couldn't be compiled
	if (!r->programData) {
		free(buffer);
		return 0;
	}

	// Write compiled output
	char path[MAX_PATH_LENGTH];
	strcpy(path, file);
	strcpy(path + length - 3, ".gxp");

	int res = WriteFile(path, (void *)r->programData, r->programSize);
	if (res < 0) {
		free(buffer);
		return res;
	}

	free(buffer);
	return 0;
}