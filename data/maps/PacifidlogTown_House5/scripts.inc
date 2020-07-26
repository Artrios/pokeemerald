PacifidlogTown_House5_MapScripts:: @ 8204110
	.byte 0

PacifidlogTown_House5_EventScript_MirageIslandWatcher:: @ 8204111
	lock
	faceplayer
	specialvar VAR_RESULT, IsMirageIslandPresent
	compare VAR_RESULT, TRUE
	goto_if_eq PacifidlogTown_House5_EventScript_MirageIslandPresent
	msgbox PacifidlogTown_House5_Text_CantSeeMirageIslandToday, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House5_EventScript_MirageIslandPresent:: @ 820412D
	msgbox PacifidlogTown_House5_Text_CanSeeMirageIslandToday, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House5_EventScript_Gentleman:: @ 8204137
	msgbox PacifidlogTown_House5_Text_MirageIslandAppearDependingOnWeather, MSGBOX_NPC
	goto_if_unset FLAG_RECEIVED_TM03, PacifidlogTown_House5_EventScript_GiveTM03
	end

PacifidlogTown_House5_EventScript_GiveTM03::
	giveitem ITEM_TM03
	setflag FLAG_RECEIVED_TM03
	end

PacifidlogTown_House5_Text_CantSeeMirageIslandToday: @ 8204140
	.string "I can't see Mirage Island today…$"

PacifidlogTown_House5_Text_CanSeeMirageIslandToday: @ 8204161
	.string "Oh! Oh my!\n"
	.string "I can see Mirage Island today!$"

PacifidlogTown_House5_Text_MirageIslandAppearDependingOnWeather: @ 820418B
	.string "I've been to a Mirage spot...\n"
	.string "I'm sure of it. But I forgot where\l"
	.string "it is...\p"
	.string "I used to be very good at\n"
	.string "remembering things...\p"
	.string "I'm so shocked that I can't\n"
	.string "remember...$"
