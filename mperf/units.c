#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include "units.h"

static struct unit unit_list[UNIT_MAX] = {
	[UNIT_BYTE] = {	.symbol = 'b', .suffix = "bits",	.shift = 0,	},
	[UNIT_KILO] = {	.symbol = 'k', .suffix = "Kbits",	.shift = 10,},
	[UNIT_MEGA] = {	.symbol = 'm', .suffix = "Mbits",	.shift = 20,},
	[UNIT_GIGA] = {	.symbol = 'g', .suffix = "Gbits",	.shift = 30,},
	[UNIT_TERA] = {	.symbol = 't', .suffix = "Tbits",	.shift = 40,},
};

uint8_t
unit_match(char s)
{
	uint8_t i = 0;
	char lower;

	lower = tolower(s);

	for (i = 0; i < UNIT_MAX; i++) {
		if (lower == unit_list[i].symbol)
			break;
	}

	if (i < UNIT_MAX)
		return i;
	return UNIT_MAX;
}

/*
 * Given a string of form #x where # is an unsigned integer and x is a
 * format character, this returns the interpreted integer.
 */
uint64_t unit_atolu(const char *s)
{
	uint64_t num;
	char suffix = '\0';
	uint8_t uid = UNIT_MAX;

	assert(s != NULL);

	/* scan the number and any suffices */
	sscanf(s, "%lu%c", &num, &suffix);

	/* get unit */
	uid = unit_match(suffix);
	if (uid == UNIT_MAX)
		return UINT64_MAX;

	return (num << unit_list[uid].shift);
}

/*
 * Given a number in bytes and a format, converts the number and prints it
 * out with a bits label.
 */
void unit_snprintf(char *s, int len, uint64_t bits, uint8_t uid)
{
	double divisor = 0, format_num = (double)bits;
	struct unit *unit = &unit_list[uid];

	divisor = (double)(1ull << unit->shift);
	format_num = bits / divisor;

	snprintf(s, len, "%.3lf %s", format_num, unit->suffix);
}
