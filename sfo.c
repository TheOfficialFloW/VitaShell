#include "main.h"
#include "archive.h"
#include "file.h"
#include "text.h"
#include "hex.h"
#include "message_dialog.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "sfo.h"

int SFOReader(char* file)
{
	uint8_t *buffer = malloc(BIG_BUFFER_SIZE);
	if (!buffer)
		return -1;

	int size = 0;

	if (isInArchive()) {
		size = ReadArchiveFile(file, buffer, BIG_BUFFER_SIZE);
	} else {
		size = ReadFile(file, buffer, BIG_BUFFER_SIZE);
	}

	if (size <= 0) {
		free(buffer);
		return size;
	}

	sfo_header_t *sfo_header = (sfo_header_t*)buffer;
	if (sfo_header->magic != 0x46535000) {
    		return -1;
    	}

	int scroll_allow = sfo_header->indexTableEntries - MAX_ENTRIES;
	if (scroll_allow < 0) { scroll_allow = 0; }

	int line_show = sfo_header->indexTableEntries;
	if (line_show > MAX_ENTRIES) { line_show = MAX_ENTRIES; }

	int current_pos = 0;

	while (1)
	{
		readPad();

		if (pressed_buttons & SCE_CTRL_CANCEL)
		{
			break;	
		}

		if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP)
		{
			if (current_pos != 0)
			{
				current_pos--;
			}
		}
		else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN)
		{
			if (current_pos != scroll_allow)
			{
				current_pos++;
			}
		}

		// Start drawing
		startDrawing();

		// Draw shell info
		drawShellInfo(file);

	    	sfo_index_t *sfo_index;
	    	int i;

	    	// Draw scroll bar
	   	if (scroll_allow > 0) {
			drawScrollBar(current_pos, sfo_header->indexTableEntries);
		}

		for (i = 0; i < line_show; i++)
		{
			sfo_index = (sfo_index_t*)(buffer + sizeof(sfo_header_t) + (sizeof(sfo_index_t) * (i + current_pos)));

	    		char* key = (char*)buffer + sfo_header->keyTableOffset + sfo_index->keyOffset;
			pgf_draw_textf(SHELL_MARGIN_X, START_Y + (FONT_Y_SPACE * i), GENERAL_COLOR, FONT_SIZE, "%s", key);

			if (sfo_index->param_fmt == 1028)
			{
				unsigned int* value = (unsigned int*)buffer + sfo_header->dataTableOffset + sfo_index->dataOffset;
				pgf_draw_textf(SHELL_MARGIN_X + 450, START_Y + (FONT_Y_SPACE * i), GENERAL_COLOR, FONT_SIZE, "%d", *value);
			}
			else
			{
				char* value = (char*)buffer + sfo_header->dataTableOffset + sfo_index->dataOffset;
				pgf_draw_textf(SHELL_MARGIN_X + 450, START_Y + (FONT_Y_SPACE * i), GENERAL_COLOR, FONT_SIZE, "%s", value);
			}
		}

		// End drawing
		endDrawing();
	}

	free(buffer);
	return 0;
}
