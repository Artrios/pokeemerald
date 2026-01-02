#ifndef GUARD_GTS_MENU_H
#define GUARD_GTS_MENU_H
#define MAX_MONS_ON_SCREEN 4

u16 GetGTSBaseBlock(void);
void CB2_MysteryGiftEReader(void);
void PrintGTSTopMenu(u8 menuScreen, bool32 usePickOkCancel);
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

struct GTSResult
{
    u32 checksum;               //0-3
    u32 pid;                    //4-7
    struct BoxPokemon boxmon;   //8-87
    u16 dexNum;                 //88-89
    u8 gender;                  //90
    u8 level;                   //91
    u16 natDexRequest;          //92-93
    u8 genderRequest;           //94
    u8 minLevel;                //95
    u8 maxLevel;                //96
    u8 trainerGender;           //97
    u16 trainerID;              //98-99
    u16 secretID;               //100-101
    char OTName[7];             //102-108
    u8 country;                 //109
    u8 region;                  //110
    u8 trainerClass;            //111
    bool8 isExchanged;          //112
    u16 gameVersion;            //113-114 +5
    u16 romHackID;              //115-116
    u16 romHackVersion;         //117-118
    u8 language;                //119
};

struct GTSPokedexView
{
    struct PokedexListItem pokedexList[NATIONAL_DEX_COUNT + 1];
    u16 pokemonListCount; //keep
    u16 selectedPokemon;
    u16 offerPokemon;
    s16 dexMode;
    u16 windowid;
    u16 dexOrder;
    u16 monSpriteIds[MAX_MONS_ON_SCREEN];
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
    u8 numResults;
    u8 currentPage; //keep
    u8 token[32];
    struct GTSResult searchResult[7];
};

struct GTSSearch
{
    u32 checksum;               //0-3
    u16 dexNum;                 //4-5
    u8 gender;                  //6
    u8 minLevel;                //7
    u8 maxLevel;                //8
};

//extern EWRAM_DATA struct GTSPokedexView *sGTSPokedexView;

#endif // GUARD_GTS_MENU_H