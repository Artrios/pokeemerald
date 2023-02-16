#ifndef GUARD_GTS_MENU_H
#define GUARD_GTS_MENU_H

u16 GetGTSBaseBlock(void);
void CB2_MysteryGiftEReader(void);
void PrintGTSTopMenu(bool8 isJapanese, bool32 usePickOkCancel);
void GTS_DrawCheckerboardPattern(u32 bg);
void MainCB_GTSFreeAllBuffersAndReturnToInitTitleScreen(void);
bool32 PrintGTSMenuMessage(u8 *textState, const u8 *str);
void GTSAddTextPrinterToWindow1(const u8 *src);
void CB2_InitEReader(void);
void CB2_InitGlobalTradeStation(void);
void GTS_DrawTextBorder(u8 windowId);
s8 DoGTSYesNo(u8 *textState, u16 *windowId, bool8 yesNoBoxPlacement, const u8 *str);

struct PokedexListItem
{
    u16 dexNum;
    u16 seen:1;
    u16 owned:1;
};

struct GTSPokedexView
{
    struct PokedexListItem pokedexList[NATIONAL_DEX_COUNT + 1];
    u16 pokemonListCount; //keep
    u16 selectedPokemon;
    u16 offerPokemon;
    u16 dexMode;
    u16 windowid;
    u16 dexOrder;
    u16 dexOrderBackup;
    u16 seenCount;
    u16 ownCount;
    u16 selectedMonSpriteId;
    u16 cursorRelPos;
    u8 atTop;
    u8 atBottom;
    u8 initialVOffset;
    u8 scrollTimer;
    u8 scrollDirection;
    s16 listVOffset;
    s16 listMovingVOffset;
    u16 scrollMonIncrement;
    u16 maxScrollTimer;
    u16 scrollSpeed;
    u8 currentPage; //keep
    u8 currentPageBackup;
    bool8 isSearchResults:1; //keep
    u8 selectedScreen;
    u8 screenSwitchState;
    u8 menuIsOpen;
    u16 menuCursorPos;
    s16 menuY;     //Menu Y position (inverted because we use REG_BG0VOFS for this)
};

//extern EWRAM_DATA struct GTSPokedexView *sGTSPokedexView;

#endif // GUARD_GTS_MENU_H