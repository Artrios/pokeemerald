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
#include "internet_options_menu.h"
#include "union_room.h"
#include "title_screen.h"
#include "ereader_screen.h"
#include "international_string_util.h"
#include "list_menu.h"
#include "string_util.h"
#include "mystery_gift.h"
#include "mystery_gift_view.h"
#include "mystery_gift_menu.h"
#include "save.h"
#include "link.h"
#include "mystery_gift_client.h"
#include "mystery_gift_server.h"
#include "event_data.h"
#include "link_rfu.h"
#include "overworld.h"
#include "party_menu.h"
#include "pokedex.h"
#include "wonder_news.h"
#include "constants/cable_club.h"
#include "constants/party_menu.h"
#include "field_weather.h"
#include "constants/rgb.h"
#include "field_screen_effect.h"
#include "trade.h"
#include "trainer_pokemon_sprites.h"
#include "trig.h"
#include "mobile_adapter.h"

#define LIST_MENU_TILE_NUM 10
#define LIST_MENU_PAL_NUM 224

#define LETTER_IN_RANGE_UPPER(letter, range) \
    ((letter) >= sLetterSearchRanges[range][0]                                  \
  && (letter) < sLetterSearchRanges[range][0] + sLetterSearchRanges[range][1])  \

#define LETTER_IN_RANGE_LOWER(letter, range) \
    ((letter) >= sLetterSearchRanges[range][2]                                  \
  && (letter) < sLetterSearchRanges[range][2] + sLetterSearchRanges[range][3])  \


static void LoadMysteryGiftTextboxBorder(u8 bgId);
static void CreateInternetOptionsTask(void);
static void Task_InternetOptions(u8 taskId);

EWRAM_DATA static u8 sDownArrowCounterAndYCoordIdx[8] = {};
EWRAM_DATA struct InternetOptionsPokedexView *sInternetOptionsPokedexView = NULL;

static const u16 sTextboxBorder_Pal[] = INCBIN_U16("graphics/interface/mystery_gift_textbox_border.gbapal");
static const u32 sTextboxBorder_Gfx[] = INCBIN_U32("graphics/interface/mystery_gift_textbox_border.4bpp.lz");

// const rom data
//#include "data/pokemon/pokedex_orders.h"
extern const u16 gPokedexOrder_Alphabetical[];

struct InternetOptionsTaskData
{
    u16 var; // Multipurpose
    u16 depositPokemon;
    u16 searchPokemon;
    u16 errorNum;
    u8 state;
    u8 nextstate;
    u8 textState;
    u8 unused5;
    bool8 isWonderNews;
    bool8 sourceIsFriend;
    u8 msgId;
    u8 * clientMsg;
};

struct profile
{
    u8 version;
    u32 romhackID;
    u16 romhackVer;
    u8 language;
    u8 country;
    u8 region;
    u8 trainerID[4];
    u8 trainerName[16];
    u8 MAC[6];
    u8 email[56];
    u32 notify;
    u16 clientSecret;
    u16 mailSecret;
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

static const struct WindowTemplate sWindowTemplate_FiveOptions = {
    .bg = 0,
    .tilemapLeft = 8,
    .tilemapTop = 6,
    .width = 14,
    .height = 10,
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

static const struct ListMenuItem sListMenuItems_InternetOptions[] = {
    { gText_GiftDownload,  0 },
    { gText_Bank,   1 },
    { gText_Friends,   2 },
    { gText_Sync,   3 },
    { gText_Exit3,        LIST_CANCEL }
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

static const struct ListMenuTemplate sListMenuTemplate_FiveOptions = {
    .items = NULL,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 5,
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

static const struct ListMenuTemplate sListMenu_Levels = {
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

static const struct ListMenuTemplate sListMenu_LevelsWanted = {
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
ALIGNED(2) static const u8 sInternetOptions_Ereader_TextColor_2[]  = { TEXT_COLOR_WHITE,       TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY };


struct SearchOptionText
{
    const u8 *title;
};

void concat_str(char *dest, const char *src)
{
    while (*dest != '\0') dest++;
    for (;;) {
        *dest++ = *src;
        if (*src == '\0') break;
        src++;
    }
}


static void VBlankCB_MysteryGiftEReader(void)
{
    ProcessSpriteCopyRequests();
    LoadOam();
    TransferPlttBuffer();
}

void CB2_InternetOptions(void)
{
    RunTasks();
    RunTextPrinters();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static bool32 HandleInternetOptionsSetup()
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
        InternetOptions_DrawCheckerboardPattern(3);
        PrintInternetOptionsTopMenu(FALSE);
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

void CB2_InitInternetOptions(void)
{
    if (HandleInternetOptionsSetup())
    {
        FadeInNewBGM(MUS_RG_MYSTERY_GIFT,4);
        SetMainCallback2(CB2_InternetOptions);
        CreateInternetOptionsTask();
    }
    RunTasks();
}

void PrintInternetOptionsTopMenu(bool32 connecting)
{
    const u8 * header;
    const u8 * options;
    options = !connecting ? gJPText_Connecting : gText_PickOKCancel;
    
    FillWindowPixelBuffer(0, 0);
    if (gSaveBlock2Ptr->PID==0xFFFFFFFF)
    {
        header = gText_InternetOptions;
    }
    else
    {
        pid_to_fc(gSaveBlock2Ptr->PID,(u8 *)options);
        header = gText_FC;
        concat_str((char *)header,(char *)options);
    }

    options = !connecting ? gJPText_Connecting : gText_PickOKCancel;

    AddTextPrinterParameterized4(0, FONT_NORMAL, 4, 1, 0, 0, sTextColors_TopMenu, TEXT_SKIP_DRAW, header);
    AddTextPrinterParameterized4(0, FONT_SMALL, GetStringRightAlignXOffset(FONT_SMALL, options, 0xDE), 1, 0, 0, sTextColors_TopMenu, TEXT_SKIP_DRAW, options);
    CopyWindowToVram(0, COPYWIN_GFX);
    PutWindowTilemap(0);
}

void InternetOptions_DrawTextBorder(u8 windowId)
{
    DrawTextBorderOuter(windowId, 0x01, 0xF);
}

void InternetOptions_DrawCheckerboardPattern(u32 bg)
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

void InternetOptionsAddTextPrinterToWindow1(const u8 *str)
{
    StringExpandPlaceholders(gStringVar4, str);
    FillWindowPixelBuffer(1, 0x11);
    AddTextPrinterParameterized4(1, FONT_NORMAL, 0, 1, 0, 0, sInternetOptions_Ereader_TextColor_2, 0, gStringVar4);
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

bool32 PrintInternetOptionsMenuMessage(u8 *textState, const u8 *str)
{
    switch (*textState)
    {
    case 0:
        InternetOptionsAddTextPrinterToWindow1(str);
        (*textState)++;
        break;
    case 1:
        DrawDownArrow(1, DOWN_ARROW_X, DOWN_ARROW_Y, 1, FALSE, &sDownArrowCounterAndYCoordIdx[0], &sDownArrowCounterAndYCoordIdx[1]);
        if (({JOY_NEW(A_BUTTON);}))
            (*textState)++;
        if (({JOY_NEW(B_BUTTON);}))
            //(*textState)==3;
            return TRUE;
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

static u32 InternetOptions_HandleThreeOptionMenu(u8 * unused0, u16 * unused1, u8 whichMenu)
{
    struct ListMenuTemplate listMenuTemplate = sListMenuTemplate_FiveOptions;
    struct WindowTemplate windowTemplate = sWindowTemplate_FiveOptions;
    s32 width;
    s32 response;

    if (whichMenu == 0)
        listMenuTemplate.items = sListMenuItems_InternetOptions;
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

s8 DoInternetOptionsYesNo(u8 * textState, u16 * windowId, bool8 yesNoBoxPlacement, const u8 * str)
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
        AddTextPrinterParameterized4(*windowId, FONT_NORMAL, 0, 1, 0, 0, sInternetOptions_Ereader_TextColor_2, 0, gStringVar4);
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


static bool32 PrintSuccessMessage(u8 * state, const u8 * msg, u16 * timer)
{
    switch (*state)
    {
    case 0:
        if (msg != NULL)
            InternetOptionsAddTextPrinterToWindow1(msg);
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

static bool32 SaveOnInternetOptionMenu(u8 * state)
{
    switch (*state)
    {
    case 0:
        InternetOptionsAddTextPrinterToWindow1(gText_DataWillBeSaved);
        (*state)++;
        break;
    case 1:
        TrySavingData(SAVE_NORMAL);
        (*state)++;
        break;
    case 2:
        InternetOptionsAddTextPrinterToWindow1(gText_SaveCompletedPressA);
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
        return PrintInternetOptionsMenuMessage(state, str);
}

void base64_encode(u32 checksum, char *data, size_t input_length, char *encoded_data) {
 
    int output_length = 4 * ((input_length + 2) / 3);
    int mod_table[] = {0, 2, 1};
    char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '-', '_'};

    char input_data[output_length+1];

    input_data[0] = checksum >> 24 & 0xFF;
    input_data[1] = checksum >> 16 & 0xFF;
    input_data[2] = checksum >> 8 & 0xFF;
    input_data[3] = checksum & 0xFF;

    for (int i = 4; i < input_length; i++) {
        input_data[i] = data[i-4];
    }

    //char encoded_data[output_length+1];
    //if (encoded_data == NULL) return NULL;
    DebugPrintf("Yello");
    DebugPrintf(input_data);
    DebugPrintf(encoded_data);
 
    for (int i = 0, j = 0; i < input_length;) {
 
        uint32_t octet_a = i < input_length ? input_data[i++] : 0;
        uint32_t octet_b = i < input_length ? input_data[i++] : 0;
        uint32_t octet_c = i < input_length ? input_data[i++] : 0;
 
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
 
        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    DebugPrintf("Uploading 3.3111");
    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';
    DebugPrintf("Uploading 3.3111111");
    encoded_data[output_length]='\0';
    DebugPrintf("Uploading 3.311111111111");
    DebugPrintf(encoded_data);
    DebugPrintf("Uploading 3.yyyyy");
    //return *encoded_data;
}

int encrypt_data(u32 pid, char *data, int datasize){
    int checksum = 0;
    int i = 0;
    checksum = checksum + (pid >> 24 & 0xFF);
    DebugPrintf("%u",checksum);
    checksum = checksum + (pid >> 16 & 0xFF);
    DebugPrintf("%u",checksum);
    checksum = checksum + (pid >> 8 & 0xFF);
    DebugPrintf("%u",checksum);
    checksum = checksum + (pid & 0xFF);

    for(i=4; i<datasize; i++){
        DebugPrintf("%u",data[i]);
        checksum = checksum + data[i];
    }
    checksum = 0x4a3b2c1d ^ checksum;

    int GRNG = checksum | (checksum << 16);
    u8 keystream = (GRNG >> 16) & 0xFF;
    i=0;
    
    DebugPrintf("%u",datasize);
    while(i<datasize){
        data[i]=data[i] ^ keystream;
        GRNG = (GRNG * 0x45 + 0x1111) & 0x7FFFFFFF;
        keystream = (GRNG >> 16) & 0xFF;
        i++;
    }
    
    return checksum;
}


static const u8 crc_table[] = {
 0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31,
 0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9,
 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
 0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1,
 0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe,
 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
 0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16,
 0x03, 0x04, 0x0d, 0x0a, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80,
 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
 0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8,
 0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10,
 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
 0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f,
 0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7,
 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
 0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef,
 0xfa, 0xfd, 0xf4, 0xf3
};

uint8_t gencrc8(uint8_t *data)
{
    uint8_t crc = 0xff;
    size_t i;
    for (i = 0; i < 8; i++) {
        crc ^= data[i];
        crc = crc_table[crc ^ data[i]];
    }
    return crc;
}

/*******************************************************************************
 * sha1digest: https://github.com/CTrabant/teeny-sha1
 *
 * Calculate the SHA-1 value for supplied data buffer and generate a
 * text representation in hexadecimal.
 *
 * Based on https://github.com/jinqiangshou/EncryptionLibrary, credit
 * goes to @jinqiangshou, all new bugs are mine.
 *
 * @input:
 *    data      -- data to be hashed
 *    databytes -- bytes in data buffer to be hashed
 *
 * @output:
 *    digest    -- the result, MUST be at least 20 bytes
 *    hexdigest -- the result in hex, MUST be at least 41 bytes
 *
 * At least one of the output buffers must be supplied.  The other, if not 
 * desired, may be set to NULL.
 *
 * @return: 0 on success and non-zero on error.
 ******************************************************************************/

int sha1digest(uint8_t *digest, u8 *hexdigest, const uint8_t *data, size_t databytes){
    #define SHA1ROTATELEFT(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

    uint32_t W[80];
    uint32_t H[] = {0x67452301,
                    0xEFCDAB89,
                    0x98BADCFE,
                    0x10325476,
                    0xC3D2E1F0};

    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t f = 0;
    uint32_t k = 0;

    uint32_t idx;
    uint32_t lidx;
    uint32_t widx;
    uint32_t didx = 0;

    int32_t wcount;
    uint32_t temp;
    uint64_t databits = ((uint64_t)databytes) * 8;
    uint32_t loopcount = (databytes + 8) / 64 + 1;
    uint32_t tailbytes = 64 * loopcount - databytes;
    uint8_t datatail[128] = {0};

    if (!digest && !hexdigest)
        return -1;

    if (!data)
        return -1;

    /* Pre-processing of data tail (includes padding to fill out 512-bit chunk):
        Add bit '1' to end of message (big-endian)
        Add 64-bit message length in bits at very end (big-endian) */
    datatail[0] = 0x80;
    datatail[tailbytes - 8] = (uint8_t) (databits >> 56 & 0xFF);
    datatail[tailbytes - 7] = (uint8_t) (databits >> 48 & 0xFF);
    datatail[tailbytes - 6] = (uint8_t) (databits >> 40 & 0xFF);
    datatail[tailbytes - 5] = (uint8_t) (databits >> 32 & 0xFF);
    datatail[tailbytes - 4] = (uint8_t) (databits >> 24 & 0xFF);
    datatail[tailbytes - 3] = (uint8_t) (databits >> 16 & 0xFF);
    datatail[tailbytes - 2] = (uint8_t) (databits >> 8 & 0xFF);
    datatail[tailbytes - 1] = (uint8_t) (databits >> 0 & 0xFF);

    /* Process each 512-bit chunk */
    for (lidx = 0; lidx < loopcount; lidx++){
        /* Compute all elements in W */
        memset (W, 0, 80 * sizeof (uint32_t));

        /* Break 512-bit chunk into sixteen 32-bit, big endian words */
        for (widx = 0; widx <= 15; widx++)
        {
            wcount = 24;

            /* Copy byte-per byte from specified buffer */
            while (didx < databytes && wcount >= 0){
                W[widx] += (((uint32_t)data[didx]) << wcount);
                didx++;
                wcount -= 8;
            }
            /* Fill out W with padding as needed */
            while (wcount >= 0){
                W[widx] += (((uint32_t)datatail[didx - databytes]) << wcount);
                didx++;
                wcount -= 8;
            }
        }

        /* Extend the sixteen 32-bit words into eighty 32-bit words, with potential optimization from:
            "Improving the Performance of the Secure Hash Algorithm (SHA-1)" by Max Locktyukhin */
        for (widx = 16; widx <= 31; widx++){
            W[widx] = SHA1ROTATELEFT ((W[widx - 3] ^ W[widx - 8] ^ W[widx - 14] ^ W[widx - 16]), 1);
        }
        for (widx = 32; widx <= 79; widx++){
            W[widx] = SHA1ROTATELEFT ((W[widx - 6] ^ W[widx - 16] ^ W[widx - 28] ^ W[widx - 32]), 2);
        }

        /* Main loop */
        a = H[0];
        b = H[1];
        c = H[2];
        d = H[3];
        e = H[4];

        for (idx = 0; idx <= 79; idx++){
            if (idx <= 19){
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            }
            else if (idx >= 20 && idx <= 39){
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if (idx >= 40 && idx <= 59){
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            }
            else if (idx >= 60 && idx <= 79){
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            temp = SHA1ROTATELEFT (a, 5) + f + e + k + W[idx];
            e = d;
            d = c;
            c = SHA1ROTATELEFT (b, 30);
            b = a;
            a = temp;
        }

        H[0] += a;
        H[1] += b;
        H[2] += c;
        H[3] += d;
        H[4] += e;
    }

    /* Store binary digest in supplied buffer */
    if (digest){
        for (idx = 0; idx < 5; idx++){
            digest[idx * 4 + 0] = (uint8_t) (H[idx] >> 24);
            digest[idx * 4 + 1] = (uint8_t) (H[idx] >> 16);
            digest[idx * 4 + 2] = (uint8_t) (H[idx] >> 8);
            digest[idx * 4 + 3] = (uint8_t) (H[idx]);
        }
    }

    /* Store hex version of digest in supplied buffer */
    ConvertIntToHexStringN_v2(hexdigest, H[0], STR_CONV_MODE_RIGHT_ALIGN, 8);
    ConvertIntToHexStringN_v2(hexdigest+8, H[1], STR_CONV_MODE_RIGHT_ALIGN, 8);
    ConvertIntToHexStringN_v2(hexdigest+16, H[2], STR_CONV_MODE_RIGHT_ALIGN, 8);
    ConvertIntToHexStringN_v2(hexdigest+24, H[3], STR_CONV_MODE_RIGHT_ALIGN, 8);
    ConvertIntToHexStringN_v2(hexdigest+32, H[4], STR_CONV_MODE_RIGHT_ALIGN, 8);
    //if (hexdigest){
    //    snprintf (hexdigest, 41, "%08x%08x%08x%08x%08x",H[0],H[1],H[2],H[3],H[4]);
    //}

    return 0;
}

void pid_to_fc(u32 pid, u8 *hexdigest) {
    if (pid == 0)
        // behaviour observed in MKWii, more likely an error condition
        hexdigest[0]='z';
    else {
        u8 i = 0;
        u8 buffer[8];
        // buffer is pid in little endian, followed by RMCJ in little endian
        buffer[0] = pid >> 0;
        buffer[1] = pid >> 8;
        buffer[2] = pid >> 16;
        buffer[3] = pid >> 24;

        buffer[4] = 'E'; // the reversed online relevant game id
        buffer[5] = 'E';
        buffer[6] = 'P';
        buffer[7] = 'B';

        // md5digest computes the digest of the second parameter and stores it in the first.
        u64 checksum = gencrc8(buffer);
        checksum = ((checksum << 32) | pid);

        ConvertUIntToDecimalStringN(hexdigest, checksum, STR_CONV_MODE_LEADING_ZEROS, 16);
        for(i=0;i<12;i++){
            hexdigest[i]=hexdigest[i+4];
        }

        for(i=13;i>9;i--){
            hexdigest[i]=hexdigest[i-2];
        }
        for(i=8;i>4;i--){
            hexdigest[i]=hexdigest[i-1];
        }
        hexdigest[4]='-';
        hexdigest[9]='-';
        hexdigest[14]='\0';
    }
}

// States for Task_MysteryGift.
// CLIENT states are for when the player is receiving a gift, and use mystery_gift_client.c link functions.
// SERVER states are for when the player is sending a gift, and use mystery_gift_server.c link functions.
// Other states handle the general Mystery Gift menu usage.
enum {
    INTERNET_STATE_TO_MAIN_MENU,
    INTERNET_STATE_MAIN_MENU,
    INTERNET_MA_CONNECTED,
    INTERNET_CONNECT_TO_SERVER,
    INTERNET_CONNECTING,
    INTERNET_GET_TOKEN,
    INTERNET_SHOW_FRIENDCODE,
    INTERNET_PING_SERVER,
    INTERNET_SET_PROFILE,
    INTERNET_ASK_PID,
    INTERNET_ASK_COUNTRY,
    INTERNET_ASK_REGION,
    INTERNET_GET_PID,
    INTERNET_INITIAL_SETUP,
    INTERNET_STATE_SEEK_SETUP,
    INTERNET_STATE_SEEKING,
    INTERNET_STATE_SEARCH_POKEMON,
    INTERNET_STATE_SEARCH_POKEMON_LIST,
    INTERNET_STATE_SEARCH_POKEMON_LEVEL_LIST,
    INTERNET_STATE_FETCHING_POKEMON,
    INTERNET_STATE_SELECT_FETCHED_POKEMON,
    INTERNET_STATE_START_SEARCH,
    INTERNET_STATE_SUCCESSFUL_SEARCH,
    INTERNET_STATE_UNSUCCESSFUL_SEARCH,
    INTERNET_STATE_FIND_MATCH,
    INTERNET_STATE_TRADE_MATCH,
    INTERNET_STATE_DEPOSIT_POKEMON,
    INTERNET_STATE_PICK_WANTED_POKEMON,
    INTERNET_STATE_DEPOSITING_POKEMON,
    INTERNET_STATE_POKEMON_LIST,
    INTERNET_STATE_POKEMON_LEVEL_LIST,
    INTERNET_STATE_RETURN_POKEMON_LIST,
    INTERNET_STATE_CONFIRM_OFFER,
    INTERNET_STATE_WAIT,
    INTERNET_STATE_WITHDRAW_POKEMON,
    INTERNET_STATE_CLIENT_LINK_START,
    INTERNET_STATE_CLIENT_LINK_WAIT,
    INTERNET_STATE_CLIENT_COMMUNICATING,
    INTERNET_STATE_CLIENT_LINK,
    INTERNET_STATE_CLIENT_YES_NO,
    INTERNET_STATE_CLIENT_MESSAGE,
    INTERNET_STATE_CLIENT_ASK_TOSS,
    INTERNET_STATE_CLIENT_ASK_TOSS_UNRECEIVED,
    INTERNET_STATE_CLIENT_LINK_END,
    INTERNET_STATE_CLIENT_COMM_COMPLETED,
    INTERNET_STATE_CLIENT_RESULT_MSG,
    INTERNET_STATE_CLIENT_ERROR,
    INTERNET_STATE_SAVE_LOAD_GIFT,
    INTERNET_STATE_LOAD_GIFT,
    INTERNET_STATE_UNUSED,
    INTERNET_STATE_HANDLE_GIFT_INPUT,
    INTERNET_STATE_HANDLE_GIFT_SELECT,
    INTERNET_STATE_ASK_TOSS,
    INTERNET_STATE_ASK_TOSS_UNRECEIVED,
    INTERNET_STATE_TOSS,
    INTERNET_STATE_TOSS_SAVE,
    INTERNET_STATE_TOSSED,
    INTERNET_STATE_GIFT_INPUT_EXIT,
    INTERNET_STATE_RECEIVE,
    INTERNET_STATE_SEND,
    INTERNET_STATE_SERVER_LINK_WAIT,
    INTERNET_STATE_SERVER_LINK_START,
    INTERNET_STATE_SERVER_LINK,
    INTERNET_STATE_SERVER_LINK_END,
    INTERNET_STATE_SERVER_LINK_END_WAIT,
    INTERNET_STATE_SERVER_RESULT_MSG,
    INTERNET_STATE_SERVER_ERROR,
    INTERNET_SAVE_GAME,
    INTERNET_STATE_EXIT,
    INTERNET_STATE_SOURCE_PROMPT,
    INTERNET_STATE_SOURCE_PROMPT_INPUT,
};

static void CreateInternetOptionsTask(void)
{
    u8 taskId = CreateTask(Task_InternetOptions, 0);
    struct InternetOptionsTaskData * data = (void *)gTasks[taskId].data;
    data->state = INTERNET_MA_CONNECTED;
    data->textState = 0;
    data->nextstate = 0;
    data->unused5 = 0;
    data->isWonderNews = 0;
    data->sourceIsFriend = 0;
    data->var = 0;
    data->depositPokemon = 0;
    data->searchPokemon = 0;
    data->errorNum = 0;
    data->msgId = 0;
    data->clientMsg = AllocZeroed(CLIENT_MAX_MSG_SIZE);
}

#define error       data[0]
#define errorType   data[1]

static void Task_InternetOptions(u8 taskId)
{
    struct InternetOptionsTaskData *data = (void *)gTasks[taskId].data;
    char pUserID[32];  //User ID from the MA EEPROM, has max lenght of 32
    pUserID[0]='\0';
    char pPassword[16];    //User password from the MA EEPROM, has max lenght of 16
    pPassword[0]='\0';
    char maMailID[30]; //User mail ID from the MA EEPROM
    char pURL[1024];
    memcpy(pURL, "\0", 1);
    u16 recvBufSize = 100;  //Size of received data
    u8 pRecvData[recvBufSize];  //Buffer to hold received data
    u16 pRecvSize;  //How many bytes were copied to pRecvData after calling maUpload once
    char halftoken[53];
    char hash[41];
    int input = 0;
    int tracker = 0;
    u32 pid = 0;
    u32 checksum = 0;
    struct profile *userprofile = AllocZeroed(sizeof(struct profile));
    char *encoded_data = 0;
    u8 pidhex[8];


    MA_TELDATA maTel;   //MA Telephone struct

    switch (data->state)
    {
    case INTERNET_STATE_TO_MAIN_MENU:
        data->state = INTERNET_STATE_MAIN_MENU;
        break;
    case INTERNET_MA_CONNECTED:
        DebugPrintf("INTERNET_MA_CONNECTED");
        if (!gPaletteFade.active)
            {
            if(maConnected()){
                data->state = INTERNET_CONNECT_TO_SERVER;
            }
            else{
                data->state = INTERNET_STATE_EXIT;
            }
        }
        break;
    case INTERNET_CONNECT_TO_SERVER:
        DebugPrintf("INTERNET_CONNECT_TO_SERVER");
        //Initialise MA Library
        data->errorNum = maInitLibrary();

        //If there was an error stop MA Library and return the error code
        if(data->errorNum !=0){
            maKill();
            data->state = INTERNET_STATE_EXIT;
        }
        
        //Get EEPROM Data
        data->errorNum=maGetEEPROMData(&maTel, pUserID, maMailID);

        //If there was an error stop MA Library and return the error code
        if(data->errorNum !=0){
            maKill();
            data->state = INTERNET_STATE_EXIT;
        }

        //Set your password, must end in Null byte
        memcpy(pPassword,"password1", 10);

        //Makes a call and establishes a PPP connection 
        data->errorNum=maConnectServer(&maTel,pUserID,pPassword);

        //If there was an error stop MA Library and return the error code
        if(data->errorNum !=0){
            maKill();
            data->state = INTERNET_STATE_EXIT;
        }

        data->state = INTERNET_PING_SERVER;
        break;
    case INTERNET_PING_SERVER:
        DebugPrintf("INTERNET_PING_SERVER");
        recvBufSize=4;
        concat_str(pURL,"http://www.PutYourDomainHere.com/pokemonrse/worldexchange/info\0");
        //Request a PID
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, pUserID, pPassword);
        if(data->errorNum !=0){
            maKill();
            data->state = INTERNET_STATE_EXIT;
        }

        recvBufSize=(pRecvData[0]<<8)+pRecvData[1];
        DebugPrintf("%u\n", recvBufSize);

        if(recvBufSize==0x0001){
            if(gSaveBlock2Ptr->PID==0xFFFFFFFF){
                data->state = INTERNET_ASK_PID;
            }
            else {
                data->state = INTERNET_SET_PROFILE;
            }
            break;
        }
        break;
    case INTERNET_SET_PROFILE:
        DebugPrintf("INTERNET_SET_PROFILE");
        concat_str(pURL,"http://www.PutYourDomainHere.com/pokemonrse/common/setprofile?pid=\0");
        //Turn hex to str
        ConvertIntToHexStringN_v2(pidhex, gSaveBlock2Ptr->PID,STR_CONV_MODE_RIGHT_ALIGN,8);

        //Add PID to URL
        concat_str(pURL,(char *)pidhex);

        //Add data= to URL
        concat_str(pURL,"&data=\0");

        userprofile->version=VERSION_EMERALD;
        userprofile->romhackID=0x00000013;
        userprofile->romhackVer=SAVE_VERSION;
        userprofile->language=LANGUAGE_ENGLISH;
        userprofile->country=12;
        userprofile->region=8;
        userprofile->trainerID[0]=gSaveBlock2Ptr->playerTrainerId[0];
        userprofile->trainerID[1]=gSaveBlock2Ptr->playerTrainerId[1];
        userprofile->trainerID[2]=gSaveBlock2Ptr->playerTrainerId[2];
        userprofile->trainerID[3]=gSaveBlock2Ptr->playerTrainerId[3];
        for (int i = 0; i < PLAYER_NAME_LENGTH && gSaveBlock2Ptr->playerName[i] != EOS; i++)
        {
            userprofile->trainerName[i]=gSaveBlock2Ptr->playerName[i];
        }
        for (int i = 0; i < 6; i++)
        {
            userprofile->MAC[i]=0;
        }
        for (int i = 0; i < 56; i++)
        {
            userprofile->email[i]=0;
        }
        userprofile->notify=0;
        userprofile->clientSecret=0;
        userprofile->mailSecret=0;


        checksum=encrypt_data(pid, (char *)userprofile, sizeof(userprofile));

        //*encoded_data=base64_encode(checksum, (char *)userprofile, sizeof(userprofile)+4);
        concat_str(pURL,encoded_data);

        recvBufSize=8;

        //Request a PID
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = INTERNET_STATE_EXIT;
        }

        int i = (pRecvData[0] << 24) + (pRecvData[1] << 16) + (pRecvData[3] << 8) + pRecvData[4];

        if(i==0){
            data->state = INTERNET_STATE_MAIN_MENU;
        }
        else{
            data->state = INTERNET_STATE_EXIT;
        }
        break;

        
        
        break;
    case INTERNET_ASK_PID:
        DebugPrintf("INTERNET_ASK_PID");
        input = DoGTSYesNo(&data->textState, &data->var, FALSE, gText_CreateFriendCode);
        switch (input)
        {
        case 0: // Yes
            data->state = INTERNET_CONNECTING;
            data->nextstate = INTERNET_GET_PID;
            break;
        case 1: // No
        case MENU_B_PRESSED:
            data->state = INTERNET_STATE_EXIT;
            break;
        }
        break;
    case INTERNET_CONNECTING:
        DebugPrintf("INTERNET_CONNECTING");
        if (PrintInternetOptionsMenuMessage(&data->textState, gJPText_Connecting))
        {
            data->state = data->nextstate;
        }
        break;
    case INTERNET_GET_PID:
        DebugPrintf("INTERNET_GET_PID");
        concat_str(pURL,"http://www.PutYourDomainHere.com/pokemonrse/common/createprofile?pid=\0");

        //Will be receiving a 32-byte long token
        recvBufSize=32;

        //Player doesn't have a PID yet, use the TID this once
        pid=(gSaveBlock2Ptr->playerTrainerId[0] << 24)+(gSaveBlock2Ptr->playerTrainerId[1] << 16)+(gSaveBlock2Ptr->playerTrainerId[2] << 8)+(gSaveBlock2Ptr->playerTrainerId[3]);
        //Turn hex to str
        ConvertIntToHexStringN_v2(pidhex, pid,STR_CONV_MODE_RIGHT_ALIGN,8);

        //Add PID to URL
        concat_str(pURL,(char *)pidhex);
        
        //Request a PID
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, pUserID, pPassword);
        if(data->errorNum !=0){
            maKill();
            data->state = INTERNET_STATE_EXIT;
        }

        //If pRecvSize<recvBufSize then not all the data that was expected was copied into pRecvData
        if(pRecvSize<recvBufSize){
            tracker=pRecvSize;  //Keep track of the number of bytes copied
            while(tracker<recvBufSize){ //While that is less than the Buffer size
                //Keep calling maUpload until all data copied over
                data->errorNum = maDownload(pURL, NULL, 0, &pRecvData[tracker], recvBufSize-tracker, &pRecvSize, pUserID, pPassword);
                if(data->errorNum !=0){
                    maKill();
                    data->state = INTERNET_STATE_EXIT;
                }
                tracker=tracker+pRecvSize;
            }
        }

        //Add salt prefix to token (using Gen 4 salt)
        memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
        concat_str(halftoken,(char *)pRecvData);
        //Just making sure string is 52 bytes long
        halftoken[52]='\0';

        //Create a hash using SHA-1
        sha1digest(NULL,(u8 *)hash,(u8 *)halftoken,52);

        //Make hash lowercase
        for(tracker=0;tracker<41;tracker++){
            if(hash[tracker]=='\0'){
                break;
            }
            else if(hash[tracker]>0x40){
                hash[tracker]=hash[tracker]+0x20;
            }
        }

        //Add hash to URL
        concat_str(pURL,"&hash=");
        concat_str(pURL,hash);

        //Now we'll be receiving a u32 to use as the PID
        recvBufSize=4;

        //Cleaning up pRecvData
        for(tracker=0;tracker<100;tracker++){
            pRecvData[tracker]='\0';
        }
        tracker=0;

        //Request a PID
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, pUserID, pPassword);
        if(data->errorNum !=0){
            maKill();
            data->state = INTERNET_STATE_EXIT;
        }

        //If pRecvSize<recvBufSize then not all the data that was expected was copied into pRecvData
        if(pRecvSize<recvBufSize){
            tracker=pRecvSize;  //Keep track of the number of bytes copied
            while(tracker<recvBufSize){ //While that is less than the Buffer size
                //Keep calling maUpload until all data copied over
                data->errorNum = maDownload(pURL, NULL, 0, &pRecvData[tracker], recvBufSize-tracker, &pRecvSize, pUserID, pPassword);
                if(data->errorNum !=0){
                    maKill();
                    data->state = INTERNET_STATE_EXIT;
                }
                tracker=tracker+pRecvSize;
            }
        }

        //Get PID from HTTP content
        pid=(pRecvData[0] << 24)+(pRecvData[1] << 16)+(pRecvData[2] << 8)+(pRecvData[3]);
        
        //Add to saveblock then save
        gSaveBlock2Ptr->PID=pid;

        //Show Friend code then save
        data->state = INTERNET_SHOW_FRIENDCODE;
        data->nextstate = INTERNET_SET_PROFILE;
        break;
    case INTERNET_SHOW_FRIENDCODE:
        DebugPrintf("INTERNET_SHOW_FRIENDCODE");
        //Convert PID to FC and display it
        concat_str(pURL,"Your Friend Code is:\n");
        pid_to_fc(gSaveBlock2Ptr->PID,(u8 *)hash);
        concat_str(pURL,hash);
        ASCIIToPkmnStr((u8 *)halftoken,(u8 *)pURL);
        if (PrintInternetOptionsMenuMessage(&data->textState, (u8 *)halftoken))
        {
            data->state = INTERNET_SAVE_GAME;
        }
        break;
    case INTERNET_ASK_COUNTRY:
        data->state = INTERNET_ASK_REGION;
        break;
    case INTERNET_ASK_REGION:
        data->state = INTERNET_INITIAL_SETUP;
        break;
    case INTERNET_INITIAL_SETUP:
        DebugPrintf("INTERNET_INITIAL_SETUP");
        concat_str(pURL,"http://www.PutYourDomainHere.com/pokemonrse/common/setprofile?pid=\0");
        pid = gSaveBlock2Ptr->PID;
        concat_str(pURL,(char *)pid);
        recvBufSize=32;

        //Initial Profile Setup
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = INTERNET_STATE_EXIT;
        }

        //If pRecvSize<recvBufSize then not all the data that was expected was copied into pRecvData
        if(pRecvSize<recvBufSize){
            tracker=pRecvSize;  //Keep track of the number of bytes copied
            while(tracker<recvBufSize){ //While that is less than the Buffer size
                //Keep calling maUpload until
                data->errorNum = maDownload(pURL, NULL, 0, &pRecvData[tracker], recvBufSize-tracker, &pRecvSize, "", "");
                if(data->errorNum !=0){
                    maKill();
                    data->state = INTERNET_STATE_EXIT;
                }
                tracker=tracker+pRecvSize;
            }
        }

        memcpy(halftoken, "sAdeqWo3voLeC5r16DYv\0", 21);
        concat_str(halftoken,(char *)pRecvData);

        sha1digest((u8 *)hash,NULL,(u8 *)halftoken,52);

        //Add hash to URL
        concat_str(pURL,"&hash=");
        concat_str(pURL,hash);

        userprofile->version=VERSION_EMERALD;
        userprofile->romhackID=0x00000013;
        userprofile->romhackVer=SAVE_VERSION;
        userprofile->language=LANGUAGE_ENGLISH;
        userprofile->country=12;
        userprofile->region=8;
        userprofile->trainerID[0]=gSaveBlock2Ptr->playerTrainerId[0];
        userprofile->trainerID[1]=gSaveBlock2Ptr->playerTrainerId[1];
        userprofile->trainerID[2]=gSaveBlock2Ptr->playerTrainerId[2];
        userprofile->trainerID[3]=gSaveBlock2Ptr->playerTrainerId[3];
        for (int i = 0; i < PLAYER_NAME_LENGTH && gSaveBlock2Ptr->playerName[i] != EOS; i++)
        {
            userprofile->trainerName[i]=gSaveBlock2Ptr->playerName[i];
        }
        for (int i = 0; i < 6; i++)
        {
            userprofile->MAC[i]=0;
        }
        for (int i = 0; i < 56; i++)
        {
            userprofile->email[i]=0;
        }
        userprofile->notify=0;
        userprofile->clientSecret=0;
        userprofile->mailSecret=0;


        checksum=encrypt_data(pid, (char *)userprofile, sizeof(userprofile));

        concat_str(pURL,"&data=");
        //*encoded_data=base64_encode(checksum, (char *)userprofile, sizeof(userprofile)+4);
        concat_str(pURL,encoded_data);

        recvBufSize=8;

        //Request a PID
        data->errorNum = maDownload(pURL, NULL, 0, pRecvData, recvBufSize, &pRecvSize, "", "");
        if(data->errorNum !=0){
            maKill();
            data->state = INTERNET_STATE_EXIT;
        }

        //If pRecvSize<recvBufSize then not all the data that was expected was copied into pRecvData
        if(pRecvSize<recvBufSize){
            tracker=pRecvSize;  //Keep track of the number of bytes copied
            while(tracker<recvBufSize){ //While that is less than the Buffer size
                //Keep calling maUpload until
                data->errorNum = maDownload(pURL, NULL, 0, &pRecvData[tracker], recvBufSize-tracker, &pRecvSize, "", "");
                if(data->errorNum !=0){
                    maKill();
                    data->state = INTERNET_STATE_EXIT;
                }
                tracker=tracker+pRecvSize;
            }
        }

        i = (pRecvData[0] << 24) + (pRecvData[1] << 16) + (pRecvData[3] << 8) + pRecvData[4];

        if(i==0){
            data->state = INTERNET_STATE_MAIN_MENU;
        }
        else{
            data->state = INTERNET_STATE_EXIT;
        }
        break;
    case INTERNET_STATE_MAIN_MENU:
        DebugPrintf("INTERNET_STATE_MAIN_MENU");
        // Main Mystery Gift menu, player can select Wonder Cards or News (or exit)
        switch (InternetOptions_HandleThreeOptionMenu(&data->textState, &data->var, 0))
        {
        case 0: // "Mystergy Gift Download"
            data->state = INTERNET_STATE_EXIT;
            PlaySE(SE_SELECT);

            break;
        case 1: // "Bank"
            data->state = INTERNET_STATE_EXIT;
            PlaySE(SE_SELECT);
            break;
        case 2: // "Friends"
            data->state = INTERNET_STATE_EXIT;
            PlaySE(SE_SELECT);

            break;
        case 3: // "Sync"
            data->state = INTERNET_STATE_EXIT;
            PlaySE(SE_SELECT);

            break;
        case LIST_CANCEL:
            data->state = INTERNET_STATE_EXIT;
            break;
        }
        break;
    case INTERNET_SAVE_GAME:
        DebugPrintf("INTERNET_SAVE_GAME");
        if (SaveOnInternetOptionMenu(&data->textState))
            data->state = data->nextstate;
        break;
    case INTERNET_STATE_EXIT:
        DebugPrintf("INTERNET_STATE_EXIT");
        CloseLink();
        Free(data->clientMsg);
        DestroyTask(taskId);
        SetMainCallback2(MainCB_FreeAllBuffersAndReturnToInitTitleScreen);
        break;
    }
}

#undef error
#undef errorType

u16 GetInternetOptionsBaseBlock(void)
{
    return 0x1A9;
}

static void LoadMysteryGiftTextboxBorder(u8 bgId)
{
    DecompressAndLoadBgGfxUsingHeap(bgId, sTextboxBorder_Gfx, 0x100, 0, 0);
}
