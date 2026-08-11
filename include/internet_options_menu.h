#ifndef GUARD_InternetOptions_MENU_H
#define GUARD_InternetOptions_MENU_H

void CB2_InitInternetOptions(void);
void InternetOptions_DrawCheckerboardPattern(u32 bg);

void PrintInternetOptionsTopMenu(bool32 connecting);
void pid_to_fc(u32 pid, u8 *hexdigest);
void concat_str(char *dest, const char *src);
int sha1digest(uint8_t *digest, u8 *hexdigest, const uint8_t *data, size_t databytes);

//extern EWRAM_DATA struct GTSPokedexView *sGTSPokedexView;

#endif // GUARD_InternetOptions_MENU_H