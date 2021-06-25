PacifidlogTown_House4_MapScripts:: @ 8203F4B
	.byte 0

PacifidlogTown_House4_EventScript_LittleGirl:: @ 8203F4C
	msgbox PacifidlogTown_House4_Text_SkyPokemon, MSGBOX_NPC
	end

PacifidlogTown_House4_EventScript_Woman:: @ 8203F55
	msgbox PacifidlogTown_House4_Text_PeopleSawHighFlyingPokemon, MSGBOX_NPC
	end

PacifidlogTown_House4_EventScript_Boy:: @ 8203F5E
	lock
	faceplayer
	msgbox PacifidlogTown_House4_Text_WhereDidYouComeFrom, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq PacifidlogTown_House4_EventScript_Yes
	compare VAR_RESULT, NO
	goto_if_eq PacifidlogTown_House4_EventScript_No
	end

PacifidlogTown_House4_EventScript_Yes:: @ 8203F7F
	msgbox PacifidlogTown_House4_Text_YesTown, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House4_EventScript_No:: @ 8203F89
	msgbox PacifidlogTown_House4_Text_YouHaveToComeFromSomewhere, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House4_EventScript_Man:: @ 8211125
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_TM56, PacifidlogTown_House4_ReceivedFling
	msgbox PacifidlogTown_Text_DontWant, MSGBOX_DEFAULT
	giveitem ITEM_TM56
	compare VAR_RESULT, FALSE
	goto_if_eq Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_TM56
	goto PacifidlogTown_House4_ReceivedFling
	release
	end

PacifidlogTown_House4_ReceivedFling::
	msgbox LavaridgeTown_Text_Unova, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House4_Text_PeopleSawHighFlyingPokemon: @ 8203F93
	.string "People were saying they saw a Pokémon\n"
	.string "flying high above Hoenn.\p"
	.string "Is it flying around all the time?\n"
	.string "Doesn't it need to rest somewhere?$"

PacifidlogTown_House4_Text_SkyPokemon: @ 8204017
	.string "A sky Pokémon!\n"
	.string "A sky Pokémon!$"

PacifidlogTown_House4_Text_WhereDidYouComeFrom: @ 8204035
	.string "Where did you come from?$"

PacifidlogTown_House4_Text_YesTown: @ 820404E
	.string "Yes?\n"
	.string "Yes Town?\p"
	.string "I've never heard of a place like that.$"

PacifidlogTown_House4_Text_YouHaveToComeFromSomewhere: @ 8204084
	.string "No? That doesn't make any sense.\n"
	.string "You have to come from somewhere.\p"
	.string "Oh! Wait! You're not going to say you\n"
	.string "were born at the bottom of the sea?$"

PacifidlogTown_Text_DontWant:
	.string "...Would you take this from me?\n"
	.string "I no longer have any use for it.$"

PacifidlogTown_Text_Unova:
	.string "That Big Stadium over in the Unova\n"
	.string "region...\l"
	.string "I always dreamed of standin on the\l"
	.string "mound there.\p"
	.string "But no matter how much I hoped and\n"
	.string "struggled, my abilities couldn't\l"
	.string "keep up with my ambitions.\p"
	.string "My dream wouldn't die, but before I\n"
	.string "knew it, I was a family man in my 50s.\p"
	.string "Not just for me, but for my family,\n"
	.string "too, it's time for me to wake up from\l"
	.string "my dream.$"