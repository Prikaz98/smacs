#ifndef CONFIG_H
#define CONFIG_H

#include "render.h"

typedef struct {
	char *font;
	int font_size;
	int message_timeout;
	int tab_size;
	int leading;
	char *theme_name;
	enum LineNumberFormat line_number_format;
} Config;

Config config_load(const char *config_path, char *fallback_font_path);
void config_apply_theme(Smacs *smacs, const char *theme_name);

#define CONFIG_FONT                "font"
#define CONFIG_FONT_DEFAULT        "unifont-16.0.04.ttf"

#define CONFIG_FONT_SIZE           "font_size"
#define CONFIG_MESSAGE_TIMEOUT     "message_timeout"
#define CONFIG_TAB_SIZE            "tab_size"
#define CONFIG_LEADING             "leading"
#define CONFIG_THEME               "theme"
#define CONFIG_LINE_NUMBER_FORMAT  "line_number_format"

#define CONFIG_LINE_NUMBER_FORMAT_RELATIVE  "relative"
#define CONFIG_LINE_NUMBER_FORMAT_ABSOLUTE  "absolute"
#define CONFIG_LINE_NUMBER_FORMAT_HIDE      "hide"
#define CONFIG_LINE_NUMBER_FORMAT_POSSIBLE_VALUES "[" \
	CONFIG_LINE_NUMBER_FORMAT_RELATIVE ", " \
	CONFIG_LINE_NUMBER_FORMAT_ABSOLUTE ", " \
	CONFIG_LINE_NUMBER_FORMAT_HIDE "]"

#endif
