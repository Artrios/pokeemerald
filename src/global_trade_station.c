#include "global.h"
#include "main.h"
#include "text.h"
#include "task.h"
#include "data.h"
#include "malloc.h"
#include "gpu_regs.h"
#include "scanline_effect.h"
#include "text_window.h"
#include "bg.h"
#include "window.h"
#include "strings.h"
#include "text_window.h"
#include "menu.h"
#include "palette.h"
#include "constants/songs.h"
#include "sound.h"
#include "global_trade_station.h"
#include "union_room.h"
#include "title_screen.h"
#include "ereader_screen.h"
#include "international_string_util.h"
#include "list_menu.h"
#include "string_util.h"
#include "mystery_gift.h"
#include "mystery_gift_view.h"
#include "save.h"
#include "link.h"
#include "mystery_gift_client.h"
#include "mystery_gift_server.h"
#include "event_data.h"
#include "link_rfu.h"
#include "overworld.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "pokemon_summary_screen.h"
#include "wonder_news.h"
#include "constants/cable_club.h"
#include "constants/expansion.h"
#include "constants/party_menu.h"
#include "field_weather.h"
#include "constants/rgb.h"
#include "field_screen_effect.h"
#include "trade.h"
#include "trainer_pokemon_sprites.h"
#include "trig.h"
#include "mobile_adapter.h"
#include "internet_options_menu.h"
#include "load_save.h"
#include "item.h"
#include "chooseboxmon.h"

#define LIST_MENU_TILE_NUM 10
#define LIST_MENU_PAL_NUM 224
#define TAG_SCROLL_ARROW 5112

#define LETTER_IN_RANGE_UPPER(letter, range) \
    ((letter) >= sLetterSearchRanges[range][0]                                  \
  && (letter) < sLetterSearchRanges[range][0] + sLetterSearchRanges[range][1])  \

#define LETTER_IN_RANGE_LOWER(letter, range) \
    ((letter) >= sLetterSearchRanges[range][2]                                  \
  && (letter) < sLetterSearchRanges[range][2] + sLetterSearchRanges[range][3])  \


static void LoadMysteryGiftTextboxBorder(u8 bgId);
static void CreateGlobalTradeStationTask(void);
//static void RecreateGlobalTradeStationTask(void);
static void Task_GlobalTradeStation(u8 taskId);
static void CreatePokedexListGTS();
static void ClearMonListEntryGTS(u8 x, u8 y);
static u16 GetNextPositionGTS(u8, u16, u16, u16);
void GTSAddWantedToWindow1(u8 selectedMon);

EWRAM_DATA static u8 sDownArrowCounterAndYCoordIdx[8] = {};
EWRAM_DATA struct GTSPokedexView *sGTSPokedexView = NULL;

static const u16 sTextboxBorder_Pal[] = INCBIN_U16("graphics/interface/mystery_gift_textbox_border.gbapal");
static const u32 sTextboxBorder_Gfx[] = INCBIN_U32("graphics/interface/mystery_gift_textbox_border.4bpp.lz");

// const rom data
//#include "data/pokemon/pokedex_orders.h"
extern const u16 gPokedexOrder_Alphabetical[];
extern EWRAM_DATA u8 sSelectionType;

struct GlobalTradeStationTaskData
{
    u16 var; // Multipurpose
    u16 depositPokemon;
    u16 searchPokemon;
    u16 errorNum;
    u8 state;
    u8 textState;
    u8 nextState;
    u8 monDeposited;
    bool8 isWonderNews;
    bool8 sourceIsFriend;
    u8 msgId;
    u8 * clientMsg;
};

static const struct BgTemplate sBGTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 15,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0x000
    }, {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 14,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0x000
    }, {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 13,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0x000
    }, {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 12,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0x000
    }
};

static const struct WindowTemplate sMainWindows[] = {
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = 12,
        .baseBlock = 0x0013
    }, {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 15,
        .width = 28,
        .height = 4,
        .paletteNum = 12,
        .baseBlock = 0x004f
    }, {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 15,
        .width = 30,
        .height = 5,
        .paletteNum = 13,
        .baseBlock = 0x004f
    }, {
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 2,
        .width = 12,
        .height = 12,
        .paletteNum = 12,
        .baseBlock = 0x00e5
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sWindowTemplate_YesNoMsg_Wide = {
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 15,
    .width = 28,
    .height = 4,
    .paletteNum = 12,
    .baseBlock = 0x00e5
};

static const struct WindowTemplate sWindowTemplate_YesNoMsg = {
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 15,
    .width = 20,
    .height = 4,
    .paletteNum = 12,
    .baseBlock = 0x00e5
};

static const struct WindowTemplate sWindowTemplate_GiftSelect = {
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 15,
    .width = 19,
    .height = 4,
    .paletteNum = 12,
    .baseBlock = 0x00e5
};

static const struct WindowTemplate sWindowTemplate_ABCSelect = {
    .bg = 0,
    .tilemapLeft = 13,
    .tilemapTop = 3,
    .width = 4,
    .height = 10,
    .paletteNum = 12,
    .baseBlock = 0x00e5
};

static const struct WindowTemplate sWindowTemplate_LevelSelect = {
    .bg = 0,
    .tilemapLeft = 17,
    .tilemapTop = 3,
    .width = 12,
    .height = 10,
    .paletteNum = 12,
    .baseBlock = 0x00e5
};

static const struct WindowTemplate sWindowTemplate_PokemonSelect = {
    .bg = 0,
    .tilemapLeft = 19,
    .tilemapTop = 3,
    .width = 10,
    .height = 10,
    .paletteNum = 12,
    .baseBlock = 0x0155
};

static const struct WindowTemplate sWindowTemplate_ThreeOptions = {
    .bg = 0,
    .tilemapLeft = 8,
    .tilemapTop = 6,
    .width = 14,
    .height = 6,
    .paletteNum = 12,
    .baseBlock = 0x0155
};

static const struct WindowTemplate sWindowTemplate_YesNoBox = {
    .bg = 0,
    .tilemapLeft = 23,
    .tilemapTop = 15,
    .width = 6,
    .height = 4,
    .paletteNum = 12,
    .baseBlock = 0x0155
};

static const struct WindowTemplate sWindowTemplate_GiftSelect_3Options = {
    .bg = 0,
    .tilemapLeft = 22,
    .tilemapTop = 11,
    .width = 7,
    .height = 8,
    .paletteNum = 12,
    .baseBlock = 0x0155
};

static const struct WindowTemplate sWindowTemplate_GiftSelect_2Options = {
    .bg = 0,
    .tilemapLeft = 22,
    .tilemapTop = 13,
    .width = 7,
    .height = 6,
    .paletteNum = 12,
    .baseBlock = 0x0155
};

static const struct WindowTemplate sWindowTemplate_GiftSelect_1Option = {
    .bg = 0,
    .tilemapLeft = 22,
    .tilemapTop = 15,
    .width = 7,
    .height = 4,
    .paletteNum = 12,
    .baseBlock = 0x0155
};

static const struct WindowTemplate sWindowTemplate_PokemonDetails = {
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 3,
    .width = 12,
    .height = 10,
    .paletteNum = 12,
    .baseBlock = 0x0175
};

static const struct ListMenuItem sListMenuItems_SearchDeposit[] = {
    { gText_SearchPokemon,  0 },
    { gText_DepositPokemon,   1 },
    { gText_Exit3,        LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_SearchWithdraw[] = {
    { gText_SearchPokemon,  0 },
    { gText_WithdrawPokemon,                1 },
    { gText_Exit3,                LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_ABC[] = {
    { gText_DexSearchAlphaABC,  0 },
    { gText_DexSearchAlphaDEF,  1 },
    { gText_DexSearchAlphaGHI,  2 },
    { gText_DexSearchAlphaJKL,  3 },
    { gText_DexSearchAlphaMNO,  4 },
    { gText_DexSearchAlphaPQR,  5 },
    { gText_DexSearchAlphaSTU,  6 },
    { gText_DexSearchAlphaVWX,  7 },
    { gText_DexSearchAlphaYZ,   8 },
    { gText_Exit3,                LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_Levels[] = {
    { gText_AnyLevel,  0 },
    { gText_UnderLevel10,  1 },
    { gText_AboveLevel10,  2 },
    { gText_AboveLevel20,  3 },
    { gText_AboveLevel30,  4 },
    { gText_AboveLevel40,  5 },
    { gText_AboveLevel50,  6 },
    { gText_AboveLevel60,  7 },
    { gText_AboveLevel70,  8 },
    { gText_AboveLevel80,  9 },
    { gText_AboveLevel90, 10 },
    { gText_Exit3,                LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_LevelsWanted[] = {
    { gText_AnyLevel,  0 },
    { gText_Level1to10,  1 },
    { gText_Level11to20,  2 },
    { gText_Level21to30,  3 },
    { gText_Level31to40,  4 },
    { gText_Level41to50,  5 },
    { gText_Level51to60,  6 },
    { gText_Level61to70,  7 },
    { gText_Level71to80,  8 },
    { gText_Level81to90,  9 },
    { gText_Level91to100, 10 },
    { gText_Exit3,                LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_GenderSelect[] = {
    { gText_AnyLevel,     0 },
    { gText_MaleSymbol,   1 },
    { gText_FemaleSymbol, 2 }
};

static const struct ListMenuTemplate sListMenuTemplate_ThreeOptions = {
    .items = NULL,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 3,
    .maxShowed = 3,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuItem sListMenuItems_ReceiveSendToss[] = {
    { gText_Receive,  0 },
    { gText_Send,     1 },
    { gText_Toss,     2 },
    { gText_Cancel2,  LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_ReceiveToss[] = {
    { gText_Receive,  0 },
    { gText_Toss,     2 },
    { gText_Cancel2,  LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_ReceiveSend[] = {
    { gText_Receive,  0 },
    { gText_Send,     1 },
    { gText_Cancel2,  LIST_CANCEL }
};

static const struct ListMenuItem sListMenuItems_Receive[] = {
    { gText_Receive,  0 },
    { gText_Cancel2,  LIST_CANCEL }
};

static const struct ListMenuTemplate sListMenu_ReceiveSendToss = {
    .items = sListMenuItems_ReceiveSendToss,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 4,
    .maxShowed = 4,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuTemplate sListMenu_ReceiveToss = {
    .items = sListMenuItems_ReceiveToss,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 3,
    .maxShowed = 3,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuTemplate sListMenu_ABCMenu = {
    .items = sListMenuItems_ABC,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 9,
    .maxShowed = 5,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuTemplate sListMenu_LevelsWanted = {
    .items = sListMenuItems_Levels,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 11,
    .maxShowed = 5,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuTemplate sListMenu_Levels = {
    .items = sListMenuItems_LevelsWanted,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 11,
    .maxShowed = 5,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuTemplate sListMenuTemplate_Genders = {
    .items = sListMenuItems_GenderSelect,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 3,
    .maxShowed = 3,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuTemplate sListMenu_ReceiveSend = {
    .items = sListMenuItems_ReceiveSend,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 3,
    .maxShowed = 3,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const struct ListMenuTemplate sListMenu_Receive = {
    .items = sListMenuItems_Receive,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 2,
    .maxShowed = 2,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .itemVerticalPadding = 0,
    .scrollMultiple = 0,
    .fontId = FONT_NORMAL,
    .cursorKind = 0
};

static const u8 *const sUnusedMenuTexts[] = {
    gText_VarietyOfEventsImportedWireless,
    gText_WonderCardsInPossession,
    gText_ReadNewsThatArrived,
    gText_ReturnToTitle
};

ALIGNED(2) static const u8 sTextColors_TopMenu[]      = { TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE,     TEXT_COLOR_DARK_GRAY };
ALIGNED(2) static const u8 sTextColors_TopMenu_Copy[] = { TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE,     TEXT_COLOR_DARK_GRAY };
ALIGNED(2) static const u8 sGTS_Ereader_TextColor_2[]  = { TEXT_COLOR_WHITE,       TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY };
ALIGNED(2) static const u8 sGTS_Ereader_Male[]  = { TEXT_COLOR_WHITE,       TEXT_COLOR_BLUE, TEXT_COLOR_LIGHT_BLUE };
ALIGNED(2) static const u8 sGTS_Ereader_Female[]  = { TEXT_COLOR_WHITE,       TEXT_COLOR_RED, TEXT_COLOR_LIGHT_RED };

// For scrolling search parameter
#define MAX_SEARCH_PARAM_ON_SCREEN   6
#define MAX_SEARCH_PARAM_CURSOR_POS  (MAX_SEARCH_PARAM_ON_SCREEN - 1)
#define LIST_SCROLL_STEP         16

// By scroll speed. Last element of each unused
static const u8 sScrollMonIncrements[] = {4, 8, 16, 32, 32};
static const u8 sScrollTimers[] = {8, 4, 2, 1, 1};
static const u8 sText_TenDashes[] = _("----------");

struct SearchOptionText
{
    const u8 *title;
};

enum
{
    NAME_ABC = 1,
    NAME_DEF,
    NAME_GHI,
    NAME_JKL,
    NAME_MNO,
    NAME_PQR,
    NAME_STU,
    NAME_VWX,
    NAME_YZ,
};

// First character in range followed by number of characters in range for upper and lowercase
static const u8 sLetterSearchRanges[][4] =
{
    {}, // Name not specified, shouldn't be reached
    [NAME_ABC] = {CHAR_A, 3, CHAR_a, 3},
    [NAME_DEF] = {CHAR_D, 3, CHAR_d, 3},
    [NAME_GHI] = {CHAR_G, 3, CHAR_g, 3},
    [NAME_JKL] = {CHAR_J, 3, CHAR_j, 3},
    [NAME_MNO] = {CHAR_M, 3, CHAR_m, 3},
    [NAME_PQR] = {CHAR_P, 3, CHAR_p, 3},
    [NAME_STU] = {CHAR_S, 3, CHAR_s, 3},
    [NAME_VWX] = {CHAR_V, 3, CHAR_v, 3},
    [NAME_YZ]  = {CHAR_Y, 2, CHAR_y, 2},
};

static const struct SearchOptionText sDexSearchNameOptions[] =
{
    [NAME_ABC] = {gText_DexSearchAlphaABC},
    [NAME_DEF] = {gText_DexSearchAlphaDEF},
    [NAME_GHI] = {gText_DexSearchAlphaGHI},
    [NAME_JKL] = {gText_DexSearchAlphaJKL},
    [NAME_MNO] = {gText_DexSearchAlphaMNO},
    [NAME_PQR] = {gText_DexSearchAlphaPQR},
    [NAME_STU] = {gText_DexSearchAlphaSTU},
    [NAME_VWX] = {gText_DexSearchAlphaVWX},
    [NAME_YZ]  = {gText_DexSearchAlphaYZ},
    {},
};

struct ScrollIndicatorPair
{
    u8 field_0;
    u16 *scrollOffset;
    u16 fullyUpThreshold;
    u16 fullyDownThreshold;
    u8 topSpriteId;
    u8 bottomSpriteId;
    u16 tileTag;
    u16 palTag;
};

static const u16 sBlueInterface_Pal[]    = INCBIN_U16("graphics/interface/blue.gbapal"); 

static void UNUSED PrintSearchText(const u8 *str, u32 x, u32 y)
{
    u8 color[3];

    color[0] = TEXT_COLOR_TRANSPARENT;
    color[1] = TEXT_DYNAMIC_COLOR_6;
    color[2] = TEXT_COLOR_DARK_GRAY;
    AddTextPrinterParameterized4(0, FONT_NORMAL, x, y, 0, 0, color, TEXT_SKIP_DRAW, str);
}

static void UNUSED PrintArrow(u32 x, u32 y)
{
    u8 color[3];

    color[0] = TEXT_COLOR_TRANSPARENT;
    color[1] = TEXT_DYNAMIC_COLOR_6;
    color[2] = TEXT_COLOR_DARK_GRAY;
    AddTextPrinterParameterized4(0, FONT_NORMAL, x, y, 0, 0, color, TEXT_SKIP_DRAW, gText_SelectorArrow);
}

static int DoPokedexSearchGTS(u8 abcGroup)
{
    u16 species;
    u16 i;
    u16 resultsCount;

    CreatePokedexListGTS();


    for (i = 0, resultsCount = 0; i < NATIONAL_DEX_COUNT; i++)
    {
        if (sGTSPokedexView->pokedexList[i].seen)
        {
            sGTSPokedexView->pokedexList[resultsCount] = sGTSPokedexView->pokedexList[i];
            resultsCount++;
        }
    }
    sGTSPokedexView->pokemonListCount = resultsCount;

    // Search by name
    if (abcGroup != 0xFF)
    {
        for (i = 0, resultsCount = 0; i < sGTSPokedexView->pokemonListCount; i++)
        {
            u8 firstLetter;

            species = NationalPokedexNumToSpecies(sGTSPokedexView->pokedexList[i].dexNum);
            firstLetter = GetSpeciesName(species)[0];
            if (LETTER_IN_RANGE_UPPER(firstLetter, abcGroup) || LETTER_IN_RANGE_LOWER(firstLetter, abcGroup))
            {
                sGTSPokedexView->pokedexList[resultsCount] = sGTSPokedexView->pokedexList[i];
                resultsCount++;
            }
        }
        sGTSPokedexView->pokemonListCount = resultsCount;
    }

    if (sGTSPokedexView->pokemonListCount != 0)
    {
        for (i = sGTSPokedexView->pokemonListCount; i < NATIONAL_DEX_COUNT; i++)
        {
            sGTSPokedexView->pokedexList[i].dexNum = 0xFFFF;
            sGTSPokedexView->pokedexList[i].seen = FALSE;
            sGTSPokedexView->pokedexList[i].owned = FALSE;
        }
    }

    return resultsCount;
}

static void ResetPokedexViewGTS(struct GTSPokedexView *pokedexView)
{
    u16 i;

    for (i = 0; i < NATIONAL_DEX_COUNT; i++)
    {
        pokedexView->pokedexList[i].dexNum = 0xFFFF;
        pokedexView->pokedexList[i].seen = FALSE;
        pokedexView->pokedexList[i].owned = FALSE;
    }
    pokedexView->pokedexList[NATIONAL_DEX_COUNT].dexNum = 0;
    pokedexView->pokedexList[NATIONAL_DEX_COUNT].seen = FALSE;
    pokedexView->pokedexList[NATIONAL_DEX_COUNT].owned = FALSE;
    pokedexView->pokemonListCount = 0;
    pokedexView->selectedPokemon = 0;
    pokedexView->offerPokemon = 0;
    pokedexView->dexMode = DEX_MODE_HOENN;
    pokedexView->windowid = DEX_MODE_HOENN;
    pokedexView->dexOrder = 0;
    //for (i = 0; i < 4; i++)
    //    pokedexView->monSpriteIds[i] = 0xFFFF;
    pokedexView->cursorRelPos = 0;
    pokedexView->atTop = 0;
    pokedexView->atBottom = 0;
    pokedexView->initialVOffset = 0;
    pokedexView->scrollTimer = 0;
    pokedexView->scrollDirection = 0;
    pokedexView->listVOffset = 0;
    pokedexView->listMovingVOffset = 0;
    pokedexView->scrollMonIncrement = 0;
    pokedexView->maxScrollTimer = 0;
    pokedexView->scrollSpeed = 0;
    pokedexView->currentPage = 0;
}

static void CreatePokedexListGTS()
{
    //u32 vars[3]; //I have no idea why three regular variables are stored in an array, but whatever.
    //u32 temp_dexCount;
    u32 temp_isHoennDex;
    u32 temp_dexNum;
    s32 i;

    sGTSPokedexView->pokemonListCount = 0;


    //temp_dexCount = NATIONAL_DEX_COUNT;
    temp_isHoennDex = FALSE;

    //sGTSPokedexView->pokemonListCount = 0;
    //if(sGTSPokedexView->pokemonListCount==0)
    //    PlayFanfare(MUS_OBTAIN_ITEM);
    
    for (i = 0; i < NATIONAL_DEX_COUNT; i++)
    {
        temp_dexNum = gPokedexOrder_Alphabetical[i];

        if (temp_dexNum <= NATIONAL_DEX_COUNT && (!temp_isHoennDex || NationalToHoennOrder(temp_dexNum) != 0) && GetSetPokedexFlag(temp_dexNum, FLAG_GET_SEEN))
        {
            sGTSPokedexView->pokedexList[sGTSPokedexView->pokemonListCount].dexNum = temp_dexNum;
            sGTSPokedexView->pokedexList[sGTSPokedexView->pokemonListCount].seen = TRUE;
            sGTSPokedexView->pokedexList[sGTSPokedexView->pokemonListCount].owned = GetSetPokedexFlag(temp_dexNum, FLAG_GET_CAUGHT);
            //if(sGTSPokedexView->pokemonListCount==0)
              //  PlayFanfare(MUS_OBTAIN_ITEM);
            sGTSPokedexView->pokemonListCount++;
        }
    }

    //if(sGTSPokedexView->pokedexList[0].dexNum==0xC002)
    //    PlayFanfare(MUS_OBTAIN_ITEM);

    for (i = sGTSPokedexView->pokemonListCount; i < NATIONAL_DEX_COUNT; i++)
    {
        sGTSPokedexView->pokedexList[i].dexNum = 0xFFFF;
        sGTSPokedexView->pokedexList[i].seen = FALSE;
        sGTSPokedexView->pokedexList[i].owned = FALSE;
    }

//    if(gPokedexOrder_Alphabetical[1]==63)
//        PlayFanfare(MUS_OBTAIN_ITEM);

//    if(sGTSPokedexView->pokedexList[1].dexNum==0xC002)
//        PlayFanfare(MUS_OBTAIN_ITEM);

}

static void PrintMonDexNumAndNameGTS(u8 windowId, u8 fontId, const u8 *str, u8 left, u8 top)
{
    u8 color[3];

    color[0] = TEXT_COLOR_TRANSPARENT;
    color[1] = TEXT_COLOR_DARK_GRAY;
    color[2] = TEXT_COLOR_LIGHT_GRAY;
    AddTextPrinterParameterized4(windowId, fontId, left * 8, (top * 8) + 1, 0, 0, color, TEXT_SKIP_DRAW, str);
    //AddTextPrinterParameterized4(windowId, fontId, 1, 1, 0, 0, color, TEXT_SKIP_DRAW, str);
}

static void PrintMonDexArrowGTS(u8 windowId, u8 fontId, u8 left, u8 top)
{
    u8 color[3];

    color[0] = TEXT_COLOR_TRANSPARENT;
    color[1] = TEXT_COLOR_DARK_GRAY;
    color[2] = TEXT_COLOR_LIGHT_GRAY;
    AddTextPrinterParameterized4(windowId, fontId, left * 8, (top * 8) + 1, 0, 0, color, TEXT_SKIP_DRAW, gText_SelectorArrow);
}

static void ClearMonListArrowGTS(u8 x, u8 y)
{
    FillWindowPixelRect(sGTSPokedexView->windowid, PIXEL_FILL(1), x * 8, y * 8, 8, 16);
    //FillWindowPixelRect(0, PIXEL_FILL(0), 0, 0, 0x60, 16);
}

static u8 CreateMonNameGTS(u16 num, u8 left, u8 top)
{
    const u8 *str;

    //if(num>0x1000)
    //    PlayFanfare(MUS_OBTAIN_ITEM);

    num = NationalPokedexNumToSpecies(num);
    
    if (num)
        str = GetSpeciesName(num);
    else
        str = sText_TenDashes;
    PrintMonDexNumAndNameGTS(sGTSPokedexView->windowid, FONT_NORMAL, str, left, top);
    return StringLength(str);
}

// u16 ignored is passed but never used
static void CreateMonListEntryGTS(u8 position, u16 b)
{
    s16 entryNum;
    s16 i=0;
    //u16 vOffset;


    switch (position)
    {
    case 0: // Initial
    default:
        entryNum = b;
        sGTSPokedexView->cursorRelPos = 0;
        PrintMonDexArrowGTS(sGTSPokedexView->windowid, FONT_NORMAL, 0, sGTSPokedexView->cursorRelPos);
        for (i = 0; i <= 4; i++)
        {
            if (entryNum < 0 || entryNum >= NATIONAL_DEX_COUNT || sGTSPokedexView->pokedexList[entryNum].dexNum == 0xFFFF)
            {
                ClearMonListEntryGTS(1, i * 2);
            }
            else
            {
                ClearMonListEntryGTS(1, i * 2);
                //if(sGTSPokedexView->pokedexList[0].dexNum==0)
                //    PlayFanfare(MUS_OBTAIN_ITEM);
                CreateMonNameGTS(sGTSPokedexView->pokedexList[entryNum].dexNum, 1, i * 2);
            }
            entryNum++;
        }
        break;
    case 1: // Up
        entryNum = b - 1;/*
        if (entryNum < 0 || entryNum >= NATIONAL_DEX_COUNT || sGTSPokedexView->pokedexList[entryNum].dexNum == 0xFFFF)
        {
            ClearMonListEntryGTS(1, sGTSPokedexView->listVOffset * 2);
        }
        else
        {
            ClearMonListEntryGTS(1, sGTSPokedexView->listVOffset * 2);
            if (sGTSPokedexView->pokedexList[entryNum].seen)
            {
                CreateMonNameGTS(sGTSPokedexView->pokedexList[entryNum].dexNum, 1, sGTSPokedexView->listVOffset * 2);
            }
            else
            {
                CreateMonNameGTS(0, 1, sGTSPokedexView->listVOffset * 2);
            }
        }
        if (sGTSPokedexView->listVOffset > 0)
            sGTSPokedexView->listVOffset--;
        else
            sGTSPokedexView->listVOffset = LIST_SCROLL_STEP - 1;*/
        if(sGTSPokedexView->cursorRelPos == 1 && sGTSPokedexView->atTop !=1){
            for (i = 0; i <= 4; i++)
            {
                if (entryNum < 0 || entryNum >= NATIONAL_DEX_COUNT || sGTSPokedexView->pokedexList[entryNum].dexNum == 0xFFFF)
                {
                    ClearMonListEntryGTS(1, i * 2);
                }
                else
                {
                    ClearMonListEntryGTS(1, i * 2);
                    //if(sGTSPokedexView->pokedexList[0].dexNum==0)
                    //    PlayFanfare(MUS_OBTAIN_ITEM);
                    CreateMonNameGTS(sGTSPokedexView->pokedexList[entryNum].dexNum, 1, i * 2);
                }
                entryNum++;
            }
            sGTSPokedexView->atBottom = 0;
        }
        else if(sGTSPokedexView->cursorRelPos == 0 && sGTSPokedexView->atTop !=1){
            entryNum = b; 
            for (i = 0; i <= 4; i++)
            {
                if (entryNum < 0 || entryNum >= NATIONAL_DEX_COUNT || sGTSPokedexView->pokedexList[entryNum].dexNum == 0xFFFF)
                {
                    ClearMonListEntryGTS(1, i * 2);
                }
                else
                {
                    ClearMonListEntryGTS(1, i * 2);
                    //if(sGTSPokedexView->pokedexList[0].dexNum==0)
                    //    PlayFanfare(MUS_OBTAIN_ITEM);
                    CreateMonNameGTS(sGTSPokedexView->pokedexList[entryNum].dexNum, 1, i * 2);
                }
                entryNum++;
            }
            sGTSPokedexView->atBottom = 0;
        }
        else{
            ClearMonListArrowGTS(0,sGTSPokedexView->cursorRelPos*2);
            sGTSPokedexView->cursorRelPos--;
            PrintMonDexArrowGTS(sGTSPokedexView->windowid, FONT_NORMAL, 0, sGTSPokedexView->cursorRelPos*2);
        }
        break;
    case 2: // Down
        entryNum = b - 3;

        if(sGTSPokedexView->cursorRelPos == 3 && sGTSPokedexView->atBottom !=1){
            for (i = 0; i <= 4; i++)
            {
                if (entryNum < 0 || entryNum >= NATIONAL_DEX_COUNT || sGTSPokedexView->pokedexList[entryNum].dexNum == 0xFFFF)
                {
                    ClearMonListEntryGTS(1, i * 2);
                }
                else
                {
                    ClearMonListEntryGTS(1, i * 2);
                    //if(sGTSPokedexView->pokedexList[0].dexNum==0)
                    //    PlayFanfare(MUS_OBTAIN_ITEM);
                    CreateMonNameGTS(sGTSPokedexView->pokedexList[entryNum].dexNum, 1, i * 2);
                }
                entryNum++;
            }
        }
        else{
            ClearMonListArrowGTS(0,sGTSPokedexView->cursorRelPos*2);
            sGTSPokedexView->cursorRelPos++;
            PrintMonDexArrowGTS(sGTSPokedexView->windowid, FONT_NORMAL, 0, sGTSPokedexView->cursorRelPos*2);

        }
        break;
    case 3: // Skip Up
        entryNum = b;
        
        if(sGTSPokedexView->atTop ==1){
            if(sGTSPokedexView->atBottom ==1){
                for (i = 0; i <= 4; i++){ 
                    if (entryNum < 0 || entryNum >= NATIONAL_DEX_COUNT || sGTSPokedexView->pokedexList[entryNum].dexNum == 0xFFFF){
                        b=i-1;
                    }
                }
                ClearMonListArrowGTS(0,sGTSPokedexView->cursorRelPos*2);
                sGTSPokedexView->cursorRelPos=b-1;
                PrintMonDexArrowGTS(sGTSPokedexView->windowid, FONT_NORMAL, 0, sGTSPokedexView->cursorRelPos*2);
            }
            else{
                ClearMonListArrowGTS(0,sGTSPokedexView->cursorRelPos*2);
                sGTSPokedexView->cursorRelPos=0;
                PrintMonDexArrowGTS(sGTSPokedexView->windowid, FONT_NORMAL, 0, sGTSPokedexView->cursorRelPos*2);
            }
        
        }
        else{
            sGTSPokedexView->atBottom = 0;
            for (i = 4; i >= 0; i++)
            {
                if (entryNum < 0 || entryNum >= NATIONAL_DEX_COUNT || sGTSPokedexView->pokedexList[entryNum].dexNum == 0xFFFF)
                    break;
                entryNum++;
            }
            entryNum=b;
            b=i;
            if(i >= 0){
                for (i = 0; i <= 4; i++)
                {
                    ClearMonListEntryGTS(1, i * 2);
                    CreateMonNameGTS(sGTSPokedexView->pokedexList[i].dexNum, 1, i * 2);
                    if(entryNum==i){
                        ClearMonListArrowGTS(0,sGTSPokedexView->cursorRelPos*2);
                        sGTSPokedexView->cursorRelPos=i;
                        PrintMonDexArrowGTS(sGTSPokedexView->windowid, FONT_NORMAL, 0, sGTSPokedexView->cursorRelPos*2);
                    }
                }
                sGTSPokedexView->atTop =1;
            }
            else{
                entryNum=entryNum-sGTSPokedexView->cursorRelPos;
                if(entryNum==0)
                    sGTSPokedexView->atTop =1;
                for (i = 0; i <= 4; i++)
                {
                    ClearMonListEntryGTS(1, i * 2);
                    CreateMonNameGTS(sGTSPokedexView->pokedexList[entryNum].dexNum, 1, i * 2);
                    entryNum++;
                }
            }
        }


        break;
    case 4: // Skip Down
        entryNum = b;
        
        if(sGTSPokedexView->atBottom ==1){
            if(sGTSPokedexView->atTop ==1){
                for (i = 0; i <= 4; i++){
                    if (entryNum >= NATIONAL_DEX_COUNT || sGTSPokedexView->pokedexList[entryNum].dexNum == 0xFFFF){
                        b=i-1;
                    }
                }
                ClearMonListArrowGTS(0,sGTSPokedexView->cursorRelPos*2);
                sGTSPokedexView->cursorRelPos=b+1;
                PrintMonDexArrowGTS(sGTSPokedexView->windowid, FONT_NORMAL, 0, sGTSPokedexView->cursorRelPos*2);
            }
            else{
                ClearMonListArrowGTS(0,sGTSPokedexView->cursorRelPos*2);
                sGTSPokedexView->cursorRelPos=4;
                PrintMonDexArrowGTS(sGTSPokedexView->windowid, FONT_NORMAL, 0, sGTSPokedexView->cursorRelPos*2);
            }
        
        }
        else{
            sGTSPokedexView->atTop = 0;
            for (i = 0; i <= 4; i++)
            {
                if (entryNum < 0 || entryNum >= NATIONAL_DEX_COUNT || sGTSPokedexView->pokedexList[entryNum].dexNum == 0xFFFF)
                    break;
                entryNum++;
            }
            entryNum=b;
            b=i;
            if(i < 5){
                for (i = 0; i <= 4; i++)
                {
                    ClearMonListEntryGTS(1, i * 2);
                    CreateMonNameGTS(sGTSPokedexView->pokedexList[sGTSPokedexView->pokemonListCount - 1 - 4 + i].dexNum, 1, i * 2);
                    if(entryNum==sGTSPokedexView->pokemonListCount - 1 - 4 + i){
                        ClearMonListArrowGTS(0,sGTSPokedexView->cursorRelPos*2);
                        sGTSPokedexView->cursorRelPos=i;
                        PrintMonDexArrowGTS(sGTSPokedexView->windowid, FONT_NORMAL, 0, sGTSPokedexView->cursorRelPos*2);
                    }
                }
                sGTSPokedexView->atBottom =1;
            }
            else{
                entryNum=entryNum-sGTSPokedexView->cursorRelPos;
                for (i = 0; i <= 4; i++)
                {
                    ClearMonListEntryGTS(1, i * 2);
                    CreateMonNameGTS(sGTSPokedexView->pokedexList[entryNum].dexNum, 1, i * 2);
                    entryNum++;
                }
                if(entryNum==sGTSPokedexView->pokemonListCount)
                    sGTSPokedexView->atBottom =1;
            }
        }


        break;
    }
    CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
}

static bool8 UpdateDexListScroll(u8 direction, u8 monMoveIncrement, u8 scrollTimerMax)
{
    //u16 i;
    u8 step;

    if (sGTSPokedexView->scrollTimer)
    {
        sGTSPokedexView->scrollTimer--;
        switch (direction)
        {
        case 1: // Up
            step = LIST_SCROLL_STEP * (scrollTimerMax - sGTSPokedexView->scrollTimer) / scrollTimerMax;
            SetGpuReg(REG_OFFSET_BG2VOFS, sGTSPokedexView->initialVOffset + sGTSPokedexView->listMovingVOffset * LIST_SCROLL_STEP - step);
            break;
        case 2: // Down
            step = LIST_SCROLL_STEP * (scrollTimerMax - sGTSPokedexView->scrollTimer) / scrollTimerMax;
            SetGpuReg(REG_OFFSET_BG2VOFS, sGTSPokedexView->initialVOffset + sGTSPokedexView->listMovingVOffset * LIST_SCROLL_STEP + step);
            break;
        }
        return FALSE;
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG2VOFS, sGTSPokedexView->initialVOffset + sGTSPokedexView->listVOffset * LIST_SCROLL_STEP);
        return TRUE;
    }
}

// u16 ignored is passed but never used
static u16 TryDoPokedexScrollGTS(u16 selectedMon)
{
    u8 scrollTimer;
    u8 scrollMonIncrement;
    u8 i;
    //u16 startingPos;
    u8 scrollDir = 0;

    if (JOY_NEW(DPAD_UP) && selectedMon != 0)
    {
        scrollDir = 1;
        selectedMon = GetNextPositionGTS(1, selectedMon, 0, sGTSPokedexView->pokemonListCount - 1);
        CreateMonListEntryGTS(1, selectedMon);
        if(selectedMon == 0 || (selectedMon == 1 && sGTSPokedexView->cursorRelPos == 1))
            sGTSPokedexView->atTop=1;
        if(sGTSPokedexView->atBottom == 1 && sGTSPokedexView->cursorRelPos == 1)
            sGTSPokedexView->atBottom = 0;
        //CreateMonListEntryGTS(1, selectedMon);
        PlaySE(SE_DEX_SCROLL);
    }
    else if (JOY_NEW(DPAD_DOWN) && (selectedMon < sGTSPokedexView->pokemonListCount - 1))
    {
        scrollDir = 2;
        selectedMon = GetNextPositionGTS(0, selectedMon, 0, sGTSPokedexView->pokemonListCount - 1);
        if(selectedMon == sGTSPokedexView->pokemonListCount - 1)
            sGTSPokedexView->atBottom=1;
        if(sGTSPokedexView->atTop == 1 && sGTSPokedexView->cursorRelPos == 3)
            sGTSPokedexView->atTop = 0;
        CreateMonListEntryGTS(2, selectedMon);
        PlaySE(SE_DEX_SCROLL);
    }
    else if (JOY_NEW(DPAD_LEFT) && (selectedMon > 0))
    {
        //startingPos = selectedMon;

        for (i = 0; i < 5; i++){
            selectedMon = GetNextPositionGTS(1, selectedMon, 0, sGTSPokedexView->pokemonListCount - 1);
            if(selectedMon == sGTSPokedexView->pokemonListCount - 1)
                break;
        }
        CreateMonListEntryGTS(3, selectedMon);
        PlaySE(SE_DEX_PAGE);
    }
    else if (JOY_NEW(DPAD_RIGHT) && (selectedMon < sGTSPokedexView->pokemonListCount - 1))
    {
        //startingPos = selectedMon;
        for (i = 0; i < 5; i++){
            selectedMon = GetNextPositionGTS(0, selectedMon, 0, sGTSPokedexView->pokemonListCount - 1);
            if(selectedMon == sGTSPokedexView->pokemonListCount - 1){
                //sGTSPokedexView->atBottom=1;
                //if(i==4)
                //    sGTSPokedexView->cursorRelPos = 4;
                break;
            }
            //if(i==4)
            //    sGTSPokedexView->atTop = 0;
        }
        CreateMonListEntryGTS(4, selectedMon);
        PlaySE(SE_DEX_PAGE);
    }

    if (scrollDir == 0)
    {
        // Left/right input just snaps up/down, no scrolling
        sGTSPokedexView->scrollSpeed = 0;
        return selectedMon;
    }

    scrollMonIncrement = sScrollMonIncrements[1];
    scrollTimer = sScrollTimers[1];
    sGTSPokedexView->scrollTimer = scrollTimer;
    sGTSPokedexView->maxScrollTimer = scrollTimer;
    sGTSPokedexView->scrollMonIncrement = scrollMonIncrement;
    sGTSPokedexView->scrollDirection = scrollDir;
    UpdateDexListScroll(sGTSPokedexView->scrollDirection, sGTSPokedexView->scrollMonIncrement, sGTSPokedexView->maxScrollTimer);
    //if (sGTSPokedexView->scrollSpeed < 12)
    //    sGTSPokedexView->scrollSpeed++;
    return selectedMon;
}

static void ClearMonListEntryGTS(u8 x, u8 y)
{
    FillWindowPixelRect(sGTSPokedexView->windowid, PIXEL_FILL(1), x * 8, y * 8, 0x60, 16);
    //FillWindowPixelRect(0, PIXEL_FILL(0), 0, 0, 0x60, 16);
}

static u16 GetNextPositionGTS(u8 direction, u16 position, u16 min, u16 max)
{
    switch (direction)
    {
    case 1: // Up/Left
        if (position > min)
            position--;
        break;
    case 0: // Down/Right
        if (position < max)
            position++;
        break;
    case 3: // Up/Left with loop (unused)
        if (position > min)
            position--;
        else
            position = max;
        break;
    case 2: // Down/Right with loop (unused)
        if (position < max)
            position++;
        else
            position = min;
        break;
    }
    return position;
}

static u16 UNUSED GetPokemonSpriteToDisplay(u16 species)
{
    if (species >= NATIONAL_DEX_COUNT || sGTSPokedexView->pokedexList[species].dexNum == 0xFFFF)
        return 0xFFFF;
    else if (sGTSPokedexView->pokedexList[species].seen)
        return sGTSPokedexView->pokedexList[species].dexNum;
    else
        return 0;
}

static u32 CreatePokedexMonSprite(u16 num, s16 x, s16 y, struct BoxPokemon *boxmon)
{
    u8 i;
    u8 spriteId;

    for (i = 0; i < MAX_MONS_ON_SCREEN; i++)
    {
        if (sGTSPokedexView->monSpriteIds[i] == 0xFFFF)
        {
            bool8 isShiny = GetBoxMonData(boxmon, MON_DATA_IS_SHINY, NULL);
            if(isShiny)
                spriteId = CreateMonPicSprite(num, TRUE, GetPokedexMonPersonality(num), TRUE, x, y, 2, TAG_NONE); //CreateMonSpriteFromNationalDexNumber(num, x, y, i);
            else
                spriteId = CreateMonPicSprite(num, FALSE, GetPokedexMonPersonality(num), TRUE, x, y, 3, TAG_NONE);
            //spriteId = CreateMonSpriteFromNationalDexNumber(num,  x, y, 3);
            gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_NORMAL;
            gSprites[spriteId].oam.priority = 3;
            gSprites[spriteId].data[0] = 0;
            gSprites[spriteId].data[1] = i;
            gSprites[spriteId].data[2] = NationalPokedexNumToSpecies(num);
            sGTSPokedexView->monSpriteIds[i] = spriteId;
            DebugPrintf("CreatePokedexMonSprite");
            DebugPrintf("%u",NationalPokedexNumToSpecies(num));
            return spriteId;
        }
    }
    return 0xFFFF;
}

static void SpriteCB_PokedexListMonSprite(struct Sprite *sprite)
{
    u8 monId = sprite->data[1];

    u32 var;
    
    sprite->x2 = gSineTable[(u8)sprite->data[5]] * 76 / 256;
    var = SAFE_DIV(0x10000, gSineTable[sprite->data[5] + 64]);
    if (var > 0xFFFF)
        var = 0xFFFF;
    //SetOamMatrix(sprite->data[1] + 1, 0x100, 0, 0, var);
    SetOamMatrix(sprite->data[1] + 1, var, 0, 0, 0x100);
    sprite->oam.matrixNum = monId + 1;

    if (sprite->data[5] > -64 && sprite->data[5] < 64)
    {
        sprite->invisible = FALSE;
        sprite->data[0] = 1;
    }
    else
    {
        sprite->invisible = TRUE;
    }

    if ((sprite->data[5] <= -64 || sprite->data[5] >= 64) && sprite->data[0] != 0)
    {
        FreeAndDestroyMonPicSprite(sGTSPokedexView->monSpriteIds[monId]);
        sGTSPokedexView->monSpriteIds[monId] = 0xFFFF;
    }
}

static u16 GetNextPosition(u8 direction, u16 position, u16 min, u16 max)
{
    switch (direction)
    {
    case 1: // Up/Left
        if (position > min)
            position--;
        break;
    case 0: // Down/Right
        if (position < max)
            position++;
        break;
    case 3: // Up/Left with loop (unused)
        if (position > min)
            position--;
        else
            position = max;
        break;
    case 2: // Down/Right with loop (unused)
        if (position < max)
            position++;
        else
            position = min;
        break;
    }
    return position;
}


// u16 ignored is passed but never used
static void CreateMonSpritesAtPos(u16 selectedMon, u16 ignored)
{
    u8 i;
    u16 dexNum;
    u8 spriteId;

    //gPaletteFade.bufferTransferDisabled = TRUE;

    for (i = 0; i < MAX_MONS_ON_SCREEN; i++)
        sGTSPokedexView->monSpriteIds[i] = 0xFFFF;

    DebugPrintf("MonSprite1");
    // Create top mon sprite
    if (selectedMon == 0)
        dexNum = 0xFFFF;
    else
        dexNum = sGTSPokedexView->searchResult[selectedMon-1].dexNum;//pokedexList[selectedMon - 1].dexNum;
    if (dexNum > SPECIES_NONE && dexNum < SPECIES_EGG)
    {
        DebugPrintf("oh no");
        spriteId = CreatePokedexMonSprite(dexNum, 0xB4, 0x40, &sGTSPokedexView->searchResult[selectedMon-1].boxmon);
        gSprites[spriteId].callback = SpriteCB_PokedexListMonSprite;
        gSprites[spriteId].data[5] = -32;
    }
    
    DebugPrintf("MonSprite2");
    // Create mid mon sprite
    dexNum = sGTSPokedexView->searchResult[selectedMon].dexNum;
    if (dexNum > SPECIES_NONE && dexNum < SPECIES_EGG)
    {
        DebugPrintf("%u",dexNum);
        spriteId = CreatePokedexMonSprite(dexNum, 0xB4, 0x40, &sGTSPokedexView->searchResult[selectedMon].boxmon);
        gSprites[spriteId].callback = SpriteCB_PokedexListMonSprite;
        gSprites[spriteId].data[5] = 0;
    }

    DebugPrintf("MonSprite3");
    // Create bottom mon sprite
    dexNum = sGTSPokedexView->searchResult[selectedMon+1].dexNum;
    if (dexNum > SPECIES_NONE && dexNum < SPECIES_EGG)
    {
        DebugPrintf("%u",dexNum);
        spriteId = CreatePokedexMonSprite(dexNum, 0xB4, 0x40, &sGTSPokedexView->searchResult[selectedMon+1].boxmon);
        gSprites[spriteId].callback = SpriteCB_PokedexListMonSprite;
        gSprites[spriteId].data[5] = 32;
    }

    DebugPrintf("MonSprite4");
    //CreateMonListEntry(0, selectedMon, ignored);
    SetGpuReg(REG_OFFSET_BG2VOFS, sGTSPokedexView->initialVOffset);

    sGTSPokedexView->listVOffset = 0;
    sGTSPokedexView->listMovingVOffset = 0;

    gPaletteFade.bufferTransferDisabled = FALSE;
    DebugPrintf("MonSprite5");
}

static void CreateScrollingPokemonSprite(u8 direction, u16 selectedMon)
{
    u16 dexNum;
    u8 spriteId;

    sGTSPokedexView->listMovingVOffset = sGTSPokedexView->listVOffset;
    switch (direction)
    {
    case 1: // up
        dexNum = sGTSPokedexView->searchResult[selectedMon - 1].dexNum;
        if (dexNum > SPECIES_NONE && dexNum < SPECIES_EGG)
        {
            spriteId = CreatePokedexMonSprite(dexNum, 0xB4, 0x40, &sGTSPokedexView->searchResult[selectedMon-1].boxmon);
            gSprites[spriteId].callback = SpriteCB_PokedexListMonSprite;
            gSprites[spriteId].data[5] = -64;
        }
        if (sGTSPokedexView->listVOffset > 0)
            sGTSPokedexView->listVOffset--;
        else
            sGTSPokedexView->listVOffset = LIST_SCROLL_STEP - 1;
        break;
    case 2: // down
        dexNum = sGTSPokedexView->searchResult[selectedMon + 1].dexNum;
        if (dexNum > SPECIES_NONE && dexNum < SPECIES_EGG)
        {
            spriteId = CreatePokedexMonSprite(dexNum, 0xB4, 0x40, &sGTSPokedexView->searchResult[selectedMon+1].boxmon);
            gSprites[spriteId].callback = SpriteCB_PokedexListMonSprite;
            gSprites[spriteId].data[5] = 64;
            DebugPrintf("%u", dexNum);
        }
        if (sGTSPokedexView->listVOffset < LIST_SCROLL_STEP - 1)
            sGTSPokedexView->listVOffset++;
        else
            sGTSPokedexView->listVOffset = 0;
        break;
    }
}

// u16 ignored is passed but never used
static u16 TryDoGTSSpriteScroll(u16 selectedMon, u16 ignored)
{
    u8 scrollTimer;
    u8 scrollMonIncrement;
    //u8 i;
    //u16 startingPos;
    u8 scrollDir = 0;

    if (JOY_NEW(A_BUTTON))
    {
        return (selectedMon+1000);
    }

    if (JOY_NEW(B_BUTTON))
    {
        return (selectedMon+500);
    }

    if (JOY_NEW(DPAD_RIGHT) && (selectedMon == 6) && sGTSPokedexView->scrollTimer == 0){
        PlaySE(SE_DEX_SCROLL);
        return 7;
    }
    else if (JOY_NEW(DPAD_LEFT) && (selectedMon == 0) && sGTSPokedexView->scrollTimer == 0 && sGTSPokedexView->currentPage != 0){
        PlaySE(SE_DEX_SCROLL);
        return 8;
    }
    else if (JOY_HELD(DPAD_LEFT) && (selectedMon > 0) && sGTSPokedexView->scrollTimer == 0)
    {
        scrollDir = 1;
        selectedMon = GetNextPosition(1, selectedMon, 0, sGTSPokedexView->pokemonListCount - 1);
        CreateScrollingPokemonSprite(1, selectedMon);
        //CreateMonListEntry(1, selectedMon, ignored);
        PlaySE(SE_DEX_SCROLL);
    }
    else if (JOY_HELD(DPAD_RIGHT) && (selectedMon < sGTSPokedexView->pokemonListCount - 1) && sGTSPokedexView->scrollTimer == 0)
    {
        scrollDir = 2;
        selectedMon = GetNextPosition(0, selectedMon, 0, sGTSPokedexView->pokemonListCount - 1);
        CreateScrollingPokemonSprite(2, selectedMon);
        //CreateMonListEntry(2, selectedMon, ignored);
        PlaySE(SE_DEX_SCROLL);
    }

    if (scrollDir == 0)
    {
        // Left/right input just snaps up/down, no scrolling
        sGTSPokedexView->scrollSpeed = 0;
        return selectedMon;
    }

    scrollMonIncrement = sScrollMonIncrements[sGTSPokedexView->scrollSpeed / 4];
    scrollTimer = sScrollTimers[sGTSPokedexView->scrollSpeed / 4];
    sGTSPokedexView->scrollTimer = scrollTimer;
    sGTSPokedexView->maxScrollTimer = scrollTimer;
    sGTSPokedexView->scrollMonIncrement = scrollMonIncrement;
    sGTSPokedexView->scrollDirection = scrollDir;
    //sPokedexView->pokeBallRotationStep = scrollMonIncrement / 2;
    //UpdateDexListScroll(sPokedexView->scrollDirection, sPokedexView->scrollMonIncrement, sPokedexView->maxScrollTimer);
    if (sGTSPokedexView->scrollSpeed < 12)
        sGTSPokedexView->scrollSpeed++;

    DebugPrintf("Print details");
    FillWindowPixelBuffer(sGTSPokedexView->windowid, 0x11);
    DrawTextBorderOuter(sGTSPokedexView->windowid, 0x001, 0x0F);
    //Print Name gender Lv
    AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 1, 1, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, sGTSPokedexView->searchResult[selectedMon].boxmon.nickname);

    if(sGTSPokedexView->searchResult[selectedMon].gender==MON_MALE){
        AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 61, 1, 0, 0, sGTS_Ereader_Male, TEXT_SKIP_DRAW, gText_MaleSymbol);
    }
    else if (sGTSPokedexView->searchResult[selectedMon].gender==MON_FEMALE)
    {
        AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 61, 1, 0, 0, sGTS_Ereader_Female, TEXT_SKIP_DRAW, gText_FemaleSymbol);
    }

    ConvertIntToDecimalStringN(gStringVar1, sGTSPokedexView->searchResult[selectedMon].level, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringExpandPlaceholders(gStringVar2,gText_LvVar1);

    AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 69, 1, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, gStringVar2);
    //Print ITEM
    AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 1, 17, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, gText_ItemCaps);
    //Print the held item
    scrollDir=GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon, MON_DATA_HELD_ITEM);
    if(scrollDir==ITEM_NONE){
        AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 9, 33, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, gText_None);
    }
    else{
        AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 9, 33, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, GetItemName(scrollDir));
    }
    //Print OFFERER
    AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 1, 49, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, gText_Offerer);
    //Print the offerer
    //StringCopy_PlayerName(gStringVar1, gSaveBlock2Ptr->playerName);
    //DebugPrintf("Name goes here");
    //DebugPrintf("%u",sGTSPokedexView->searchResult[selectedMon].OTName[0]);
    //DebugPrintf("%u",sGTSPokedexView->searchResult[selectedMon].OTName[1]);
    //DebugPrintf("%u",sGTSPokedexView->searchResult[selectedMon].OTName[2]);
    //DebugPrintf("%u",sGTSPokedexView->searchResult[selectedMon].OTName[3]);
    ASCIIToPkmnStrLength(gStringVar1,(u8 *)sGTSPokedexView->searchResult[selectedMon].OTName,7);
    if(sGTSPokedexView->searchResult[selectedMon].trainerGender==0){
        AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 9, 65, 0, 0, sGTS_Ereader_Male, TEXT_SKIP_DRAW, gStringVar1);
    }
    else{
        AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 9, 65, 0, 0, sGTS_Ereader_Female, TEXT_SKIP_DRAW, gStringVar1);
    }
    // /AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 9, 65, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, gStringVar1);
    CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
    GTSAddWantedToWindow1(selectedMon);
    return selectedMon;
}

static bool8 UpdateDexListScrollSprites(u8 direction, u8 monMoveIncrement, u8 scrollTimerMax)
{
    u16 i;
    u8 step;

    if (sGTSPokedexView->scrollTimer)
    {
        sGTSPokedexView->scrollTimer--;
        switch (direction)
        {
        case 1: // Up
            for (i = 0; i < MAX_MONS_ON_SCREEN; i++)
            {
                if (sGTSPokedexView->monSpriteIds[i] != 0xFFFF)
                    gSprites[sGTSPokedexView->monSpriteIds[i]].data[5] += monMoveIncrement;
            }
            step = LIST_SCROLL_STEP * (scrollTimerMax - sGTSPokedexView->scrollTimer) / scrollTimerMax;
            SetGpuReg(REG_OFFSET_BG2VOFS, sGTSPokedexView->initialVOffset + sGTSPokedexView->listMovingVOffset * LIST_SCROLL_STEP - step);
            break;
        case 2: // Down
            for (i = 0; i < MAX_MONS_ON_SCREEN; i++)
            {
                if (sGTSPokedexView->monSpriteIds[i] != 0xFFFF)
                    gSprites[sGTSPokedexView->monSpriteIds[i]].data[5] -= monMoveIncrement;
            }
            step = LIST_SCROLL_STEP * (scrollTimerMax - sGTSPokedexView->scrollTimer) / scrollTimerMax;
            SetGpuReg(REG_OFFSET_BG2VOFS, sGTSPokedexView->initialVOffset + sGTSPokedexView->listMovingVOffset * LIST_SCROLL_STEP + step);
            break;
        }
        return FALSE;
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG2VOFS, sGTSPokedexView->initialVOffset + sGTSPokedexView->listVOffset * LIST_SCROLL_STEP);
        return TRUE;
    }
}

void ZeroSearchResults(struct GTSResult *searchResults)
{
    u8 *raw = (u8 *)searchResults;
    u32 i;
    for (i = 0; i < sizeof(struct GTSResult); i++)
        raw[i] = 0;
}

static void VBlankCB_MysteryGiftEReader(void)
{
    ProcessSpriteCopyRequests();
    LoadOam();
    TransferPlttBuffer();
}

void CB2_GlobalTradeStation(void)
{
    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void LoadMysteryGiftTextboxBorder(u8 bgId)
{
    DecompressAndLoadBgGfxUsingHeap(bgId, sTextboxBorder_Gfx, 0x100, 0, 0);
}

static bool32 HandleGlobalTradeStationSetup()
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        //ResetPaletteFade();
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetTasks();
        ScanlineEffect_Stop();
        ResetBgsAndClearDma3BusyFlags(0);

        InitBgsFromTemplates(0, sBGTemplates, ARRAY_COUNT(sBGTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 0, BG_COORD_SET);

        SetBgTilemapBuffer(3, Alloc(BG_SCREEN_SIZE));
        SetBgTilemapBuffer(2, Alloc(BG_SCREEN_SIZE));
        SetBgTilemapBuffer(1, Alloc(BG_SCREEN_SIZE));
        SetBgTilemapBuffer(0, Alloc(BG_SCREEN_SIZE));

        LoadMysteryGiftTextboxBorder(3);
        InitWindows(sMainWindows);
        DeactivateAllTextPrinters();
        ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        gMain.state++;
        break;
    case 1:
        LoadPalette(sTextboxBorder_Pal, 0, 0x20);
        LoadPalette(GetTextWindowPalette(2), 0xd0, 0x20);
        Menu_LoadStdPalAt(0xC0);
        LoadUserWindowBorderGfx(0, 0xA, 0xE0);
        LoadUserWindowBorderGfx_(0, 0x1, 0xF0);
        FillBgTilemapBufferRect(0, 0x000, 0, 0, 32, 32, 0x11);
        FillBgTilemapBufferRect(1, 0x000, 0, 0, 32, 32, 0x11);
        FillBgTilemapBufferRect(2, 0x000, 0, 0, 32, 32, 0x11);
        GTS_DrawCheckerboardPattern(3);
        PrintGTSTopMenu(0, FALSE);
        gMain.state++;
        break;
    case 2:
        CopyBgTilemapBufferToVram(3);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(0);
        gMain.state++;
        break;
    case 3:
        ShowBg(0);
        ShowBg(3);
        FadeInFromBlack();
        SetVBlankCallback(VBlankCB_MysteryGiftEReader);
        EnableInterrupts(INTR_FLAG_VBLANK | INTR_FLAG_VCOUNT | INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
        return TRUE;
    }

    return FALSE;
}

void CB2_InitGlobalTradeStation(void)
{
    if (HandleGlobalTradeStationSetup())
    {
        if(VarGet(VAR_UNUSED_0x40FF)==0)
            FadeInNewBGM(MUS_RG_MYSTERY_GIFT,4);
        SetMainCallback2(CB2_GlobalTradeStation);
        CreateGlobalTradeStationTask();
    }
    RunTasks();
}
/*
void CB2_ReturnToGlobalTradeStation(void)
{
    if (HandleGlobalTradeStationSetup())
    {
        //gSpecialVar_0x8004 = GetCursorSelectionMonId();
        SetMainCallback2(CB2_GlobalTradeStation);
        RecreateGlobalTradeStationTask();
    }
    RunTasks();
}

void CB2_ReturnToGlobalTradeStationFromSummary(void)
{
    if (HandleGlobalTradeStationSetup())
    {
        //gSpecialVar_0x8004 = GetCursorSelectionMonId();
        SetMainCallback2(CB2_GlobalTradeStation);
        ReRecreateGlobalTradeStationTask();
    }
    RunTasks();
}*/

void MainCB_GTSFreeAllBuffersAndReturnToInitTitleScreen(void)
{
    FreeAllWindowBuffers();
    Free(GetBgTilemapBuffer(0));
    Free(GetBgTilemapBuffer(1));
    Free(GetBgTilemapBuffer(2));
    Free(GetBgTilemapBuffer(3));
    //SetWarpDestination(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum, 2, gSaveBlock1Ptr->pos.x, gSaveBlock1Ptr->pos.y);
    FadeInNewBGM(MUS_POKE_CENTER, 4);
    SetMainCallback2(CB2_ReturnToFieldContinueScript);
    //WarpIntoMap();
}

void PrintGTSTopMenu(u8 menuScreen, bool32 useCancel)
{
    const u8 * header;
    const u8 * options;
    FillWindowPixelBuffer(0, 0);

    switch(menuScreen){
        default:
        case 0:
            header = gText_GlobalTradeStation;
            options = !useCancel ? gText_PickOKExit : gText_PickOKCancel;
            break;
        case 1:
            header = gText_GlobalTradeStation;
            options = gText_PickPokemonOKCancel;
            break;
    }

    AddTextPrinterParameterized4(0, FONT_NORMAL, 4, 1, 0, 0, sTextColors_TopMenu, TEXT_SKIP_DRAW, header);
    AddTextPrinterParameterized4(0, FONT_SMALL, GetStringRightAlignXOffset(FONT_SMALL, options, 0xDE), 1, 0, 0, sTextColors_TopMenu, TEXT_SKIP_DRAW, options);
    CopyWindowToVram(0, COPYWIN_GFX);
    PutWindowTilemap(0);
}

void GTS_DrawTextBorder(u8 windowId)
{
    DrawTextBorderOuter(windowId, 0x01, 0xF);
}

void GTS_DrawCheckerboardPattern(u32 bg)
{
    s32 i = 0, j;

    FillBgTilemapBufferRect(bg, 0x003, 0, 0, 32, 2, 0x11);

    for (i = 0; i < 18; i++)
    {
        for (j = 0; j < 32; j++)
        {
            if ((i & 1) != (j & 1))
                FillBgTilemapBufferRect(bg, 1, j, i + 2, 1, 1, 0x11);
            else
                FillBgTilemapBufferRect(bg, 2, j, i + 2, 1, 1, 0x11);
        }
    }
}

static void ClearScreenInBg0(bool32 ignoreTopTwoRows)
{
    switch (ignoreTopTwoRows)
    {
    case 0:
        FillBgTilemapBufferRect(0, 0, 0, 0, 32, 32, 0x11);
        break;
    case 1:
        FillBgTilemapBufferRect(0, 0, 0, 2, 32, 30, 0x11);
        break;
    }
    CopyBgTilemapBufferToVram(0);
}

void GTSAddTextPrinterToWindow1(const u8 *str)
{
    StringExpandPlaceholders(gStringVar4, str);
    FillWindowPixelBuffer(1, 0x11);
    AddTextPrinterParameterized4(1, FONT_NORMAL, 0, 1, 0, 0, sGTS_Ereader_TextColor_2, 0, gStringVar4);
    DrawTextBorderOuter(1, 0x001, 0xF);
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_FULL);
}

void GTSAddWantedToWindow1(u8 selectedMon)
{
    FillWindowPixelBuffer(1, 0x11);
    AddTextPrinterParameterized4(1, FONT_NORMAL, 0, 1, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_Wanted);
    AddTextPrinterParameterized4(1, FONT_NORMAL, 8, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, GetSpeciesName(sGTSPokedexView->searchResult[selectedMon].natDexRequest));
    if(sGTSPokedexView->searchResult[selectedMon].minLevel==1){
        if(sGTSPokedexView->searchResult[selectedMon].maxLevel==100){
            AddTextPrinterParameterized4(1, FONT_NORMAL, 72, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_AnyLevel);
        }
        else{
            AddTextPrinterParameterized4(1, FONT_NORMAL, 72, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_UnderLevel10);
        }
    }
    else if(sGTSPokedexView->searchResult[selectedMon].minLevel==10){
        AddTextPrinterParameterized4(1, FONT_NORMAL, 72, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_AboveLevel10);
    }
    else if(sGTSPokedexView->searchResult[selectedMon].minLevel==20){
        AddTextPrinterParameterized4(1, FONT_NORMAL, 72, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_AboveLevel20);
    }
    else if(sGTSPokedexView->searchResult[selectedMon].minLevel==30){
        AddTextPrinterParameterized4(1, FONT_NORMAL, 72, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_AboveLevel30);
    }
    else if(sGTSPokedexView->searchResult[selectedMon].minLevel==40){
        AddTextPrinterParameterized4(1, FONT_NORMAL, 72, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_AboveLevel40);
    }
    else if(sGTSPokedexView->searchResult[selectedMon].minLevel==50){
        AddTextPrinterParameterized4(1, FONT_NORMAL, 72, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_AboveLevel50);
    }
    else if(sGTSPokedexView->searchResult[selectedMon].minLevel==60){
        AddTextPrinterParameterized4(1, FONT_NORMAL, 72, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_AboveLevel60);
    }
    else if(sGTSPokedexView->searchResult[selectedMon].minLevel==70){
        AddTextPrinterParameterized4(1, FONT_NORMAL, 72, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_AboveLevel70);
    }
    else if(sGTSPokedexView->searchResult[selectedMon].minLevel==80){
        AddTextPrinterParameterized4(1, FONT_NORMAL, 72, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_AboveLevel80);
    }
    else {
        AddTextPrinterParameterized4(1, FONT_NORMAL, 72, 17, 0, 0, sGTS_Ereader_TextColor_2, 0, gText_AboveLevel90);
    }
    DrawTextBorderOuter(1, 0x001, 0xF);
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_FULL);
}

static void ClearTextWindow(void)
{
    rbox_fill_rectangle(1);
    ClearWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_MAP);
}

#define DOWN_ARROW_X 208
#define DOWN_ARROW_Y 20

bool32 PrintGTSMenuMessage(u8 *textState, const u8 *str)
{
    switch (*textState)
    {
    case 0:
        GTSAddTextPrinterToWindow1(str);
        (*textState)++;
        break;
    case 1:
        DrawDownArrow(1, DOWN_ARROW_X, DOWN_ARROW_Y, 1, FALSE, &sDownArrowCounterAndYCoordIdx[0], &sDownArrowCounterAndYCoordIdx[1]);
        if (({JOY_NEW(A_BUTTON);}))
            (*textState)++;
        if (({JOY_NEW(B_BUTTON);}))
            //(*textState)==3;
            return FALSE;
        break;
    case 2:
        DrawDownArrow(1, DOWN_ARROW_X, DOWN_ARROW_Y, 1, TRUE, &sDownArrowCounterAndYCoordIdx[0], &sDownArrowCounterAndYCoordIdx[1]);
        *textState = 0;
        //ClearTextWindow();
        return TRUE;
    case 3:
        //DrawDownArrow(1, DOWN_ARROW_X, DOWN_ARROW_Y, 1, TRUE, &sDownArrowCounterAndYCoordIdx[0], &sDownArrowCounterAndYCoordIdx[1]);
        //*textState = 3;
        //ClearTextWindow();
        return TRUE;
    case 0xFF:
        *textState = 2;
        return FALSE;
    }
    return FALSE;
}

static void HideDownArrow(void)
{
    DrawDownArrow(1, DOWN_ARROW_X, DOWN_ARROW_Y, 1, FALSE, &sDownArrowCounterAndYCoordIdx[0], &sDownArrowCounterAndYCoordIdx[1]);
}

static void ShowDownArrow(void)
{
    DrawDownArrow(1, DOWN_ARROW_X, DOWN_ARROW_Y, 1, TRUE, &sDownArrowCounterAndYCoordIdx[0], &sDownArrowCounterAndYCoordIdx[1]);
}

// Unused
static bool32 UNUSED HideDownArrowAndWaitButton(u8 * textState)
{
    switch (*textState)
    {
    case 0:
        HideDownArrow();
        if (JOY_NEW(A_BUTTON | B_BUTTON))
            (*textState)++;
        break;
    case 1:
        ShowDownArrow();
        *textState = 0;
        return TRUE;
    }
    return FALSE;
}

static bool32 PrintStringAndWait2Seconds(u8 * counter, const u8 * str)
{
    if (*counter == 0)
        GTSAddTextPrinterToWindow1(str);

    if (++(*counter) > 120)
    {
        *counter = 0;
        ClearTextWindow();
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static u32 GlobalTradeStation_HandleThreeOptionMenu(u8 * unused0, u16 * unused1, u8 whichMenu)
{
    struct ListMenuTemplate listMenuTemplate = sListMenuTemplate_ThreeOptions;
    struct WindowTemplate windowTemplate = sWindowTemplate_ThreeOptions;
    s32 width;
    s32 response;

    if (whichMenu == 0)
        listMenuTemplate.items = sListMenuItems_SearchDeposit;
    else
        listMenuTemplate.items = sListMenuItems_SearchWithdraw;

    width = Intl_GetListMenuWidth(&listMenuTemplate);
    if (width & 1)
        width++;

    windowTemplate.width = width;
    if (width < 30)
        windowTemplate.tilemapLeft = (30 - width) / 2;
    else
        windowTemplate.tilemapLeft = 0;

    response = DoMysteryGiftListMenu(&windowTemplate, &listMenuTemplate, 1, LIST_MENU_TILE_NUM, LIST_MENU_PAL_NUM);
    if (response != LIST_NOTHING_CHOSEN)
    {
        ClearWindowTilemap(2);
        CopyWindowToVram(2, COPYWIN_MAP);
    }
    return response;
}

s8 DoGTSYesNo(u8 * textState, u16 * windowId, bool8 yesNoBoxPlacement, const u8 * str)
{
    struct WindowTemplate windowTemplate;
    s8 input;

    switch (*textState)
    {
    case 0:
        // Print question message
        StringExpandPlaceholders(gStringVar4, str);
        if (yesNoBoxPlacement == 0)
            *windowId = AddWindow(&sWindowTemplate_YesNoMsg_Wide);
        else
            *windowId = AddWindow(&sWindowTemplate_YesNoMsg);
        FillWindowPixelBuffer(*windowId, 0x11);
        AddTextPrinterParameterized4(*windowId, FONT_NORMAL, 0, 1, 0, 0, sGTS_Ereader_TextColor_2, 0, gStringVar4);
        DrawTextBorderOuter(*windowId, 0x001, 0x0F);
        CopyWindowToVram(*windowId, COPYWIN_GFX);
        PutWindowTilemap(*windowId);
        (*textState)++;
        break;
    case 1:
        // Create Yes/No
        windowTemplate = sWindowTemplate_YesNoBox;
        if (yesNoBoxPlacement == 0)
            windowTemplate.tilemapTop = 9;
        else
            windowTemplate.tilemapTop = 15;
        CreateYesNoMenu(&windowTemplate, 10, 14, 0);
        (*textState)++;
        break;
    case 2:
        // Handle Yes/No input
        input = Menu_ProcessInputNoWrapClearOnChoose();
        if (input == MENU_B_PRESSED || input == 0 || input == 1)
        {
            *textState = 0;
            rbox_fill_rectangle(*windowId);
            ClearWindowTilemap(*windowId);
            CopyWindowToVram(*windowId, COPYWIN_MAP);
            RemoveWindow(*windowId);
            return input;
        }
        break;
    case 0xFF:
        *textState = 0;
        rbox_fill_rectangle(*windowId);
        ClearWindowTilemap(*windowId);
        CopyWindowToVram(*windowId, COPYWIN_MAP);
        RemoveWindow(*windowId);
        return MENU_B_PRESSED;
    }

    return MENU_NOTHING_CHOSEN;
}

// Handle the "Receive/Send/Toss" menu that appears when selecting Wonder Card/News
static s32 HandleGiftSelectMenu(u8 * textState, u16 * windowId, bool32 cannotToss, bool32 cannotSend)
{
    //struct WindowTemplate windowTemplate;
    s32 input;

    switch (*textState)
    {
    case 0:
        // Print menu message
        if (!cannotToss)
            StringExpandPlaceholders(gStringVar4, gText_WhatToDoWithCards);
        else
            StringExpandPlaceholders(gStringVar4, gText_WhatToDoWithNews);
        *windowId = AddWindow(&sWindowTemplate_GiftSelect);
        FillWindowPixelBuffer(*windowId, 0x11);
        AddTextPrinterParameterized4(*windowId, FONT_NORMAL, 0, 1, 0, 0, sGTS_Ereader_TextColor_2, 0, gStringVar4);
        DrawTextBorderOuter(*windowId, 0x001, 0x0F);
        CopyWindowToVram(*windowId, COPYWIN_GFX);
        PutWindowTilemap(*windowId);
        (*textState)++;
        break;
    case 1:
        //windowTemplate = sWindowTemplate_YesNoBox;
        if (cannotSend)
        {
            if (!cannotToss)
                input = DoMysteryGiftListMenu(&sWindowTemplate_GiftSelect_2Options, &sListMenu_ReceiveToss, 1, LIST_MENU_TILE_NUM, LIST_MENU_PAL_NUM);
            else
                input = DoMysteryGiftListMenu(&sWindowTemplate_GiftSelect_1Option, &sListMenu_Receive, 1, LIST_MENU_TILE_NUM, LIST_MENU_PAL_NUM);
        }
        else
        {
            if (!cannotToss)
                input = DoMysteryGiftListMenu(&sWindowTemplate_GiftSelect_3Options, &sListMenu_ReceiveSendToss, 1, LIST_MENU_TILE_NUM, LIST_MENU_PAL_NUM);
            else
                input = DoMysteryGiftListMenu(&sWindowTemplate_GiftSelect_2Options, &sListMenu_ReceiveSend, 1, LIST_MENU_TILE_NUM, LIST_MENU_PAL_NUM);
        }
        if (input != LIST_NOTHING_CHOSEN)
        {
            *textState = 0;
            rbox_fill_rectangle(*windowId);
            ClearWindowTilemap(*windowId);
            CopyWindowToVram(*windowId, COPYWIN_MAP);
            RemoveWindow(*windowId);
            return input;
        }
        break;
    case 0xFF:
        *textState = 0;
        rbox_fill_rectangle(*windowId);
        ClearWindowTilemap(*windowId);
        CopyWindowToVram(*windowId, COPYWIN_MAP);
        RemoveWindow(*windowId);
        return LIST_CANCEL;
    }

    return LIST_NOTHING_CHOSEN;
}

static bool32 HandleLoadWonderCardOrNews(u8 * state, bool32 isWonderNews)
{
    switch (*state)
    {
    case 0:
        if (!isWonderNews)
            WonderCard_Init(GetSavedWonderCard(), GetSavedWonderCardMetadata());
        else
            WonderNews_Init(GetSavedWonderNews());
        (*state)++;
        break;
    case 1:
        if (!isWonderNews)
        {
            if (!WonderCard_Enter())
                return FALSE;
        }
        else
        {
            if (!WonderNews_Enter())
                return FALSE;
        }
        *state = 0;
        return TRUE;
    }

    return FALSE;
}

static bool32 ClearSavedNewsOrCard(bool32 isWonderNews)
{
    if (!isWonderNews)
        ClearSavedWonderCardAndRelated();
    else
        ClearSavedWonderNewsAndRelated();
    return TRUE;
}

static bool32 ExitWonderCardOrNews(bool32 isWonderNews, bool32 useCancel)
{
    if (!isWonderNews)
    {
        if (WonderCard_Exit(useCancel))
        {
            WonderCard_Destroy();
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        if (WonderNews_Exit(useCancel))
        {
            WonderNews_Destroy();
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

static s32 AskDiscardGift(u8 * textState, u16 * windowId, bool32 isWonderNews)
{
    if (!isWonderNews)
        return DoGTSYesNo(textState, windowId, TRUE, gText_IfThrowAwayCardEventWontHappen);
    else
        return DoGTSYesNo(textState, windowId, TRUE, gText_OkayToDiscardNews);
}

static bool32 PrintThrownAway(u8 * textState, bool32 isWonderNews)
{
    if (!isWonderNews)
        return PrintGTSMenuMessage(textState, gText_WonderCardThrownAway);
    else
        return PrintGTSMenuMessage(textState, gText_WonderNewsThrownAway);
}

static bool32 SaveOnMysteryGiftMenu(u8 * state)
{
    switch (*state)
    {
    case 0:
        GTSAddTextPrinterToWindow1(gText_DataWillBeSaved);
        (*state)++;
        break;
    case 1:
        TrySavingData(SAVE_NORMAL);
        (*state)++;
        break;
    case 2:
        GTSAddTextPrinterToWindow1(gText_SaveCompletedPressA);
        (*state)++;
        break;
    case 3:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
            (*state)++;
        break;
    case 4:
        *state = 0;
        ClearTextWindow();
        return TRUE;
    }

    return FALSE;
}

static const u8 * GetClientResultMessage(bool32 * successMsg, bool8 isWonderNews, bool8 sourceIsFriend, u32 msgId)
{
    const u8 * msg = NULL;
    *successMsg = FALSE;

    switch (msgId)
    {
    case CLI_MSG_NOTHING_SENT:
        *successMsg = FALSE;
        msg = gText_NothingSentOver;
        break;
    case CLI_MSG_RECORD_UPLOADED:
        *successMsg = FALSE;
        msg = gText_RecordUploadedViaWireless;
        break;
    case CLI_MSG_CARD_RECEIVED:
        *successMsg = TRUE;
        msg = !sourceIsFriend ? gText_WonderCardReceived : gText_WonderCardReceivedFrom;
        break;
    case CLI_MSG_NEWS_RECEIVED:
        *successMsg = TRUE;
        msg = !sourceIsFriend ? gText_WonderNewsReceived : gText_WonderNewsReceivedFrom;
        break;
    case CLI_MSG_STAMP_RECEIVED:
        *successMsg = TRUE;
        msg = gText_NewStampReceived;
        break;
    case CLI_MSG_HAD_CARD:
        *successMsg = FALSE;
        msg = gText_AlreadyHadCard;
        break;
    case CLI_MSG_HAD_STAMP:
        *successMsg = FALSE;
        msg = gText_AlreadyHadStamp;
        break;
    case CLI_MSG_HAD_NEWS:
        *successMsg = FALSE;
        msg = gText_AlreadyHadNews;
        break;
    case CLI_MSG_NO_ROOM_STAMPS:
        *successMsg = FALSE;
        msg = gText_NoMoreRoomForStamps;
        break;
    case CLI_MSG_COMM_CANCELED:
        *successMsg = FALSE;
        msg = gText_CommunicationCanceled;
        break;
    case CLI_MSG_CANT_ACCEPT:
        *successMsg = FALSE;
        msg = !isWonderNews ? gText_CantAcceptCardFromTrainer : gText_CantAcceptNewsFromTrainer;
        break;
    case CLI_MSG_COMM_ERROR:
        *successMsg = FALSE;
        msg = gText_CommunicationError;
        break;
    case CLI_MSG_TRAINER_RECEIVED:
        *successMsg = TRUE;
        msg = gText_NewTrainerReceived;
        break;
    case CLI_MSG_BUFFER_SUCCESS:
        *successMsg = TRUE;
        // msg is NULL, use buffer
        break;
    case CLI_MSG_BUFFER_FAILURE:
        *successMsg = FALSE;
        // msg is NULL, use buffer
        break;
    }

    return msg;
}

static bool32 PrintSuccessMessage(u8 * state, const u8 * msg, u16 * timer)
{
    switch (*state)
    {
    case 0:
        if (msg != NULL)
            GTSAddTextPrinterToWindow1(msg);
        PlayFanfare(MUS_OBTAIN_ITEM);
        *timer = 0;
        (*state)++;
        break;
    case 1:
        if (++(*timer) > 240)
            (*state)++;
        break;
    case 2:
        if (IsFanfareTaskInactive())
        {
            *state = 0;
            ClearTextWindow();
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static bool32 PrintFailureMessage(u8 * state, const u8 * msg, u16 * timer)
{
    switch (*state)
    {
    case 0:
        if (msg != NULL)
            GTSAddTextPrinterToWindow1(msg);
        PlaySE(SE_FAILURE);
        *timer = 0;
        (*state)++;
        break;
    case 1:
        if (++(*timer) > 120)
            (*state)++;
        break;
    case 2:
        *state = 0;
        ClearTextWindow();
        return TRUE;
    }
    return FALSE;
}

static const u8 * GetServerResultMessage(bool32 * wonderSuccess, bool8 sourceIsFriend, u32 msgId)
{
    const u8 * result = gText_CommunicationError;
    *wonderSuccess = FALSE;
    switch (msgId)
    {
    case SVR_MSG_NOTHING_SENT:
        result = gText_NothingSentOver;
        break;
    case SVR_MSG_RECORD_UPLOADED:
        result = gText_RecordUploadedViaWireless;
        break;
    case SVR_MSG_CARD_SENT:
        result = gText_WonderCardSentTo;
        *wonderSuccess = TRUE;
        break;
    case SVR_MSG_NEWS_SENT:
        result = gText_WonderNewsSentTo;
        *wonderSuccess = TRUE;
        break;
    case SVR_MSG_STAMP_SENT:
        result = gText_StampSentTo;
        break;
    case SVR_MSG_HAS_CARD:
        result = gText_OtherTrainerHasCard;
        break;
    case SVR_MSG_HAS_STAMP:
        result = gText_OtherTrainerHasStamp;
        break;
    case SVR_MSG_HAS_NEWS:
        result = gText_OtherTrainerHasNews;
        break;
    case SVR_MSG_NO_ROOM_STAMPS:
        result = gText_NoMoreRoomForStamps;
        break;
    case SVR_MSG_CLIENT_CANCELED:
        result = gText_OtherTrainerCanceled;
        break;
    case SVR_MSG_CANT_SEND_GIFT_1:
        result = gText_CantSendGiftToTrainer;
        break;
    case SVR_MSG_COMM_ERROR:
        result = gText_CommunicationError;
        break;
    case SVR_MSG_GIFT_SENT_1:
        result = gText_GiftSentTo;
        break;
    case SVR_MSG_GIFT_SENT_2:
        result = gText_GiftSentTo;
        break;
    case SVR_MSG_CANT_SEND_GIFT_2:
        result = gText_CantSendGiftToTrainer;
        break;
    }
    return result;
}

static bool32 UNUSED PrintServerResultMessage(u8 * state, u16 * timer, bool8 sourceIsFriend, u32 msgId)
{
    bool32 wonderSuccess;
    const u8 * str = GetServerResultMessage(&wonderSuccess, sourceIsFriend, msgId);
    if (wonderSuccess)
        return PrintSuccessMessage(state, str, timer);
    else
        return PrintGTSMenuMessage(state, str);
}

// States for Task_MysteryGift.
// CLIENT states are for when the player is receiving a gift, and use mystery_gift_client.c link functions.
// SERVER states are for when the player is sending a gift, and use mystery_gift_server.c link functions.
// Other states handle the general Mystery Gift menu usage.
enum {
    GTS_STATE_TO_MAIN_MENU,
    GTS_CONNECT_TO_SERVER,
    GTS_PING_SERVER,
    GTS_CHECK_RESULT,
    GTS_RECEIVE_POKEMON,
    GTS_RECEIVED_POKEMON,
    GTS_STATE_MAIN_MENU,
    GTS_STATE_TRADE_WITH_THIS_PERSON,
    GTS_STATE_CHOOSE_EXCHANGE,
    GTS_STATE_CANCEL_SEARCH,
    GTS_STATE_CONFIRM_EXCHANGE,
    GTS_STATE_EXCHANGE_ANIMATION,
    GTS_STATE_TRADE_ANIMATION,
    GTS_STATE_SAVE_1,
    GTS_STATE_SAVE_2,
    GTS_STATE_SAVE_3,
    GTS_STATE_SAVE_4,
    GTS_STATE_SAVE_5,
    GTS_STATE_SAVE_POST_FINISH,
    GTS_STATE_SAVE_EXCHANGE_FINISH,
    GTS_STATE_SAVE_EXCHANGED_POKEMON,
    GTS_STATE_RETRIEVE_POKEMON_YES_NO,
    GTS_STATE_SAVE_RETRIEVED_POKEMON,
    GTS_STATE_SEEK_SETUP,
    GTS_STATE_SEEKING,
    GTS_STATE_SEARCH_POKEMON,
    GTS_STATE_SEARCH_POKEMON_LIST,
    GTS_STATE_SEARCH_POKEMON_GENDER,
    GTS_STATE_SEARCH_POKEMON_LEVEL_LIST,
    GTS_STATE_FETCHING_POKEMON,
    GTS_STATE_FETCHED_POKEMON_SETUP,
    GTS_POKEMON_NOT_FOUND,
    GTS_STATE_SELECT_FETCHED_POKEMON,
    GTS_STATE_START_SEARCH,
    GTS_STATE_SUCCESSFUL_SEARCH,
    GTS_STATE_UNSUCCESSFUL_SEARCH,
    GTS_STATE_FIND_MATCH,
    GTS_STATE_TRADE_MATCH,
    GTS_STATE_DEPOSIT_POKEMON,
    GTS_STATE_PICK_WANTED_POKEMON,
    GTS_STATE_DEPOSITING_POKEMON,
    GTS_STATE_POKEMON_LIST,
    GTS_STATE_POKEMON_LEVEL_LIST,
    GTS_STATE_RETURN_POKEMON_LIST,
    GTS_STATE_CONFIRM_OFFER,
    GTS_STATE_WAIT,
    GTS_STATE_WITHDRAW_POKEMON,
    GTS_STATE_CLIENT_LINK_START,
    GTS_STATE_CLIENT_LINK_WAIT,
    GTS_STATE_CLIENT_COMMUNICATING,
    GTS_STATE_CLIENT_LINK,
    GTS_STATE_CLIENT_YES_NO,
    GTS_STATE_CLIENT_MESSAGE,
    GTS_STATE_CLIENT_ASK_TOSS,
    GTS_STATE_CLIENT_ASK_TOSS_UNRECEIVED,
    GTS_STATE_CLIENT_LINK_END,
    GTS_STATE_CLIENT_COMM_COMPLETED,
    GTS_STATE_CLIENT_RESULT_MSG,
    GTS_STATE_CLIENT_ERROR,
    GTS_STATE_SAVE_LOAD_GIFT,
    GTS_STATE_LOAD_GIFT,
    GTS_STATE_UNUSED,
    GTS_STATE_HANDLE_GIFT_INPUT,
    GTS_STATE_HANDLE_GIFT_SELECT,
    GTS_STATE_ASK_TOSS,
    GTS_STATE_ASK_TOSS_UNRECEIVED,
    GTS_STATE_TOSS,
    GTS_STATE_TOSS_SAVE,
    GTS_STATE_TOSSED,
    GTS_STATE_GIFT_INPUT_EXIT,
    GTS_STATE_RECEIVE,
    GTS_STATE_SEND,
    GTS_STATE_SERVER_LINK_WAIT,
    GTS_STATE_SERVER_LINK_START,
    GTS_STATE_SERVER_LINK,
    GTS_STATE_SERVER_LINK_END,
    GTS_STATE_SERVER_LINK_END_WAIT,
    GTS_STATE_SERVER_RESULT_MSG,
    GTS_STATE_SERVER_ERROR,
    GTS_STATE_EXIT,
    GTS_STATE_SOURCE_PROMPT,
    GTS_STATE_SOURCE_PROMPT_INPUT,
};

static void CreateGlobalTradeStationTask(void)
{
    u8 taskId = CreateTask(Task_GlobalTradeStation, 0);
    struct GlobalTradeStationTaskData * data = (void *)gTasks[taskId].data;
    data->state = VarGet(VAR_UNUSED_0x40FF); //GTS_STATE_TO_MAIN_MENU;
    data->textState = 0;
    if(data->state==GTS_CHECK_RESULT)
        data->nextState = GTS_STATE_MAIN_MENU;
    else
        data->nextState = 0;
    data->monDeposited = 0;
    data->isWonderNews = 0;
    data->sourceIsFriend = 0;
    data->var = 0;
    data->depositPokemon = 0;
    data->searchPokemon = 0;
    data->errorNum = 0;
    data->msgId = 0;
    data->clientMsg = AllocZeroed(CLIENT_MAX_MSG_SIZE);
    if(data->state==GTS_STATE_TO_MAIN_MENU)
        sGTSPokedexView = AllocZeroed(sizeof(struct GTSPokedexView));
    if(data->state!=GTS_STATE_CONFIRM_EXCHANGE)
        ResetPokedexViewGTS(sGTSPokedexView);
    if(data->state==GTS_STATE_PICK_WANTED_POKEMON){
        sGTSPokedexView->windowid = AddWindow(&sWindowTemplate_PokemonSelect); //Add Pokemon list box (empty for now)
        FillWindowPixelBuffer(sGTSPokedexView->windowid, 0x11);
    }
    else if(data->state==GTS_STATE_RETRIEVE_POKEMON_YES_NO){
        sGTSPokedexView->windowid = AddWindow(&sWindowTemplate_PokemonSelect); //Add Pokemon list box (empty for now)
        FillWindowPixelBuffer(sGTSPokedexView->windowid, 0x11);
    }
}
/*
static void RecreateGlobalTradeStationTask(void)
{
    u8 taskId = CreateTask(Task_GlobalTradeStation, 0);
    struct GlobalTradeStationTaskData * data = (void *)gTasks[taskId].data;
    data->state = GTS_STATE_PICK_WANTED_POKEMON;
    data->textState = 0;
    data->nextState = 0;
    data->monDeposited = 0;
    data->isWonderNews = 0;
    data->sourceIsFriend = 0;
    data->var = 0;
    data->depositPokemon = 0;
    data->searchPokemon = 0;
    data->errorNum = 0;
    data->msgId = 0;
    data->clientMsg = AllocZeroed(CLIENT_MAX_MSG_SIZE);
    sGTSPokedexView = AllocZeroed(sizeof(struct GTSPokedexView));
    ResetPokedexViewGTS(sGTSPokedexView);
    sGTSPokedexView->windowid = AddWindow(&sWindowTemplate_PokemonSelect); //Add Pokemon list box (empty for now)
    FillWindowPixelBuffer(sGTSPokedexView->windowid, 0x11);
    //FillWindowPixelRect(sGTSPokedexView->windowid, PIXEL_FILL(1), 0, 0, 8, 16);
    //AddTextPrinterParameterized4(sGTSPokedexView->windowid, FONT_NORMAL, 0, 1, 0, 0, sGTS_Ereader_TextColor_2, 0, gStringVar4);
    //DrawTextBorderOuter(sGTSPokedexView->windowid, 0x001, 0x0F);
    //CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_MAP);
    //CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
    //PutWindowTilemap(sGTSPokedexView->windowid);
}

static void ReRecreateGlobalTradeStationTask(void)
{
    u8 taskId = CreateTask(Task_GlobalTradeStation, 0);
    struct GlobalTradeStationTaskData * data = (void *)gTasks[taskId].data;
    data->state = GTS_STATE_RETRIEVE_POKEMON_YES_NO;
    data->textState = 0;
    data->nextState = 0;
    data->monDeposited = 0;
    data->isWonderNews = 0;
    data->sourceIsFriend = 0;
    data->var = 0;
    data->depositPokemon = 0;
    data->searchPokemon = 0;
    data->errorNum = 0;
    data->msgId = 0;
    data->clientMsg = AllocZeroed(CLIENT_MAX_MSG_SIZE);
    sGTSPokedexView = AllocZeroed(sizeof(struct GTSPokedexView));
    ResetPokedexViewGTS(sGTSPokedexView);
    sGTSPokedexView->windowid = AddWindow(&sWindowTemplate_PokemonSelect); //Add Pokemon list box (empty for now)
    FillWindowPixelBuffer(sGTSPokedexView->windowid, 0x11);
}*/


static void Task_GlobalTradeStation(u8 taskId)
{
    //DebugPrintf("BuffedXXX");
    struct GlobalTradeStationTaskData *data = (void *)gTasks[taskId].data;
    u32 successMsg, input;
    const u8 *msg;
    u8 i = 0;
    bool8 j = TRUE;
    char pUserID[32];  //User ID from the MA EEPROM, has max lenght of 32
    pUserID[0]='\0';
    char pPassword[16];    //User password from the MA EEPROM, has max lenght of 16
    pPassword[0]='\0';
    char maMailID[30]; //User mail ID from the MA EEPROM
    u16 recvBufSize = 100;  //Size of received data
    u8 pRecvData[recvBufSize];  //Buffer to hold received data
    u16 pRecvSize;  //How many bytes were copied to pRecvData after calling maUpload once
    char pURL[1024];
    memcpy(pURL, "\0", 1);
    char halftoken[53];
    char hash[41];
    MA_TELDATA maTel;   //MA Telephone struct
    //char *encoded_data = 0;
    char encoded_data[173];
    //char pid[5];
    u8 pidhex[8];
    //DebugPrintf("BuffedXXX");

    switch (data->state)
    {
    case GTS_STATE_TO_MAIN_MENU: //Done
        DebugPrintf("INTERNET_MA_CONNECTED");
        if (!gPaletteFade.active)
            {
            if(maConnected()){
                data->state = GTS_CONNECT_TO_SERVER;
            }
            else{
                data->state = GTS_STATE_CLIENT_ERROR;
            }
        }
        //data->state = GTS_STATE_MAIN_MENU;
        break;
    case GTS_CONNECT_TO_SERVER: //Done
        DebugPrintf("INTERNET_CONNECT_TO_SERVER");
        //Initialise MA Library
        data->errorNum = maInitLibrary();

        //If there was an error stop MA Library and return the error code
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }
        
        //Get EEPROM Data
        data->errorNum=maGetEEPROMData(&maTel, pUserID, maMailID);

        //If there was an error stop MA Library and return the error code
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }

        //Set your password, must end in Null byte
        memcpy(pPassword,"password1", 10);

        //Makes a call and establishes a PPP connection 
        data->errorNum=maConnectServer(&maTel,pUserID,pPassword);

        //If there was an error stop MA Library and return the error code
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }

        data->state = GTS_PING_SERVER;
        break;
    case GTS_PING_SERVER: //Done
        DebugPrintf("INTERNET_PING_SERVER");
        recvBufSize=4;
        concat_str(pURL,"http://gts.paccypad.com/pokemonrse/worldexchange/info\0");
        //Request a PID
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, pUserID, pPassword);
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }

        recvBufSize=(pRecvData[0]<<8)+pRecvData[1];
        DebugPrintf("%u\n", recvBufSize);
        DebugPrintf("%u\n", *pRecvData);

        if(recvBufSize==1){
            data->state = GTS_CHECK_RESULT;
            data->nextState = GTS_STATE_MAIN_MENU;
            DebugPrintf("Test");
        }
        else
            data->state = GTS_STATE_CLIENT_ERROR;
        
        break;
    case GTS_CHECK_RESULT: //Done
        DebugPrintf("GTS_CHECK_RESULT");
        ResetPokedexViewGTS(sGTSPokedexView);
        recvBufSize=32;
        concat_str(pURL,"http://gts.paccypad.com/pokemonrse/worldexchange/result?pid=\0");
        DebugPrintf("Test 1");
        //Turn hex to str
        ConvertIntToHexStringN_v2(pidhex, gSaveBlock2Ptr->PID,STR_CONV_MODE_RIGHT_ALIGN,8);

        //Add PID to URL
        concat_str(pURL,(char *)pidhex);
        
        DebugPrintf(pURL);
        
        DebugPrintf("Test 2");
        //Initial Profile Setup
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }
        DebugPrintf("%u",pRecvSize);



        recvBufSize=0x92;
        memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
        concat_str(halftoken,(char *)pRecvData);

        //Cleaning up pRecvData
        for(i=0;i<32;i++){
            pRecvData[i]='\0';
        }

        sha1digest((u8 *)hash,NULL,(u8 *)halftoken,52);

        //Add hash to URL
        concat_str(pURL,"&hash=");
        for(i = 0; i < 20; i++){
            ConvertIntToHexStringN_v2(pidhex, hash[i],STR_CONV_MODE_LEFT_ALIGN,2);
            pidhex[2]='\0';
            concat_str(pURL,(char *)pidhex);
        }
        //concat_str(pURL,hash);
        DebugPrintf(pURL);
        //concat_str(pURL,"\0");
        recvBufSize=0x7C;
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        //(u8 *)sGTSPokedexView->searchResult[0].checksum=pRecvData
        DebugPrintf("a");
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }
        DebugPrintf("b");
        DebugPrintf("%u",pRecvSize);
        DebugPrintf("%u",pRecvData[0]);
        DebugPrintf("%u",pRecvData[1]);
        DebugPrintf("%u",&pRecvData);

        if(pRecvSize==2){
            DebugPrintf("c");
            //int result = (pRecvData[0]<<8) + pRecvData[1];
            if(pRecvData[1]==0x5)
                data->monDeposited=0;
            else if(pRecvData[1]==0x4)
                data->monDeposited=1;
            else{
                data->state = GTS_STATE_CLIENT_ERROR;
                break;
            }

        }
        else if(pRecvSize==0x7C){
            DebugPrintf("d");
            memcpy(&sGTSPokedexView->searchResult[0].boxmon.personality,&pRecvData,80);
            memcpy(&gParties[B_TRAINER_OPPONENT_A][0].box,&sGTSPokedexView->searchResult[0].boxmon,80);
            data->state = GTS_RECEIVE_POKEMON;
            break;
        }
        else{
            DebugPrintf("e");
            data->state = GTS_STATE_CLIENT_ERROR;
        }
        DebugPrintf("f");
        data->state = data->nextState;
        if(data->state==0)
            data->state=GTS_STATE_MAIN_MENU;
        DebugPrintf("%u",(u32)pRecvData[1]);
        break;
    case GTS_RECEIVE_POKEMON: //Done
        //Check PC isn't full
        //if(GiveBoxMonToPlayer(&sGTSPokedexView->searchResult[1].boxmon)==2){
        //    data->state = GTS_STATE_MAIN_MENU;
        //    break;
        //}
        if(GiveBoxMonToPlayer(&gParties[B_TRAINER_OPPONENT_A][0].box)==2){
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }
        data->state = GTS_STATE_SAVE_1;
        data->nextState = GTS_RECEIVED_POKEMON;
        break;
    case GTS_RECEIVED_POKEMON: //Done
        recvBufSize=32;
        concat_str(pURL,"http://gts.paccypad.com/pokemonrse/worldexchange/delete?pid=\0");

        //Turn hex to str
        ConvertIntToHexStringN_v2(pidhex, gSaveBlock2Ptr->PID,STR_CONV_MODE_RIGHT_ALIGN,8);

        //Add PID to URL
        concat_str(pURL,(char *)pidhex);

        //Get hash
        DebugPrintf(pURL);
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }
        DebugPrintf("%u",pRecvSize);

        memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
        concat_str(halftoken,(char *)pRecvData);

        //Cleaning up pRecvData
        for(i=0;i<32;i++){
            pRecvData[i]='\0';
        }

        sha1digest((u8 *)hash,NULL,(u8 *)halftoken,52);

        //Add hash to URL
        concat_str(pURL,"&hash=");
        for(i = 0; i < 20; i++){
            ConvertIntToHexStringN_v2(pidhex, hash[i],STR_CONV_MODE_LEFT_ALIGN,2);
            pidhex[2]='\0';
            concat_str(pURL,(char *)pidhex);
        }

        DebugPrintf(pURL);
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }
        DebugPrintf("%u",pRecvSize);


        if(pRecvData[1]==0x01){
            LinkFullSave_SetLastSectorSignature();
            DebugPrintf("Successfully retreived");
            sGTSPokedexView->currentPage=3;
            VarSet(VAR_UNUSED_0x40FF,GTS_STATE_MAIN_MENU);
            DestroyTask(taskId);
            FreeAllWindowBuffers();
            DoGTSExchangeScene();
            // /data->state = GTS_STATE_MAIN_MENU;
        }
        else{
            DebugPrintf("Lol fail");
            data->state = GTS_STATE_SERVER_ERROR;
        }

        break;
    case GTS_STATE_MAIN_MENU:
        // Main Mystery Gift menu, player can select Wonder Cards or News (or exit)
        switch (GlobalTradeStation_HandleThreeOptionMenu(&data->textState, &data->var, data->monDeposited))
        {
        case 0: // "Search Pokemon"
            data->searchPokemon = 0;
            data->state = GTS_STATE_SEEK_SETUP;
            PlaySE(SE_SELECT);
            //ClearStdWindowAndFrame(0, TRUE);
            //ClearStdWindowAndFrame(1, TRUE);
            //ClearStdWindowAndFrame(2, TRUE);
            //ResetPokedexViewGTS(sGTSPokedexView);
            //RemoveWindow(2);
            //sGTSPokedexView->windowid = AddWindow(&sWindowTemplate_PokemonSelect);
            //FillWindowPixelBuffer(sGTSPokedexView->windowid, 0x11);
            //DrawTextBorderOuter(sGTSPokedexView->windowid, 0x001, 0x0F);
            //CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_MAP);
            //CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
            //PutWindowTilemap(sGTSPokedexView->windowid);
            break;
        case 1: // "Deposit Pokemon"
            //data->isWonderNews = TRUE;
            //if (data->depositPokemon == 0)
            if(data->monDeposited)
                data->state = GTS_STATE_WITHDRAW_POKEMON;
            else
                data->state = GTS_STATE_DEPOSIT_POKEMON;

            //FadeScreen(FADE_TO_BLACK, 0);
            //else
            //    data->state = GTS_STATE_WITHDRAW_POKEMON;
            PlaySE(SE_SELECT);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            break;
        case LIST_CANCEL:
            data->state = GTS_STATE_EXIT;
            break;
        }
        break;
    case GTS_STATE_SEEK_SETUP:
        sGTSPokedexView->windowid = AddWindow(&sWindowTemplate_PokemonSelect);
        FillWindowPixelBuffer(sGTSPokedexView->windowid, 0x11);
        DrawTextBorderOuter(sGTSPokedexView->windowid, 0x001, 0x0F);
        CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_MAP);
        CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
        PutWindowTilemap(sGTSPokedexView->windowid);
        data->state = GTS_STATE_SEEKING;
        break;
    case GTS_STATE_SEEKING:
        sGTSPokedexView->dexMode = DoGTSListMenu(&sWindowTemplate_ABCSelect, &sListMenu_ABCMenu, 1, LIST_MENU_TILE_NUM, LIST_MENU_PAL_NUM);
        if (sGTSPokedexView->dexMode == LIST_CANCEL) {
            ClearStdWindowAndFrame(sGTSPokedexView->windowid, FALSE);
            ClearStdWindowAndFrame(1, FALSE);
            data->textState = 0;
            sGTSPokedexView->dexMode = 0;
            PlaySE(SE_SELECT);
            //BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            data->state = GTS_STATE_MAIN_MENU;
        } 
        else if (PrintGTSMenuMessage(&data->textState, gText_ChooseGTSPokemon))
        {
            data->state = GTS_STATE_SEARCH_POKEMON;
            PrintGTSTopMenu(0, TRUE);
        }
        break;
    case GTS_STATE_SEARCH_POKEMON:
        input = DoPokedexSearchGTS(sGTSPokedexView->dexMode+1); //Gets alphabetical list of ABC option selected
        if(sGTSPokedexView->pokemonListCount != 0){
            CreateMonListEntryGTS(0, 0);
            data->state = GTS_STATE_SEARCH_POKEMON_LIST;
        }
        else {
            PlaySE(SE_FAILURE);
            data->state = GTS_STATE_SEEKING;
        }
        break;
    case GTS_STATE_SEARCH_POKEMON_LIST:
        sGTSPokedexView->selectedPokemon = TryDoPokedexScrollGTS(sGTSPokedexView->selectedPokemon);
        if (JOY_NEW(A_BUTTON))
        {
            sGTSPokedexView->selectedPokemon=GET_BASE_SPECIES_ID(sGTSPokedexView->pokedexList[sGTSPokedexView->selectedPokemon].dexNum);
            data->state = GTS_STATE_SEARCH_POKEMON_GENDER;
            FillWindowPixelRect(sGTSPokedexView->windowid, PIXEL_FILL(1), 0, 0, 80, 80);
            CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
            //ClearStdWindowAndFrame(3, FALSE);
            ClearStdWindowAndFrame(sGTSPokedexView->windowid, FALSE);
        }
        if (JOY_NEW(B_BUTTON))
        {
            data->state = GTS_STATE_SEEKING;
            FillWindowPixelRect(sGTSPokedexView->windowid, PIXEL_FILL(1), 0, 0, 80, 80);
            CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
            //RemoveWindow(0);
        }
        break;
    case GTS_STATE_SEARCH_POKEMON_GENDER:
        //sGTSPokedexView->dexOrder;
        s32 result = DoGTSListMenu(&sWindowTemplate_ThreeOptions, &sListMenuTemplate_Genders, 1, LIST_MENU_TILE_NUM, LIST_MENU_PAL_NUM);
        if (result == LIST_CANCEL) {
            DebugPrintf("Go back to GTS_STATE_SEEK_SETUP");
            PlaySE(SE_SELECT);
            ResetPokedexViewGTS(sGTSPokedexView);
            sGTSPokedexView->windowid = AddWindow(&sWindowTemplate_PokemonSelect); //Add Pokemon list box (empty for now)
            FillWindowPixelBuffer(sGTSPokedexView->windowid, 0x11);
            sGTSPokedexView->dexOrder = 0;
            data->textState = 0;
            data->state = GTS_STATE_SEEK_SETUP;
        }
        else if (PrintGTSMenuMessage(&data->textState, gText_ChooseGTSPokemonGender))
        {
            if(result==1){
                sGTSPokedexView->dexOrder=0;
            }
            else if(result==2){
                sGTSPokedexView->dexOrder=0xFE;
            }
            else{
                sGTSPokedexView->dexOrder=0xFF;
            }
            //GetMonData(mon, MON_DATA_NICKNAME, name);
            //StringCopy_Nickname(gStringVar1, gParties[B_TRAINER_PLAYER][sGTSPokedexView->offerPokemon].box.nickname);
            //DebugPrintf("yo");
            //DebugPrintf("%u",sGTSPokedexView->pokedexList[sGTSPokedexView->selectedPokemon].dexNum);
            StringCopy(gStringVar2, GetSpeciesName(sGTSPokedexView->selectedPokemon));
            //DebugPrintf("yo2");
            sGTSPokedexView->cursorRelPos = 0;
            sGTSPokedexView->atTop = 1;
            sGTSPokedexView->atBottom = 0;
            //sGTSPokedexView->selectedPokemon = 0;
            sGTSPokedexView->pokemonListCount = 0;
            sGTSPokedexView->scrollTimer = 0;
            sGTSPokedexView->maxScrollTimer = 0;
            sGTSPokedexView->scrollMonIncrement = 0;
            sGTSPokedexView->scrollDirection = 0;
            data->state = GTS_STATE_SEARCH_POKEMON_LEVEL_LIST;
            //RemoveWindow(sGTSPokedexView->windowid);
        }
        break;
    case GTS_STATE_SEARCH_POKEMON_LEVEL_LIST:
       sGTSPokedexView->dexMode = DoGTSListMenu(&sWindowTemplate_LevelSelect, &sListMenu_Levels, 1, LIST_MENU_TILE_NUM, LIST_MENU_PAL_NUM);
       if (sGTSPokedexView->dexMode == LIST_CANCEL) {
            PlaySE(SE_SELECT);
            //BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            ResetPokedexViewGTS(sGTSPokedexView);
            sGTSPokedexView->windowid = AddWindow(&sWindowTemplate_PokemonSelect); //Add Pokemon list box (empty for now)
            FillWindowPixelBuffer(sGTSPokedexView->windowid, 0x11);
            sGTSPokedexView->dexMode = 0;
            data->textState = 0;
            data->state = GTS_STATE_SEARCH_POKEMON_GENDER;
        }
        else if (PrintGTSMenuMessage(&data->textState, gText_ChooseGTSPokemonLevel))
        {
            DebugPrintf("%d",sGTSPokedexView->dexMode);
            //GetMonData(mon, MON_DATA_NICKNAME, name);
            //StringCopy_Nickname(gStringVar1, gParties[B_TRAINER_PLAYER][sGTSPokedexView->offerPokemon].box.nickname);

            StringCopy(gStringVar2, GetSpeciesName(sGTSPokedexView->selectedPokemon));
            sGTSPokedexView->cursorRelPos = 0;
            sGTSPokedexView->atTop = 1;
            sGTSPokedexView->atBottom = 0;
            //sGTSPokedexView->selectedPokemon = 0;
            sGTSPokedexView->pokemonListCount = 0;
            sGTSPokedexView->scrollTimer = 0;
            sGTSPokedexView->maxScrollTimer = 0;
            sGTSPokedexView->scrollMonIncrement = 0;
            sGTSPokedexView->scrollDirection = 0;
            sGTSPokedexView->currentPage = 0;
            data->state = GTS_STATE_FETCHING_POKEMON;
            //RemoveWindow(sGTSPokedexView->windowid);
        }
        break;
    case GTS_STATE_FETCHING_POKEMON: //Done
        GTSAddTextPrinterToWindow1(gText_SearchingForPokemon);
        DebugPrintf("GTS_STATE_FETCHING_POKEMON");
        struct GTSSearch *searchpoke = NULL;
        searchpoke = AllocZeroed(sizeof(struct GTSSearch));

        
        recvBufSize=32;
        concat_str(pURL,"http://gts.paccypad.com/pokemonrse/worldexchange/search?pid=\0");

        //Turn hex to str
        ConvertIntToHexStringN_v2(pidhex, gSaveBlock2Ptr->PID,STR_CONV_MODE_RIGHT_ALIGN,8);

        //Add PID to URL
        concat_str(pURL,(char *)pidhex);
        DebugPrintf("Buffed");
        DebugPrintf(pURL);
        //Initial Profile Setup
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }
        DebugPrintf("2616");

        memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
        concat_str(halftoken,(char *)pRecvData);

        //Cleaning up pRecvData
        for(i=0;i<32;i++){
            pRecvData[i]='\0';
        }

        sha1digest((u8 *)hash,NULL,(u8 *)halftoken,52);
        DebugPrintf("2623");
        //Add hash to URL
        concat_str(pURL,"&hash=");
        for(i = 0; i < 20; i++){
            ConvertIntToHexStringN_v2(pidhex, hash[i],STR_CONV_MODE_LEFT_ALIGN,2);
            pidhex[2]='\0';
            concat_str(pURL,(char *)pidhex);
        }
        recvBufSize=0x7C*7;
        
        //concat_str(pURL,hash);
        searchpoke->dexNum=sGTSPokedexView->selectedPokemon;
        searchpoke->gender=sGTSPokedexView->dexOrder;
        switch(sGTSPokedexView->dexMode)
        {
            case 0:
                searchpoke->minLevel=1;
                searchpoke->maxLevel=100;
                break;
            case 1:
                searchpoke->minLevel=1;
                searchpoke->maxLevel=10;
                break;
            case 2:
                searchpoke->minLevel=11;
                searchpoke->maxLevel=20;
                break;
            case 3:
                searchpoke->minLevel=21;
                searchpoke->maxLevel=30;
                break;
            case 4:
                searchpoke->minLevel=31;
                searchpoke->maxLevel=40;
                break;
            case 5:
                searchpoke->minLevel=41;
                searchpoke->maxLevel=50;
                break;
            case 6:
                searchpoke->minLevel=51;
                searchpoke->maxLevel=60;
                break;
            case 7:
                searchpoke->minLevel=61;
                searchpoke->maxLevel=70;
                break;
            case 8:
                searchpoke->minLevel=71;
                searchpoke->maxLevel=80;
                break;
            case 9:
                searchpoke->minLevel=81;
                searchpoke->maxLevel=90;
                break;
            case 10:
                searchpoke->minLevel=91;
                searchpoke->maxLevel=100;
                break;
        }

        concat_str(pURL,"&data=");
        searchpoke->country=0;
        searchpoke->pageNum=sGTSPokedexView->currentPage;
        searchpoke->checksum=0;
        searchpoke->checksum=encrypt_data(gSaveBlock2Ptr->PID, (char *)searchpoke, sizeof(struct GTSSearch));
        DebugPrintf("%u",searchpoke->checksum);
        DebugPrintf("%u",searchpoke->checksum^0x4a3b2c1d);
        DebugPrintf("%u",sizeof(struct GTSSearch));

        base64_encode(searchpoke->checksum, (char *)searchpoke, sizeof(struct GTSSearch)+4,encoded_data);
        concat_str(pURL,encoded_data);

        DebugPrintf(pURL);
        DebugPrintf("%u",sGTSPokedexView->searchResult[1].dexNum);
        data->errorNum = maDownload(pURL, NULL, 0, (u8 *)sGTSPokedexView->searchResult, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }

        DebugPrintf("%u",pRecvSize);
        DebugPrintf("%u",sizeof(struct GTSResult));

        if(pRecvSize<0x7C){
            data->state = GTS_POKEMON_NOT_FOUND;
            data->textState=0;
            break;
        }
        DebugPrintf("Buffed3");
        sGTSPokedexView->pokemonListCount=pRecvSize/0x7C;
        DebugPrintf("%u",sGTSPokedexView->pokemonListCount);
        Free(searchpoke);
        data->state = GTS_STATE_FETCHED_POKEMON_SETUP;
        break;
    case GTS_POKEMON_NOT_FOUND:
        input = PrintFailureMessage(&data->textState, gText_PokemonNotFound, &data->var);
        if (input){
            data->state = GTS_CHECK_RESULT;
        }
        break;
    case GTS_STATE_FETCHED_POKEMON_SETUP:
        
        sGTSPokedexView->selectedPokemon = 0;
        CreateMonSpritesAtPos(sGTSPokedexView->selectedPokemon, 0xE);
        PrintGTSTopMenu(1, TRUE);
        sGTSPokedexView->windowid = AddWindow(&sWindowTemplate_PokemonDetails);
        FillWindowPixelBuffer(sGTSPokedexView->windowid, 0x11);


        DrawTextBorderOuter(sGTSPokedexView->windowid, 0x001, 0x0F);
        CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_MAP);
        CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
        PutWindowTilemap(sGTSPokedexView->windowid);
        
        //Print Name gender Lv
        AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 1, 1, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, sGTSPokedexView->searchResult[0].boxmon.nickname);

        if(sGTSPokedexView->searchResult[0].gender==MON_MALE){
            AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 61, 1, 0, 0, sGTS_Ereader_Male, TEXT_SKIP_DRAW, gText_MaleSymbol);
        }
        else if (sGTSPokedexView->searchResult[0].gender==MON_FEMALE)
        {
            AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 61, 1, 0, 0, sGTS_Ereader_Female, TEXT_SKIP_DRAW, gText_FemaleSymbol);
        }
        
        ConvertIntToDecimalStringN(gStringVar1, sGTSPokedexView->searchResult[0].level, STR_CONV_MODE_LEFT_ALIGN, 3);
        StringExpandPlaceholders(gStringVar2,gText_LvVar1);

        AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 69, 1, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, gStringVar2);
        //Print ITEM
        AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 1, 17, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, gText_ItemCaps);
        //Print the held item
        i=GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon, MON_DATA_HELD_ITEM);
        if(i==ITEM_NONE){
            AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 9, 33, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, gText_None);
        }
        else{
            AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 9, 33, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, GetItemName(i));
        }
        
        //AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 9, 33, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, GetItemName(GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon, MON_DATA_HELD_ITEM)));
        //Print OFFERER
        AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 1, 49, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, gText_Offerer);
        //Print the offerer
        //StringCopy_PlayerName(gStringVar1, (u8 *)sGTSPokedexView->searchResult[0].OTName);
        ASCIIToPkmnStrLength(gStringVar1,(u8 *)sGTSPokedexView->searchResult[0].OTName,7);
        if(sGTSPokedexView->searchResult[0].trainerGender==0){
            AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 9, 65, 0, 0, sGTS_Ereader_Male, TEXT_SKIP_DRAW, gStringVar1);
        }
        else{
            AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 9, 65, 0, 0, sGTS_Ereader_Female, TEXT_SKIP_DRAW, gStringVar1);
        }
        //AddTextPrinterParameterized4(sGTSPokedexView->windowid,FONT_NORMAL, 9, 65, 0, 0, sGTS_Ereader_TextColor_2, TEXT_SKIP_DRAW, gStringVar1);
        DebugPrintf("Buffed4");
        CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
        DebugPrintf("Buffed5");
        GTSAddWantedToWindow1(0);
        sGTSPokedexView->atTop = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_LEFT, 72, 152, 208, sGTSPokedexView->pokemonListCount,
                                                                               TAG_SCROLL_ARROW, TAG_SCROLL_ARROW, &sGTSPokedexView->selectedPokemon);
        DebugPrintf("Buffed6");
        LoadPalette(sBlueInterface_Pal, OBJ_PLTT_ID(1), PLTT_SIZE_4BPP);
        DebugPrintf("Buffed7");
        data->state = GTS_STATE_SELECT_FETCHED_POKEMON;
        break;
    case GTS_STATE_SELECT_FETCHED_POKEMON: //sGTSPokedexView->pokedexList[0] TODO
        DebugPrintf("Buffed8");
        if (j) {
            sGTSPokedexView->selectedPokemon = TryDoGTSSpriteScroll(sGTSPokedexView->selectedPokemon, 0xE);
            if(sGTSPokedexView->selectedPokemon==7){
                u8 monId;
                sGTSPokedexView->currentPage=sGTSPokedexView->currentPage+1;
                RemoveScrollIndicatorArrowPair(sGTSPokedexView->atTop);
                sGTSPokedexView->cursorRelPos = 0;
                sGTSPokedexView->atTop = 1;
                sGTSPokedexView->atBottom = 0;
                //sGTSPokedexView->selectedPokemon = 0;
                sGTSPokedexView->pokemonListCount = 0;
                sGTSPokedexView->scrollTimer = 0;
                sGTSPokedexView->maxScrollTimer = 0;
                sGTSPokedexView->scrollMonIncrement = 0;
                sGTSPokedexView->scrollDirection = 0;
                sGTSPokedexView->selectedPokemon = sGTSPokedexView->searchResult[0].dexNum;
                DebugPrintf("here");
                DebugPrintf("%u",sGTSPokedexView->selectedPokemon);
                DebugPrintf("%u",sGTSPokedexView->selectedPokemon);
                for(monId=0; monId < MAX_MONS_ON_SCREEN;monId++){
                    FreeAndDestroyMonPicSprite(sGTSPokedexView->monSpriteIds[monId]);
                    sGTSPokedexView->monSpriteIds[monId] = 0xFFFF;
                }
                for(monId=0; monId < 7;monId++){
                    ZeroSearchResults(&sGTSPokedexView->searchResult[monId]);
                }
                ClearStdWindowAndFrame(sGTSPokedexView->windowid, FALSE);
                data->state = GTS_STATE_FETCHING_POKEMON;
                break;
            }
            else if (sGTSPokedexView->selectedPokemon==8){
                u8 monId;
                sGTSPokedexView->currentPage=sGTSPokedexView->currentPage-1;
                RemoveScrollIndicatorArrowPair(sGTSPokedexView->atTop);
                sGTSPokedexView->cursorRelPos = 0;
                sGTSPokedexView->atTop = 1;
                sGTSPokedexView->atBottom = 0;
                //sGTSPokedexView->selectedPokemon = 0;
                sGTSPokedexView->pokemonListCount = 0;
                sGTSPokedexView->scrollTimer = 0;
                sGTSPokedexView->maxScrollTimer = 0;
                sGTSPokedexView->scrollMonIncrement = 0;
                sGTSPokedexView->scrollDirection = 0;
                sGTSPokedexView->selectedPokemon = sGTSPokedexView->searchResult[0].dexNum;
                for(monId=0; monId < MAX_MONS_ON_SCREEN;monId++){
                    FreeAndDestroyMonPicSprite(sGTSPokedexView->monSpriteIds[monId]);
                    sGTSPokedexView->monSpriteIds[monId] = 0xFFFF;
                }
                for(monId=0; monId < 7;monId++){
                    ZeroSearchResults(&sGTSPokedexView->searchResult[monId]);
                }
                ClearStdWindowAndFrame(sGTSPokedexView->windowid, FALSE);
                data->state = GTS_STATE_FETCHING_POKEMON;
                break;
            }
            else if(sGTSPokedexView->selectedPokemon>=1000){
                sGTSPokedexView->selectedPokemon=sGTSPokedexView->selectedPokemon-1000;
                RemoveScrollIndicatorArrowPair(sGTSPokedexView->atTop);
                data->state = GTS_STATE_TRADE_WITH_THIS_PERSON;
                break;
            }
            else if (sGTSPokedexView->selectedPokemon>=500)
            {
                sGTSPokedexView->selectedPokemon=sGTSPokedexView->selectedPokemon-500;
                data->state = GTS_STATE_CANCEL_SEARCH;
                break;
            }

            j=FALSE;
        }
        if (sGTSPokedexView->scrollTimer) {
            if (UpdateDexListScrollSprites(sGTSPokedexView->scrollDirection, sGTSPokedexView->scrollMonIncrement, sGTSPokedexView->maxScrollTimer)){
                j = TRUE;
            }
        }
        break;
    case GTS_STATE_CANCEL_SEARCH:
        u8 monId;
        for(monId=0; monId < MAX_MONS_ON_SCREEN;monId++){
            FreeAndDestroyMonPicSprite(sGTSPokedexView->monSpriteIds[monId]);
            sGTSPokedexView->monSpriteIds[monId] = 0xFFFF;
        }
        ClearTextWindow();
        rbox_fill_rectangle(sGTSPokedexView->windowid);
        ClearWindowTilemap(sGTSPokedexView->windowid);
        CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_MAP);
        RemoveScrollIndicatorArrowPair(sGTSPokedexView->atTop);
        data->state = GTS_CHECK_RESULT;
        break;
    case GTS_STATE_TRADE_WITH_THIS_PERSON:
        input = DoGTSYesNo(&data->textState, &data->var, FALSE, gText_TradeQuestion);
        switch (input)
        {
        case 0: // Yes, Select from Box
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            data->state = GTS_STATE_CHOOSE_EXCHANGE;
            break;
        case 1: // No
        case MENU_B_PRESSED:
            sGTSPokedexView->atTop = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_LEFT, 72, 152, 208, sGTSPokedexView->pokemonListCount,
                                                                               TAG_SCROLL_ARROW, TAG_SCROLL_ARROW, &sGTSPokedexView->selectedPokemon);
            data->state = GTS_STATE_SELECT_FETCHED_POKEMON;
            break;
        }
        break;
    case GTS_STATE_CHOOSE_EXCHANGE:
        if (!gPaletteFade.active)
        {
            u8 monId;
            DestroyTask(taskId);
            FreeAllWindowBuffers();
            Free(GetBgTilemapBuffer(0));
            Free(GetBgTilemapBuffer(1));
            Free(GetBgTilemapBuffer(2));
            Free(GetBgTilemapBuffer(3));
            VarSet(VAR_UNUSED_0x40FF,GTS_STATE_CONFIRM_EXCHANGE);
            for(monId=0; monId < MAX_MONS_ON_SCREEN;monId++){
                FreeAndDestroyMonPicSprite(sGTSPokedexView->monSpriteIds[monId]);
                sGTSPokedexView->monSpriteIds[monId] = 0xFFFF;
            }
            DebugPrintf("GTS_STATE_CHOOSE_EXCHANGE");
            gSpecialVar_0x8009 = sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].natDexRequest;
            gSpecialVar_0x800A = sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].minLevel;
            gSpecialVar_0x800B = sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].maxLevel;
            sSelectionType = SELECT_PC_MON_GTS_TRADE;
            SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL);
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(7, 11));
            ChooseMonFromStorage();
            //ChooseBoxMon2(SELECT_PC_MON_GTS_TRADE);
        }
        break;
    case GTS_STATE_CONFIRM_EXCHANGE:
        if (!gPaletteFade.active)
        {
            //u16 spriteId;
            if(gSpecialVar_0x8004 == PC_MON_CHOSEN){
                StringCopy_Nickname(gStringVar1, gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos].nickname);
                //spriteId = CreateMonPicSprite(GetBoxMonData(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos], MON_DATA_SPECIES, NULL), GetBoxMonData(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos], MON_DATA_IS_SHINY, NULL), GetBoxMonData(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos], MON_DATA_PERSONALITY, NULL), TRUE, 120, 64, 2, TAG_NONE);
            }
            else{
                StringCopy_Nickname(gStringVar1, gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004].box.nickname);
                //spriteId = CreateMonPicSprite(GetMonData(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], MON_DATA_SPECIES, NULL), GetMonData(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], MON_DATA_IS_SHINY, NULL), GetMonData(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], MON_DATA_PERSONALITY, NULL), TRUE, 120, 64, 2, TAG_NONE);
            }
            //gSprites[spriteId].oam.priority = 3;
            //DebugPrintf("%u",spriteId);

            StringCopy_Nickname(gStringVar2, sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].boxmon.nickname);

            input = DoGTSYesNo(&data->textState, &data->var, FALSE, gText_ConfirmTrade);
            switch (input)
            {
            case 0: // Yes, Select from Box
                //DebugPrintf("%u",spriteId);
                //FreeAndDestroyMonPicSprite(spriteId);
                //ClearStdWindowAndFrame(spriteId, FALSE);
                recvBufSize=0x92;
                concat_str(pURL,"http://gts.paccypad.com/pokemonrse/worldexchange/exchange?pid=\0");
                //Turn hex to str
                ConvertIntToHexStringN_v2(pidhex, gSaveBlock2Ptr->PID,STR_CONV_MODE_RIGHT_ALIGN,8);

                //Add PID to URL
                concat_str(pURL,(char *)pidhex);
                recvBufSize=0x96*7;

                //Initial Profile Setup
                data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
                if(data->errorNum !=0){
                    maKill();
                    data->state = GTS_STATE_CLIENT_ERROR;
                    break;
                }

                memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
                concat_str(halftoken,(char *)pRecvData);

                sha1digest((u8 *)hash,NULL,(u8 *)halftoken,52);

                //Add hash to URL
                //Add hash to URL
                concat_str(pURL,"&hash=");
                for(i = 0; i < 20; i++){
                    ConvertIntToHexStringN_v2(pidhex, hash[i],STR_CONV_MODE_LEFT_ALIGN,2);
                    pidhex[2]='\0';
                    concat_str(pURL,(char *)pidhex);
                }

                //Add data to URL
                concat_str(pURL,"&data=");


                DebugPrintf("Uploading 2");
                //sGTSPokedexView->searchResult[0].pid=gSaveBlock2Ptr->PID; //sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].pid;
                //sGTSPokedexView->searchResult[1].boxmon=sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].boxmon;
                memcpy(&sGTSPokedexView->searchResult[1].boxmon,&sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].boxmon,80);
                BoxMonToMon(&sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].boxmon, &gParties[B_TRAINER_OPPONENT_A][0]);
                sGTSPokedexView->searchResult[1].pid=sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].pid;
                //sGTSPokedexView->searchResult[1].OTName=sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].OTName;
                StringCopy_PlayerName((u8 *)sGTSPokedexView->searchResult[1].OTName,(u8 *)sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].OTName);
                sGTSPokedexView->searchResult[1].checksum=sGTSPokedexView->searchResult[1].pid;

                sGTSPokedexView->searchResult[0].pid=gSaveBlock2Ptr->PID;

                //Get mon and replace with new
                if(gSpecialVar_0x8004 == PC_MON_CHOSEN){
                    DebugPrintf("BOX TRADED!");
                    CopyBoxMonAt(gSpecialVar_MonBoxId,gSpecialVar_MonBoxPos,&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box);
                    CopyMon(&sGTSPokedexView->searchResult[0].boxmon,&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos],80);
                    CopyMon(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos], &gParties[B_TRAINER_OPPONENT_A][0].box,80);
                    //memcpy(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos], &gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box,80);
                    //ZeroBoxMonData(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos]);
                }
                else{
                    DebugPrintf("TRADED!");
                    CopyMon(&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC], &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004],100);
                    CopyMon(&sGTSPokedexView->searchResult[0].boxmon,&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004].box,80);
                    CopyMon(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], &gParties[B_TRAINER_OPPONENT_A][0],100);
                    //BoxMonToMon(&sGTSPokedexView->searchResult[sGTSPokedexView->selectedPokemon].boxmon, &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004]);
                    //ZeroMonData(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004]);
                }
                
                sGTSPokedexView->searchResult[0].dexNum=GET_BASE_SPECIES_ID(GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon,MON_DATA_SPECIES,NULL));
                sGTSPokedexView->searchResult[0].gender=GetBoxMonGender(&sGTSPokedexView->searchResult[0].boxmon);
                sGTSPokedexView->searchResult[0].level=GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon,MON_DATA_LEVEL);
                sGTSPokedexView->searchResult[0].genderRequest=0x01;

                sGTSPokedexView->searchResult[0].minLevel=1;
                sGTSPokedexView->searchResult[0].maxLevel=100;
                
                sGTSPokedexView->searchResult[0].trainerGender=GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon,MON_DATA_OT_GENDER,NULL);
                sGTSPokedexView->searchResult[0].trainerID=GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon,MON_DATA_OT_ID,NULL);
                sGTSPokedexView->searchResult[0].secretID=GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon,MON_DATA_OT_ID,NULL) >> 16;
                PkmnStrToASCIILength((u8 *)sGTSPokedexView->searchResult[0].OTName,gSaveBlock2Ptr->playerName, 7);
                
                sGTSPokedexView->searchResult[0].country=0;
                sGTSPokedexView->searchResult[0].region=0;
                sGTSPokedexView->searchResult[0].trainerClass=0;
                sGTSPokedexView->searchResult[0].isExchanged=TRUE;
                sGTSPokedexView->searchResult[0].gameVersion=VERSION_EMERALD;
                sGTSPokedexView->searchResult[0].romHackID=EXPANSION_VERSION_MAJOR;
                sGTSPokedexView->searchResult[0].romHackVersion=EXPANSION_VERSION_MINOR;
                sGTSPokedexView->searchResult[0].language=LANGUAGE_ENGLISH;
                
                sGTSPokedexView->searchResult[0].checksum=0;
                sGTSPokedexView->searchResult[0].checksum=encrypt_data(sGTSPokedexView->searchResult[0].pid, (char *)sGTSPokedexView->searchResult, sizeof(sGTSPokedexView->searchResult[0]));
                DebugPrintf("%u",sGTSPokedexView->searchResult[0].checksum);
                DebugPrintf("%u",sGTSPokedexView->searchResult[0].checksum^0x4a3b2c1d);
                DebugPrintf("%u",sGTSPokedexView->searchResult[0].pid);
                
                base64_encode(sGTSPokedexView->searchResult[0].checksum, (char *)sGTSPokedexView->searchResult, sizeof(sGTSPokedexView->searchResult[0])+4,encoded_data);
                concat_str(pURL,encoded_data);
                
                DebugPrintf(pURL);
                data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
                if(data->errorNum !=0){
                    maKill();
                    //Restore mon
                    if(gSpecialVar_0x8004 == PC_MON_CHOSEN){
                        memcpy(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos], &gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box,80);
                    }
                    else{
                        CopyMon(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004],&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC],100);
                    }
                    
                    data->state = GTS_STATE_CLIENT_ERROR;
                    break;
                }

                if(pRecvData[1]==0x01){
                    data->state = GTS_STATE_SAVE_1;
                    data->nextState = GTS_STATE_SAVE_EXCHANGE_FINISH;
                }
                else{
                    //Restore mon
                    if(gSpecialVar_0x8004 == PC_MON_CHOSEN)
                        memcpy(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos],&sGTSPokedexView->searchResult[0].boxmon,80);
                    else
                        GiveBoxMonToPlayer(&sGTSPokedexView->searchResult[0].boxmon);
                    data->state = GTS_STATE_SERVER_ERROR;
                    break;
                }
                
                sGTSPokedexView->currentPage=0;
                VarSet(VAR_UNUSED_0x40FF,GTS_CHECK_RESULT);
                break;
            case 1: // No
            case MENU_B_PRESSED:
                //FreeAndDestroyMonPicSprite(spriteId);
                data->state = GTS_STATE_FETCHED_POKEMON_SETUP;
                break;
            }
        }
        break;
    case GTS_STATE_SAVE_EXCHANGED_POKEMON:  //Done
        if (SaveOnMysteryGiftMenu(&data->textState))
            // Choose where to access the Wonder Card/News from
            recvBufSize=32;
            concat_str(pURL,"http://gts.paccypad.com/pokemonrse/worldexchange/exchange_finish?pid=\0");

            //Turn hex to str
            ConvertIntToHexStringN_v2(pidhex, gSaveBlock2Ptr->PID,STR_CONV_MODE_RIGHT_ALIGN,8);

            //Add PID to URL
            concat_str(pURL,(char *)pidhex);

            //memcpy(pid,&gSaveBlock2Ptr->PID,4);
            //pid[4]='\0';
            //DebugPrintf("%u\n", pid);
            //concat_str(pURL,(char *)pid);

            //Get hash
            data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
            if(data->errorNum !=0){
                maKill();
                data->state = GTS_STATE_CLIENT_ERROR;
                break;
            }

            memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
            concat_str(halftoken,(char *)pRecvData);

            sha1digest((u8 *)hash,NULL,(u8 *)halftoken,52);

            //Add hash to URL
            concat_str(pURL,"&hash=");
            concat_str(pURL,hash);

            data->errorNum = maDownload(pURL, NULL, 0, pRecvData, 0x4, &pRecvSize, "", "");
            if(data->errorNum !=0){
                maKill();
                data->state = GTS_STATE_CLIENT_ERROR;
                break;
            }

            if(*pRecvData==0x0001){
                sGTSPokedexView->currentPage=0;
                DoGTSExchangeScene();
                data->state = GTS_STATE_MAIN_MENU;
            }
            else{
                if(gSpecialVar_0x8004 == PC_MON_CHOSEN){
                    memcpy(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos], &gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box,80);
                }
                else{
                    CopyMon(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004],&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC],100);
                }
                data->state = GTS_STATE_SERVER_ERROR;
            }
        break;
    case GTS_STATE_DEPOSIT_POKEMON: //Done
        if (!gPaletteFade.active)
        {
            DestroyTask(taskId);
            FreeAllWindowBuffers();
            Free(GetBgTilemapBuffer(0));
            Free(GetBgTilemapBuffer(1));
            Free(GetBgTilemapBuffer(2));
            Free(GetBgTilemapBuffer(3));
            VarSet(VAR_UNUSED_0x40FF,GTS_STATE_PICK_WANTED_POKEMON);
            ChooseMonFromStorage();
        }
        break;
    case GTS_STATE_PICK_WANTED_POKEMON: //Done
        if(gSpecialVar_0x8004 == 0xFF){
            gSpecialVar_0x8004=0;
            data->state = GTS_STATE_MAIN_MENU;
        }
        else{
            DrawTextBorderOuter(sGTSPokedexView->windowid, 0x001, 0x0F);
            CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_MAP);
            CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
            PutWindowTilemap(sGTSPokedexView->windowid);
            sGTSPokedexView->offerPokemon = gSpecialVar_0x8004;//gSpecialVar_0x8004;
            //PrintSearchText(sDexSearchNameOptions[searchParamId].title, 0x2D, 0x11);
            sGTSPokedexView->dexMode = DoGTSListMenu(&sWindowTemplate_ABCSelect, &sListMenu_ABCMenu, 1, LIST_MENU_TILE_NUM, LIST_MENU_PAL_NUM);
        
            if (sGTSPokedexView->dexMode == LIST_CANCEL) {
                PlaySE(SE_SELECT);
                BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
                data->state = GTS_STATE_DEPOSIT_POKEMON;
            } else if (PrintGTSMenuMessage(&data->textState, gText_ChooseGTSPokemon)) {
                data->state = GTS_STATE_DEPOSITING_POKEMON;
                PrintGTSTopMenu(0, TRUE);
            }
        }
        break;
    case GTS_STATE_DEPOSITING_POKEMON: //Done
        input = DoPokedexSearchGTS(sGTSPokedexView->dexMode+1); //Gets alphabetical list of ABC option selected
        //sGTSPokedexView->windowid = AddWindow(&sWindowTemplate_PokemonSelect);
        //CopyWindowToVram(sGTSPokedexView->windowid, 3);
        //CreatePokedexListGTS();
        if(sGTSPokedexView->pokemonListCount != 0){
            CreateMonListEntryGTS(0, 0);
            sGTSPokedexView->atTop = 1;
            data->state = GTS_STATE_POKEMON_LIST;
        }
        else {
            PlaySE(SE_FAILURE);
            data->state = GTS_STATE_PICK_WANTED_POKEMON;
        }
        break;
    case GTS_STATE_POKEMON_LIST: //Done
        sGTSPokedexView->selectedPokemon = TryDoPokedexScrollGTS(sGTSPokedexView->selectedPokemon);
        if (JOY_NEW(A_BUTTON))
        {
            data->state = GTS_STATE_POKEMON_LEVEL_LIST;
            FillWindowPixelRect(sGTSPokedexView->windowid, PIXEL_FILL(1), 0, 0, 80, 80);
            CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
            //ClearStdWindowAndFrame(3, FALSE);
            ClearStdWindowAndFrame(sGTSPokedexView->windowid, FALSE);
        }
        if (JOY_NEW(B_BUTTON))
        {
            data->state = GTS_STATE_RETURN_POKEMON_LIST;
            FillWindowPixelRect(sGTSPokedexView->windowid, PIXEL_FILL(1), 0, 0, 80, 80);
            CopyWindowToVram(sGTSPokedexView->windowid, COPYWIN_GFX);
            //RemoveWindow(0);
        }
        break;
    case GTS_STATE_POKEMON_LEVEL_LIST: //Done
        sGTSPokedexView->dexMode = DoGTSListMenu(&sWindowTemplate_LevelSelect, &sListMenu_LevelsWanted, 1, LIST_MENU_TILE_NUM, LIST_MENU_PAL_NUM);
        if (sGTSPokedexView->dexMode == LIST_CANCEL) {
            PlaySE(SE_SELECT);
            //BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            ResetPokedexViewGTS(sGTSPokedexView);
            sGTSPokedexView->windowid = AddWindow(&sWindowTemplate_PokemonSelect); //Add Pokemon list box (empty for now)
            FillWindowPixelBuffer(sGTSPokedexView->windowid, 0x11);
            sGTSPokedexView->dexMode = 0;
            data->textState = 0;
            data->state = GTS_STATE_PICK_WANTED_POKEMON;
        }
        else if (PrintGTSMenuMessage(&data->textState, gText_ChooseGTSPokemonLevel))
        {
            //GetMonData(mon, MON_DATA_NICKNAME, name);
            if(gSpecialVar_0x8004 == PC_MON_CHOSEN)
                StringCopy_Nickname(gStringVar1, gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos].nickname);
            else
                StringCopy_Nickname(gStringVar1, gParties[B_TRAINER_PLAYER][sGTSPokedexView->offerPokemon].box.nickname);
            StringCopy(gStringVar2, GetSpeciesName(sGTSPokedexView->pokedexList[sGTSPokedexView->selectedPokemon].dexNum));

            sGTSPokedexView->searchResult[0].natDexRequest=GET_BASE_SPECIES_ID(sGTSPokedexView->pokedexList[sGTSPokedexView->selectedPokemon].dexNum);

            sGTSPokedexView->cursorRelPos = 0;
            sGTSPokedexView->atTop = 1;
            sGTSPokedexView->atBottom = 0;
            sGTSPokedexView->selectedPokemon = 0;
            sGTSPokedexView->pokemonListCount = 0;
            data->state = GTS_STATE_CONFIRM_OFFER;
            //RemoveWindow(sGTSPokedexView->windowid);
        }
        break;
    case GTS_STATE_RETURN_POKEMON_LIST: //Done
        sGTSPokedexView->dexMode = DoGTSListMenu(&sWindowTemplate_ABCSelect, &sListMenu_ABCMenu, 1, LIST_MENU_TILE_NUM, LIST_MENU_PAL_NUM);
        if (PrintGTSMenuMessage(&data->textState, gText_ChooseGTSPokemon))
        {
            sGTSPokedexView->cursorRelPos = 0;
            sGTSPokedexView->atTop = 1;
            sGTSPokedexView->atBottom = 0;
            sGTSPokedexView->selectedPokemon = 0;
            sGTSPokedexView->pokemonListCount = 0;
            data->state = GTS_STATE_DEPOSITING_POKEMON;
            //RemoveWindow(sGTSPokedexView->windowid);
        }
        break;    
    case GTS_STATE_CONFIRM_OFFER: //Done
        input = DoGTSYesNo(&data->textState, &data->var, FALSE, gText_ConfirmOffer);
        switch (input)
        {
        case 0: // Yes, Upload Pokemon
            DebugPrintf("Uploading 1");
            recvBufSize=0x92;
            concat_str(pURL,"http://gts.paccypad.com/pokemonrse/worldexchange/post?pid=\0");

            //memcpy(pid,&gSaveBlock2Ptr->PID,4);
            //pid[4]='\0';
            //DebugPrintf("%u\n", pid);
            //concat_str(pURL,(char *)pid);
            recvBufSize=32;

            //Turn hex to str
            ConvertIntToHexStringN_v2(pidhex, gSaveBlock2Ptr->PID,STR_CONV_MODE_RIGHT_ALIGN,8);

            //Add PID to URL
            concat_str(pURL,(char *)pidhex);


            DebugPrintf(pURL);
            //Initial Profile Setup
            data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
            if(data->errorNum !=0){
                maKill();
                data->state = GTS_STATE_CLIENT_ERROR;
                break;
            }

            memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
            concat_str(halftoken,(char *)pRecvData);

            sha1digest((u8 *)hash,NULL,(u8 *)halftoken,52);

            //Add hash to URL
            //Add hash to URL
            concat_str(pURL,"&hash=");
            for(i = 0; i < 20; i++){
                ConvertIntToHexStringN_v2(pidhex, hash[i],STR_CONV_MODE_LEFT_ALIGN,2);
                pidhex[2]='\0';
                concat_str(pURL,(char *)pidhex);
            }
            //concat_str(pURL,"&hash=");
            //concat_str(pURL,hash);

            //Add data to URL
            concat_str(pURL,"&data=");


            DebugPrintf("Uploading 2");
            sGTSPokedexView->searchResult[0].pid=gSaveBlock2Ptr->PID;

            //Get mon and delete from party/box
            if(gSpecialVar_0x8004 == PC_MON_CHOSEN){
                CopyBoxMonAt(gSpecialVar_MonBoxId,gSpecialVar_MonBoxPos,&sGTSPokedexView->searchResult[0].boxmon);
                BoxMonToMon(&sGTSPokedexView->searchResult[0].boxmon, &gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC]);
                ZeroBoxMonData(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos]);
            }
            else{
                //memcpy(&sGTSPokedexView->searchResult[0].boxmon,&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004].box,80);  //StorePokemonInEmptyDaycareSlot(&gPlayerParty[monId], &gSaveBlock1Ptr->daycare);
                CopyMon(&sGTSPokedexView->searchResult[0].boxmon, &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004].box, 80);
                CopyMon(&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC], &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], 100);
                ZeroMonData(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004]);
            }
            //memcpy(&sGTSPokedexView->searchResult[0].boxmon,&gPokemonStoragePtr->boxes[StorageGetCurrentBox()][GetSavedCursorPos()],80);
            
            sGTSPokedexView->searchResult[0].dexNum=GET_BASE_SPECIES_ID(GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon,MON_DATA_SPECIES,NULL));
            sGTSPokedexView->searchResult[0].gender=GetBoxMonGender(&sGTSPokedexView->searchResult[0].boxmon); //TODO
            sGTSPokedexView->searchResult[0].level=GetLevelFromBoxMonExp(&sGTSPokedexView->searchResult[0].boxmon);
            //sGTSPokedexView->searchResult[0].natDexRequest=gSaveBlock2Ptr->PID;  get before
            sGTSPokedexView->searchResult[0].genderRequest=0x01;

            switch(sGTSPokedexView->dexMode)
            {
                case 0:
                    sGTSPokedexView->searchResult[0].minLevel=1;
                    sGTSPokedexView->searchResult[0].maxLevel=100;
                    break;
                case 1:
                    sGTSPokedexView->searchResult[0].minLevel=1;
                    sGTSPokedexView->searchResult[0].maxLevel=9;
                    break;
                case 2:
                    sGTSPokedexView->searchResult[0].minLevel=10;
                    sGTSPokedexView->searchResult[0].maxLevel=100;
                    break;
                case 3:
                    sGTSPokedexView->searchResult[0].minLevel=20;
                    sGTSPokedexView->searchResult[0].maxLevel=100;
                    break;
                case 4:
                    sGTSPokedexView->searchResult[0].minLevel=30;
                    sGTSPokedexView->searchResult[0].maxLevel=100;
                    break;
                case 5:
                    sGTSPokedexView->searchResult[0].minLevel=40;
                    sGTSPokedexView->searchResult[0].maxLevel=100;
                    break;
                case 6:
                    sGTSPokedexView->searchResult[0].minLevel=50;
                    sGTSPokedexView->searchResult[0].maxLevel=100;
                    break;
                case 7:
                    sGTSPokedexView->searchResult[0].minLevel=60;
                    sGTSPokedexView->searchResult[0].maxLevel=100;
                    break;
                case 8:
                    sGTSPokedexView->searchResult[0].minLevel=70;
                    sGTSPokedexView->searchResult[0].maxLevel=100;
                    break;
                case 9:
                    sGTSPokedexView->searchResult[0].minLevel=80;
                    sGTSPokedexView->searchResult[0].maxLevel=100;
                    break;
                case 10:
                    sGTSPokedexView->searchResult[0].minLevel=90;
                    sGTSPokedexView->searchResult[0].maxLevel=100;
                    break;
                    
            }
            DebugPrintf("Uploading 3");
            sGTSPokedexView->searchResult[0].trainerGender=GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon,MON_DATA_OT_GENDER,NULL);
            sGTSPokedexView->searchResult[0].trainerID=GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon,MON_DATA_OT_ID,NULL);
            sGTSPokedexView->searchResult[0].secretID=GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon,MON_DATA_OT_ID,NULL) >> 16;
            PkmnStrToASCIILength((u8 *)sGTSPokedexView->searchResult[0].OTName,gSaveBlock2Ptr->playerName, 7);
            //GetBoxMonData(&sGTSPokedexView->searchResult[0].boxmon,MON_DATA_OT_NAME,(u8 *)sGTSPokedexView->searchResult[0].OTName);
            DebugPrintf("Uploading 3.1");
            sGTSPokedexView->searchResult[0].country=0;
            sGTSPokedexView->searchResult[0].region=0;
            sGTSPokedexView->searchResult[0].trainerClass=0;
            sGTSPokedexView->searchResult[0].isExchanged=FALSE;
            sGTSPokedexView->searchResult[0].gameVersion=VERSION_EMERALD;
            sGTSPokedexView->searchResult[0].romHackID=EXPANSION_VERSION_MAJOR;
            sGTSPokedexView->searchResult[0].romHackVersion=EXPANSION_VERSION_MINOR;
            sGTSPokedexView->searchResult[0].language=LANGUAGE_ENGLISH;
            DebugPrintf("Uploading 3.2");
            //Determine checksum for pokemon data and encrypt data
            sGTSPokedexView->searchResult[0].checksum=0;
            sGTSPokedexView->searchResult[0].checksum=encrypt_data(sGTSPokedexView->searchResult[0].pid, (char *)sGTSPokedexView->searchResult, sizeof(sGTSPokedexView->searchResult[0]));
            DebugPrintf("%u",sGTSPokedexView->searchResult[0].checksum);
            DebugPrintf("%u",sGTSPokedexView->searchResult[0].checksum^0x4a3b2c1d);
            DebugPrintf("%u",sGTSPokedexView->searchResult[0].pid);
            DebugPrintf("Uploading 3.3");
            base64_encode(sGTSPokedexView->searchResult[0].checksum, (char *)sGTSPokedexView->searchResult, sizeof(sGTSPokedexView->searchResult[0])+4,encoded_data);
            //*encoded_data=base64_encode(sGTSPokedexView->searchResult[0].checksum, (char *)sGTSPokedexView->searchResult, sizeof(sGTSPokedexView->searchResult[0])+4);
            //DebugPrintf("%u",encoded_data[93]);
            DebugPrintf("Uploading 3.xx");
            //DebugPrintf(encoded_data);
            //DebugPrintf(encoded_data[1]);
            concat_str(pURL,encoded_data);
            DebugPrintf("Uploading 3.4");
            DebugPrintf(pURL);
            data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
            if(data->errorNum !=0){
                maKill();
                //Restore mon
                if(gSpecialVar_0x8004 == PC_MON_CHOSEN)
                    memcpy(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos],&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box,80);
                else
                    GiveBoxMonToPlayer(&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box);
                
                data->state = GTS_STATE_CLIENT_ERROR;
                break;
            }
            DebugPrintf("Uploading 4");
            //DebugPrintf("%u\n",*pRecvData);
            if(pRecvData[1]==0x01){
                data->state = GTS_STATE_SAVE_1;
                data->nextState = GTS_STATE_SAVE_POST_FINISH;
            }
            else{
                //Restore mon
                if(gSpecialVar_0x8004 == PC_MON_CHOSEN)
                    memcpy(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos],&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box,80);
                else
                    GiveBoxMonToPlayer(&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box);
                data->state = GTS_STATE_SERVER_ERROR;
                break;
            }
            
            sGTSPokedexView->currentPage=2;
            VarSet(VAR_UNUSED_0x40FF,GTS_CHECK_RESULT);
            break;
        case 1: // No
        case MENU_B_PRESSED:
            MysteryGiftClient_SetParam(TRUE);
            MysteryGiftClient_AdvanceState();
            data->state = GTS_STATE_POKEMON_LEVEL_LIST;
            break;
        }
        break;
    case GTS_STATE_SAVE_1:
        GTSAddTextPrinterToWindow1(gText_CheckingGTSStatus);
        data->state = GTS_STATE_SAVE_2;
        break;
    case GTS_STATE_SAVE_2:
        //SetContinueGameWarpStatusToDynamicWarp();
        gSaveBlock2Ptr->specialSaveWarpFlags |= 1;
        gSaveBlock1Ptr->continueGameWarp.mapGroup = gSaveBlock1Ptr->location.mapGroup;
        gSaveBlock1Ptr->continueGameWarp.mapNum = gSaveBlock1Ptr->location.mapNum;
        gSaveBlock1Ptr->continueGameWarp.warpId = 2;
        gSaveBlock1Ptr->continueGameWarp.x = gSaveBlock1Ptr->pos.x;
        gSaveBlock1Ptr->continueGameWarp.y = gSaveBlock1Ptr->pos.y;
        LinkFullSave_Init();
        data->state = GTS_STATE_SAVE_3;
        data->var = 0;
        break;
    case GTS_STATE_SAVE_3:
        if (++data->var == 5)
            data->state = GTS_STATE_SAVE_4;
        break;
    case GTS_STATE_SAVE_4:
        if (LinkFullSave_WriteSector())
        {
            ClearContinueGameWarpStatus2();
            data->state = GTS_STATE_SAVE_5;
        }
        else
        {
            // Save isn't finished, delay again
            data->var = 0;
            data->state = GTS_STATE_SAVE_3;
        }
        break;
    case GTS_STATE_SAVE_5:
        LinkFullSave_ReplaceLastSector();
        data->state = data->nextState;
        data->var = 0;
        break;
    case GTS_STATE_SAVE_POST_FINISH:
        recvBufSize=32;
        concat_str(pURL,"http://gts.paccypad.com/pokemonrse/worldexchange/post_finish?pid=\0");

        //Turn hex to str
        ConvertIntToHexStringN_v2(pidhex, gSaveBlock2Ptr->PID,STR_CONV_MODE_RIGHT_ALIGN,8);

        //Add PID to URL
        concat_str(pURL,(char *)pidhex);
        
        DebugPrintf(pURL);
        
        //Get hash
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            //Restore mon
            if(gSpecialVar_0x8004 == PC_MON_CHOSEN)
                memcpy(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos],&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box,80);
            else
                GiveBoxMonToPlayer(&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box);
            
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }
        
        //Game has fully saved send final confirmation to server
        memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
        concat_str(halftoken,(char *)pRecvData);

        //Cleaning up pRecvData
        for(i=0;i<32;i++){
            pRecvData[i]='\0';
        }

        sha1digest((u8 *)hash,NULL,(u8 *)halftoken,52);

        //Add hash to URL
        concat_str(pURL,"&hash=");
        for(i = 0; i < 20; i++){
            ConvertIntToHexStringN_v2(pidhex, hash[i],STR_CONV_MODE_LEFT_ALIGN,2);
            pidhex[2]='\0';
            concat_str(pURL,(char *)pidhex);
        }
        
        DebugPrintf(pURL);
        DebugPrintf("ZA");
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }

        DebugPrintf("hi");
        DebugPrintf("%u\n", *pRecvData);
        
        if(pRecvData[1]==0x01){
            LinkFullSave_SetLastSectorSignature();
            data->state = GTS_STATE_TRADE_ANIMATION;
        }
        else{
            data->state = GTS_STATE_SERVER_ERROR;
        }

        break;
    case GTS_STATE_SAVE_EXCHANGE_FINISH:
        recvBufSize=32;
        concat_str(pURL,"http://gts.paccypad.com/pokemonrse/worldexchange/exchange_finish?pid=\0");

        //Turn hex to str
        ConvertIntToHexStringN_v2(pidhex, gSaveBlock2Ptr->PID,STR_CONV_MODE_RIGHT_ALIGN,8);

        //Add PID to URL
        concat_str(pURL,(char *)pidhex);
        
        DebugPrintf(pURL);
        
        //Get hash
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            //Restore mon
            if(gSpecialVar_0x8004 == PC_MON_CHOSEN){
                memcpy(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos], &gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box,80);
            }
            else{
                CopyMon(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004],&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC],100);
            }
            
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }
        
        //Game has fully saved send final confirmation to server
        memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
        concat_str(halftoken,(char *)pRecvData);

        //Cleaning up pRecvData
        for(i=0;i<32;i++){
            pRecvData[i]='\0';
        }

        sha1digest((u8 *)hash,NULL,(u8 *)halftoken,52);

        //Add hash to URL
        concat_str(pURL,"&hash=");
        for(i = 0; i < 20; i++){
            ConvertIntToHexStringN_v2(pidhex, hash[i],STR_CONV_MODE_LEFT_ALIGN,2);
            pidhex[2]='\0';
            concat_str(pURL,(char *)pidhex);
        }

        //Add data to URL
        concat_str(pURL,"&data=");
        //Turn hex to str
        ConvertIntToHexStringN_v2(pidhex, sGTSPokedexView->searchResult[1].pid,STR_CONV_MODE_RIGHT_ALIGN,8); //???? This is giving players PID
        //Add PID to URL
        concat_str(pURL,(char *)pidhex);
        
        DebugPrintf(pURL);
        DebugPrintf("ZA");
        DebugPrintf((char *)pidhex);
        DebugPrintf("%u\n", sGTSPokedexView->searchResult[1].pid);
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }

        DebugPrintf("hi");
        DebugPrintf("%u\n", *pRecvData);
        
        if(pRecvData[1]==0x01){
            LinkFullSave_SetLastSectorSignature();
            data->state = GTS_STATE_TRADE_ANIMATION;
        }
        else{
            if(gSpecialVar_0x8004 == PC_MON_CHOSEN){
                memcpy(&gPokemonStoragePtr->boxes[gSpecialVar_MonBoxId][gSpecialVar_MonBoxPos], &gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC].box,80);
            }
            else{
                CopyMon(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004],&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC],100);
            }
            data->state = GTS_STATE_SERVER_ERROR;
        }

        break;
    case GTS_STATE_TRADE_ANIMATION:
        data->state = GTS_STATE_WAIT;
        //sGTSPokedexView->currentPage=2;
        //sGTSPokedexView->dexOrder=taskId;
        VarSet(VAR_UNUSED_0x40FF,GTS_CHECK_RESULT);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        //Free(GetBgTilemapBuffer(0));
        //Free(GetBgTilemapBuffer(1));
        //Free(GetBgTilemapBuffer(2));
        //Free(GetBgTilemapBuffer(3));
        DoGTSExchangeScene();
        break;
    case GTS_STATE_WAIT:
        break;
    case GTS_STATE_WITHDRAW_POKEMON:  //Done
        DebugPrintf("GTS_STATE_WITHDRAW_POKEMON");
        recvBufSize=32;
        concat_str(pURL,"http://gts.paccypad.com/pokemonrse/worldexchange/get?pid=\0");

        //Turn hex to str
        ConvertIntToHexStringN_v2(pidhex, gSaveBlock2Ptr->PID,STR_CONV_MODE_RIGHT_ALIGN,8);

        //Add PID to URL
        concat_str(pURL,(char *)pidhex);

        //Get hash
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            DebugPrintf("Fail 1");
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }

        memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
        concat_str(halftoken,(char *)pRecvData);

        //Cleaning up pRecvData
        for(i=0;i<32;i++){
            pRecvData[i]='\0';
        }

        sha1digest((u8 *)hash,NULL,(u8 *)halftoken,52);

        //Add hash to URL
        concat_str(pURL,"&hash=");
        for(i = 0; i < 20; i++){
            ConvertIntToHexStringN_v2(pidhex, hash[i],STR_CONV_MODE_LEFT_ALIGN,2);
            pidhex[2]='\0';
            concat_str(pURL,(char *)pidhex);
        }
        DebugPrintf(pURL);
        DebugPrintf("Send hash");
        recvBufSize=80;
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            DebugPrintf("Fail 2");
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }

        if(pRecvSize!=80){
            DebugPrintf("Fail 3");
            DebugPrintf("%u\n", pRecvSize);
            data->state = GTS_STATE_SERVER_ERROR;
            break;
        }

        memcpy(&sGTSPokedexView->searchResult[0].boxmon.personality,&pRecvData,80);

        DebugPrintf("Get summary");
        //data->state = GTS_STATE_RETRIEVE_POKEMON_YES_NO;
        DestroyTask(taskId);
        VarSet(VAR_UNUSED_0x40FF,GTS_STATE_RETRIEVE_POKEMON_YES_NO);
        memcpy(&gParties[B_TRAINER_OPPONENT_A][0].box,&sGTSPokedexView->searchResult[0].boxmon,80);
        //FreeAllWindowBuffers();
        //Free(GetBgTilemapBuffer(0));
        //Free(GetBgTilemapBuffer(1));
        //Free(GetBgTilemapBuffer(2));
        //Free(GetBgTilemapBuffer(3));
        ShowPokemonSummaryScreen(SUMMARY_MODE_BOX, &sGTSPokedexView->searchResult[0].boxmon, 0, 0, CB2_InitGlobalTradeStation);
        FreeAllWindowBuffers();

        break;
    case GTS_STATE_RETRIEVE_POKEMON_YES_NO: //Done
        input = DoGTSYesNo(&data->textState, &data->var, FALSE, gText_WithdrawPokemon2);
        switch (input)
        {
        case 0: // Yes, Retrieve Pokemon, since we stopepd and started the task we must fetch pokemon again
            //Check PC isn't full
            if(GiveBoxMonToPlayer(&sGTSPokedexView->searchResult[1].boxmon)==2){
                data->state = GTS_STATE_CLIENT_ERROR;
                break;
            }
            if(GiveBoxMonToPlayer(&gParties[B_TRAINER_OPPONENT_A][0].box)==2){
                data->state = GTS_STATE_CLIENT_ERROR;
                break;
            }
            data->state = GTS_STATE_SAVE_1;
            data->nextState = GTS_STATE_SAVE_RETRIEVED_POKEMON;
            break;
        case 1: // Go to Main Menu
        case MENU_B_PRESSED:
            data->state = GTS_CHECK_RESULT;
            data->nextState = GTS_STATE_MAIN_MENU;
        }
        break;
    case GTS_STATE_SAVE_RETRIEVED_POKEMON:  //Done
        // Choose where to access the Wonder Card/News from
        recvBufSize=32;
        concat_str(pURL,"http://gts.paccypad.com/pokemonrse/worldexchange/return?pid=\0");

        //Turn hex to str
        ConvertIntToHexStringN_v2(pidhex, gSaveBlock2Ptr->PID,STR_CONV_MODE_RIGHT_ALIGN,8);

        //Add PID to URL
        concat_str(pURL,(char *)pidhex);

        //Get hash
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }

        memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
        concat_str(halftoken,(char *)pRecvData);

        //Cleaning up pRecvData
        for(i=0;i<32;i++){
            pRecvData[i]='\0';
        }

        sha1digest((u8 *)hash,NULL,(u8 *)halftoken,52);

        //Add hash to URL
        concat_str(pURL,"&hash=");
        for(i = 0; i < 20; i++){
            ConvertIntToHexStringN_v2(pidhex, hash[i],STR_CONV_MODE_LEFT_ALIGN,2);
            pidhex[2]='\0';
            concat_str(pURL,(char *)pidhex);
        }

        recvBufSize=2;
        DebugPrintf(pURL);
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = GTS_STATE_CLIENT_ERROR;
            break;
        }

        if(pRecvData[1]==0x01){
            LinkFullSave_SetLastSectorSignature();
            DebugPrintf("Successfully retreived");
            sGTSPokedexView->currentPage=1;
            VarSet(VAR_UNUSED_0x40FF,GTS_STATE_MAIN_MENU);
            DestroyTask(taskId);
            FreeAllWindowBuffers();
            DoGTSExchangeScene();
            // /data->state = GTS_STATE_MAIN_MENU;
        }
        else{
            DebugPrintf("Lol fail");
            data->state = GTS_STATE_SERVER_ERROR;
        }
        break;
    case GTS_STATE_CLIENT_LINK_WAIT:
        if (gReceivedRemoteLinkPlayers != 0)
        {
            ClearScreenInBg0(TRUE);
            data->state = GTS_STATE_CLIENT_COMMUNICATING;
            MysteryGiftClient_Create(data->isWonderNews);
        }
        else if (gSpecialVar_Result == LINKUP_FAILED)
        {
            // Link failed, return to link start menu 
            ClearScreenInBg0(TRUE);
            data->state = GTS_STATE_SOURCE_PROMPT;
        }
        break;
    case GTS_STATE_CLIENT_COMMUNICATING:
        GTSAddTextPrinterToWindow1(gText_Communicating);
        data->state = GTS_STATE_CLIENT_LINK;
        break;
    case GTS_STATE_CLIENT_LINK:
        switch (MysteryGiftClient_Run(&data->var))
        {
        case CLI_RET_END:
            Rfu_SetCloseLinkCallback();
            data->msgId = data->var;
            data->state = GTS_STATE_CLIENT_LINK_END;
            break;
        case CLI_RET_COPY_MSG:
            memcpy(data->clientMsg, MysteryGiftClient_GetMsg(), 0x40);
            MysteryGiftClient_AdvanceState();
            break;
        case CLI_RET_PRINT_MSG:
            data->state = GTS_STATE_CLIENT_MESSAGE;
            break;
        case CLI_RET_YES_NO:
            data->state = GTS_STATE_CLIENT_YES_NO;
            break;
        case CLI_RET_ASK_TOSS:
            data->state = GTS_STATE_CLIENT_ASK_TOSS;
            StringCopy(gStringVar1, gLinkPlayers[0].name);
            break;
        }
        break;
    case GTS_STATE_CLIENT_YES_NO:
        input = DoGTSYesNo(&data->textState, &data->var, FALSE, MysteryGiftClient_GetMsg());
        switch (input)
        {
        case 0: // Yes
            MysteryGiftClient_SetParam(FALSE);
            MysteryGiftClient_AdvanceState();
            data->state = GTS_STATE_CLIENT_COMMUNICATING;
            break;
        case 1: // No
        case MENU_B_PRESSED:
            MysteryGiftClient_SetParam(TRUE);
            MysteryGiftClient_AdvanceState();
            data->state = GTS_STATE_CLIENT_COMMUNICATING;
            break;
        }
        break;
    case GTS_STATE_CLIENT_MESSAGE:
        if (PrintGTSMenuMessage(&data->textState, MysteryGiftClient_GetMsg()))
        {
            MysteryGiftClient_AdvanceState();
            data->state = GTS_STATE_CLIENT_COMMUNICATING;
        }
        break;
    case GTS_STATE_CLIENT_ASK_TOSS:
        // Player is receiving a new Wonder Card/News but needs to toss an existing one to make room.
        // Ask for confirmation.
        input = DoGTSYesNo(&data->textState, &data->var, FALSE, gText_ThrowAwayWonderCard);
        switch (input)
        {
        case 0: // Yes
            if (IsSavedWonderCardGiftNotReceived() == TRUE)
            {
                data->state = GTS_STATE_CLIENT_ASK_TOSS_UNRECEIVED;
            }
            else
            {
                MysteryGiftClient_SetParam(FALSE);
                MysteryGiftClient_AdvanceState();
                data->state = GTS_STATE_CLIENT_COMMUNICATING;
            }
            break;
        case 1: // No
        case MENU_B_PRESSED:
            MysteryGiftClient_SetParam(TRUE);
            MysteryGiftClient_AdvanceState();
            data->state = GTS_STATE_CLIENT_COMMUNICATING;
            break;
        }
        break;
    case GTS_STATE_CLIENT_ASK_TOSS_UNRECEIVED:
        // Player has selected to toss a Wonder Card that they haven't received the gift for.
        // Ask for confirmation again.
        input = DoGTSYesNo(&data->textState, &data->var, FALSE, gText_HaventReceivedCardsGift);
        switch (input)
        {
        case 0: // Yes
            MysteryGiftClient_SetParam(FALSE);
            MysteryGiftClient_AdvanceState();
            data->state = GTS_STATE_CLIENT_COMMUNICATING;
            break;
        case 1: // No
        case MENU_B_PRESSED:
            MysteryGiftClient_SetParam(TRUE);
            MysteryGiftClient_AdvanceState();
            data->state = GTS_STATE_CLIENT_COMMUNICATING;
            break;
        }
        break;
    case GTS_STATE_CLIENT_LINK_END:
        if (gReceivedRemoteLinkPlayers == 0)
        {
            DestroyWirelessStatusIndicatorSprite();
            data->state = GTS_STATE_CLIENT_COMM_COMPLETED;
        }
        break;
    case GTS_STATE_CLIENT_COMM_COMPLETED:
        if (PrintStringAndWait2Seconds(&data->textState, gText_CommunicationCompleted))
        {
            if (data->sourceIsFriend == TRUE)
                StringCopy(gStringVar1, gLinkPlayers[0].name);
            data->state = GTS_STATE_CLIENT_RESULT_MSG;
        }
        break;
    case GTS_STATE_CLIENT_RESULT_MSG:
        msg = GetClientResultMessage(&successMsg, data->isWonderNews, data->sourceIsFriend, data->msgId);
        if (msg == NULL)
            msg = data->clientMsg;
        if (successMsg)
            input = PrintSuccessMessage(&data->textState, msg, &data->var);
        else
            input = PrintGTSMenuMessage(&data->textState, msg);
        // input var re-used, here it is TRUE if the message is finished
        if (input)
        {
            if (!successMsg)
            {
                // Did not receive card/news, return to main menu
                data->state = GTS_STATE_MAIN_MENU;
                PrintGTSTopMenu(0, FALSE);
            }
            else
            {
                data->state = GTS_STATE_SAVE_LOAD_GIFT;
            }
        }
        break;
    case GTS_STATE_SAVE_LOAD_GIFT:
        if (SaveOnMysteryGiftMenu(&data->textState))
            data->state = GTS_STATE_LOAD_GIFT;
        break;
    case GTS_STATE_LOAD_GIFT:
        if (HandleLoadWonderCardOrNews(&data->textState, data->isWonderNews))
            data->state = GTS_STATE_HANDLE_GIFT_INPUT;
        break;
    case GTS_STATE_HANDLE_GIFT_INPUT:
        if (!data->isWonderNews)
        {
            // Handle Wonder Card input
            if (JOY_NEW(A_BUTTON))
                data->state = GTS_STATE_HANDLE_GIFT_SELECT;
            if (JOY_NEW(B_BUTTON))
                data->state = GTS_STATE_GIFT_INPUT_EXIT;
        }
        else
        {
            switch (WonderNews_GetInput(gMain.newKeys))
            {
            case NEWS_INPUT_A:
                WonderNews_RemoveScrollIndicatorArrowPair();
                data->state = GTS_STATE_HANDLE_GIFT_SELECT;
                break;
            case NEWS_INPUT_B:
                data->state = GTS_STATE_GIFT_INPUT_EXIT;
                break;
            }
        }
        break;
    case GTS_STATE_HANDLE_GIFT_SELECT:
    {
        // A Wonder Card/News has been selected, handle its menu
        u32 result;
        if (!data->isWonderNews)
        {
            if (IsSendingSavedWonderCardAllowed())
                result = HandleGiftSelectMenu(&data->textState, &data->var, data->isWonderNews, FALSE);
            else
                result = HandleGiftSelectMenu(&data->textState, &data->var, data->isWonderNews, TRUE);
        }
        else
        {
            if (IsSendingSavedWonderNewsAllowed())
                result = HandleGiftSelectMenu(&data->textState, &data->var, data->isWonderNews, FALSE);
            else
                result = HandleGiftSelectMenu(&data->textState, &data->var, data->isWonderNews, TRUE);
        }
        switch (result)
        {
        case 0: // Receive
            data->state = GTS_STATE_RECEIVE;
            break;
        case 1: // Send
            data->state = GTS_STATE_SEND;
            break;
        case 2: // Toss
            data->state = GTS_STATE_ASK_TOSS;
            break;
        case LIST_CANCEL:
            if (data->isWonderNews == TRUE)
                WonderNews_AddScrollIndicatorArrowPair();
            data->state = GTS_STATE_HANDLE_GIFT_INPUT;
            break;
        }
        break;
    }
    case GTS_STATE_ASK_TOSS:
        // Player is attempting to discard a saved Wonder Card/News
        switch (AskDiscardGift(&data->textState, &data->var, data->isWonderNews))
        {
        case 0: // Yes
            if (!data->isWonderNews && IsSavedWonderCardGiftNotReceived() == TRUE)
                data->state = GTS_STATE_ASK_TOSS_UNRECEIVED;
            else
                data->state = GTS_STATE_TOSS;
            break;
        case 1: // No
        case MENU_B_PRESSED:
            data->state = GTS_STATE_HANDLE_GIFT_SELECT;
            break;
        }
        break;
    case GTS_STATE_ASK_TOSS_UNRECEIVED:
        // Player has selected to toss a Wonder Card that they haven't received the gift for.
        // Ask for confirmation again.
        switch ((u32)DoGTSYesNo(&data->textState, &data->var, TRUE, gText_HaventReceivedGiftOkayToDiscard))
        {
        case 0: // Yes
            data->state = GTS_STATE_TOSS;
            break;
        case 1: // No
        case MENU_B_PRESSED:
            data->state = GTS_STATE_HANDLE_GIFT_SELECT;
            break;
        }
        break;
    case GTS_STATE_TOSS:
        if (ExitWonderCardOrNews(data->isWonderNews, TRUE))
        {
            ClearSavedNewsOrCard(data->isWonderNews);
            data->state = GTS_STATE_TOSS_SAVE;
        }
        break;
    case GTS_STATE_TOSS_SAVE:
        if (SaveOnMysteryGiftMenu(&data->textState))
            data->state = GTS_STATE_TOSSED;
        break;
    case GTS_STATE_TOSSED:
        if (PrintThrownAway(&data->textState, data->isWonderNews))
        {
            data->state = GTS_STATE_MAIN_MENU;
            PrintGTSTopMenu(0, FALSE);
        }
        break;
    case GTS_STATE_GIFT_INPUT_EXIT:
        if (ExitWonderCardOrNews(data->isWonderNews, FALSE))
            data->state = GTS_STATE_MAIN_MENU;
        break;
    case GTS_STATE_RECEIVE:
        if (ExitWonderCardOrNews(data->isWonderNews, TRUE))
            data->state = GTS_STATE_SOURCE_PROMPT;
        break;
    case GTS_STATE_SEND:
        if (ExitWonderCardOrNews(data->isWonderNews, TRUE))
        {
            switch (data->isWonderNews)
            {
            case FALSE:
                CreateTask_SendMysteryGift(ACTIVITY_WONDER_CARD);
                break;
            case TRUE:
                CreateTask_SendMysteryGift(ACTIVITY_WONDER_NEWS);
                break;
            }
            data->sourceIsFriend = TRUE;
            data->state = GTS_STATE_SERVER_LINK_WAIT;
        }
        break;
    case GTS_STATE_SERVER_LINK_WAIT:
        if (gReceivedRemoteLinkPlayers != 0)
        {
            ClearScreenInBg0(TRUE);
            data->state = GTS_STATE_SERVER_LINK_START;
        }
        else if (gSpecialVar_Result == LINKUP_FAILED)
        {
            ClearScreenInBg0(TRUE);
            data->state = GTS_STATE_LOAD_GIFT;
        }
        break;
    case GTS_STATE_SERVER_LINK_START:
        *gStringVar1 = EOS;
        *gStringVar2 = EOS;
        *gStringVar3 = EOS;

        if (!data->isWonderNews)
        {
            GTSAddTextPrinterToWindow1(gText_SendingWonderCard);
            MysterGiftServer_CreateForCard();
        }
        else
        {
            GTSAddTextPrinterToWindow1(gText_SendingWonderNews);
            MysterGiftServer_CreateForNews();
        }
        data->state = GTS_STATE_SERVER_LINK;
        break;
    case GTS_STATE_SERVER_LINK:
        if (MysterGiftServer_Run(&data->var) == SVR_RET_END)
        {
            data->msgId = data->var;
            data->state = GTS_STATE_SERVER_LINK_END;
        }
        break;
    case GTS_STATE_SERVER_LINK_END:
        Rfu_SetCloseLinkCallback();
        StringCopy(gStringVar1, gLinkPlayers[1].name);
        data->state = GTS_STATE_SERVER_LINK_END_WAIT;
        break;
    case GTS_STATE_SERVER_LINK_END_WAIT:
        if (gReceivedRemoteLinkPlayers == 0)
        {
            DestroyWirelessStatusIndicatorSprite();
            data->state = GTS_STATE_SERVER_RESULT_MSG;
        }
        break;
    case GTS_STATE_SERVER_RESULT_MSG:
        break;
    case GTS_STATE_CLIENT_ERROR:
    case GTS_STATE_SERVER_ERROR:
        if (PrintGTSMenuMessage(&data->textState, gText_CommunicationError))
        {
            data->state = GTS_STATE_EXIT;
            PrintGTSTopMenu(0, FALSE);
        }
        break;
    case GTS_STATE_EXIT:
        CloseLink();
        Free(data->clientMsg);
        DestroyTask(taskId);
        VarSet(VAR_UNUSED_0x40FF,GTS_STATE_TO_MAIN_MENU);
        SetMainCallback2(MainCB_GTSFreeAllBuffersAndReturnToInitTitleScreen);
        break;
    }
}

/*
Offset 			Contents
int 0x00-0x03 	X	Checksum (sum of all bytes xor 0x2db842b2)
int 0x04-0x07 	X	pid
bin 0x08-0x6C 	X	Encrypted party pokemon data structure
sma 0x6D-0x6E 	X	Nat. Dex ID
bin 0x6F 		X	Gender
bin 0x70 		X	Level
sma 0x71-0x72 	X	Requested Nat. Dex ID
bin 0x73 		X	Requested Gender
bin 0x74 		X	Requested Min Level
bin 0x75 		X	Requested Max Level
bin 0x76 		X	Trainer gender
sma 0x77-0x78	X	Trainer ID
sma 0x79-0x7A	X	Secret ID
str 0x7B-0x8A	X	OT Name (Unicode, must end with 0xFFFF)
bin 0x8B 		X	Country
bin 0x8C 		X	Region
bin 0x8D 		X	Trainer class/sprite
boo 0x8E 		X	Is Exchanged Flag (Always 0)
sma 0x8F-0x90   X	Game version
sma 0x91-0x92	X	Game RomHackID
sma 0x93-0x94		X	RomHackVer
bin 0x95 		X	Language


worldexchange/search.asp

u16 species
u8 gender
u8 minlevel
u8 maxLevel
u8 pageNo
u8 resultscount = 7, how many pokemon requested
u8 country

*/