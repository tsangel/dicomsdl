/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * codec_keys.h
 */

// built using codec_keys_h__builder.py

#define ARGKEY_RATE 1 /* rate */
#define ARGKEY_STEP 2 /* step */
#define ARGKEY_PRECISE 3 /* precise */
#define ARGKEY_LAYER 4 /* layer */
#define ARGKEY_LEVEL 5 /* level */
#define ARGKEY_REVERSIBLE 6 /* reversible */
#define ARGKEY_MODE 7 /* mode */
#define ARGKEY_QUALITY 8 /* quality */

static int __stricmp(const char *a, const char *b)
{
    for (;*a && *b;a++,b++) if (tolower(*a) != tolower(*b)) break;
    return (*a?1:0); 
}
static int get_argkey(char *arg) {
	char *c = arg;
	int key=0;
	switch (*c++) {
		case 'm': case 'M': key=ARGKEY_MODE; goto L_EXIT; break;
		case 'l': case 'L': {
			switch (*c++) {
				case 'a': case 'A': key=ARGKEY_LAYER; goto L_EXIT; break;
				case 'e': case 'E': key=ARGKEY_LEVEL; goto L_EXIT; break;
				default: goto L_EXIT; break;
			};
		}; break;
		case 'q': case 'Q': key=ARGKEY_QUALITY; goto L_EXIT; break;
		case 'p': case 'P': key=ARGKEY_PRECISE; goto L_EXIT; break;
		case 's': case 'S': key=ARGKEY_STEP; goto L_EXIT; break;
		case 'r': case 'R': {
			switch (*c++) {
				case 'a': case 'A': key=ARGKEY_RATE; goto L_EXIT; break;
				case 'e': case 'E': key=ARGKEY_REVERSIBLE; goto L_EXIT; break;
				default: goto L_EXIT; break;
			};
		}; break;
		default: goto L_EXIT; break;
	};
L_EXIT:
	switch (key) {
		case ARGKEY_RATE: if (__stricmp(arg, "rate")) return 0; break;
		case ARGKEY_STEP: if (__stricmp(arg, "step")) return 0; break;
		case ARGKEY_PRECISE: if (__stricmp(arg, "precise")) return 0; break;
		case ARGKEY_LAYER: if (__stricmp(arg, "layer")) return 0; break;
		case ARGKEY_LEVEL: if (__stricmp(arg, "level")) return 0; break;
		case ARGKEY_REVERSIBLE: if (__stricmp(arg, "reversible")) return 0; break;
		case ARGKEY_MODE: if (__stricmp(arg, "mode")) return 0; break;
		case ARGKEY_QUALITY: if (__stricmp(arg, "quality")) return 0; break;
		default: break;
	}
	return key;
};

