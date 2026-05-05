#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "themes.h"

#define CONFIG_LINE_SIZE 256
#define LITERAL_SIZE 128

void config_mismatch(char *key, char *possible_values)
{
	fprintf(stderr, "Configuration mismatch %s: possible values %s", key, possible_values);
}

Config config_load(const char *config_path, char *fallback_font_path)
{
	Config config = {
		.font = fallback_font_path,
		.font_size = 14,
		.message_timeout = 5,
		.tab_size = 8,
		.leading = 1,
		.theme_name = "jblow_nastalgia",
		.line_number_format = HIDE};

	FILE *config_file = fopen(config_path, "r");
	if (config_file == NULL) {
		fprintf(stderr, "Not found config file %s. Use default params\n", config_path);
		return config;
	}

	char line[CONFIG_LINE_SIZE];
	while (fgets(line, CONFIG_LINE_SIZE, config_file)) {
		char key[LITERAL_SIZE], value[LITERAL_SIZE];
		if (2 == sscanf(line, "%127s = %127s", key, value)) {
			if (0 == strcmp(key, CONFIG_FONT)) {
				config.font = strdup(value);
			} else if (0 == strcmp(key, CONFIG_FONT_SIZE)) {
				config.font_size = atoi(value);
			} else if (0 == strcmp(key, CONFIG_MESSAGE_TIMEOUT)) {
				config.message_timeout = atoi(value);
			} else if (0 == strcmp(key, CONFIG_TAB_SIZE)) {
			   config.tab_size = atoi(value);
			} else if (0 == strcmp(key, CONFIG_LEADING)) {
				config.leading = atoi(value);
			} else if (0 == strcmp(key, CONFIG_THEME)) {
				config.theme_name = strdup(value);
			} else if (0 == strcmp(key, CONFIG_LINE_NUMBER_FORMAT)) {
				if (0 == strcmp(value, CONFIG_LINE_NUMBER_FORMAT_RELATIVE)) config.line_number_format = RELATIVE;
				else if (0 == strcmp(value, CONFIG_LINE_NUMBER_FORMAT_ABSOLUTE)) config.line_number_format = ABSOLUTE;
				else if (0 == strcmp(value, CONFIG_LINE_NUMBER_FORMAT_HIDE)) config.line_number_format = HIDE;
				else config_mismatch(CONFIG_LINE_NUMBER_FORMAT, CONFIG_LINE_NUMBER_FORMAT_POSSIBLE_VALUES);
			}
		}
	}

	fclose(config_file);
	return config;
}

void config_apply_theme(Smacs *smacs, const char *theme_name)
{
	if (0 == strcmp(theme_name, "naysayer")) {
		themes_naysayer(smacs);
	} else if (0 == strcmp(theme_name, "mindre")) {
		themes_mindre(smacs);
	} else if (0 == strcmp(theme_name, "acme")) {
		themes_acme(smacs);
	} else if (0 == strcmp(theme_name, "jblow_nastalgia")) {
		themes_jblow_nastalgia(smacs);
	} else if (0 == strcmp(theme_name, "qemacs")) {
		themes_qemacs(smacs);
	} else {
		themes_jblow_nastalgia(smacs);
	}
}
