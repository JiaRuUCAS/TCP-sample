#ifndef __UNITS_H__
#define __UNITS_H__

enum {
	UNIT_BYTE = 0,
	UNIT_KILO,
	UNIT_MEGA,
	UNIT_GIGA,
	UNIT_TERA,
	UNIT_MAX,
};

struct unit {
	/* Symbol */
	char symbol;
	/* Suffix */
	char *suffix;
	/* shift bits */
	uint8_t shift;
};

uint8_t unit_match(char symbol);

/*
 * Given a string of form #x where # is an unsigned integer and x is a
 * format character, this returns the interpreted integer.
 */
uint64_t unit_atolu(const char *s);

/*
 * Given a number in bits and a format, converts the number and prints it
 * out with a bits label.
 */
void unit_snprintf(char *s, int len, uint64_t bits, uint8_t uid);

#endif
