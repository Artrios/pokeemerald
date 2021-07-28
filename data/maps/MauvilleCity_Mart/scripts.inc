MauvilleCity_Mart_MapScripts:: @ 82110E5
	.byte 0

MauvilleCity_Mart_EventScript_Clerk:: 
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	goto_if_set FLAG_DEFEATED_PETALBURG_GYM, Pokemart_5_Gym_Setup
	goto_if_set FLAG_DEFEATED_LAVARIDGE_GYM, Pokemart_4_Gym_Setup
	goto_if_set FLAG_DEFEATED_MAUVILLE_GYM, Pokemart_3_Gym_Setup
	goto_if_set FLAG_DEFEATED_DEWFORD_GYM, Pokemart_2_Gym_Setup
	goto_if_set FLAG_DEFEATED_RUSTBORO_GYM, Pokemart_1_Gym_Setup
	goto_if_set FLAG_ADVENTURE_STARTED, Pokemart_Have_Pokedex_Setup
	pokemart Pokemart_No_Pokedex
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

MauvilleCity_Mart_EventScript_Clerk2:: 
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart MauvilleCity_Mart_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
MauvilleCity_Mart_Pokemart: @ 8211100
	.2byte ITEM_TM09_VENOSHOCK
	.2byte ITEM_TM40_AERIAL_ACE
	.2byte ITEM_TM42_FACADE
	.2byte ITEM_TM47_LOW_SWEEP
	.2byte ITEM_TM48_ROUND
	.2byte ITEM_TM57_CHARGE_BEAM
	.2byte ITEM_TM78_BULLDOZE
	.2byte ITEM_TM82_DRAGON_TAIL
	.2byte ITEM_TM89_U_TURN
	.2byte ITEM_TM96_NATURE_POWER
	.2byte ITEM_TM98_POWER_UP_PUNCH
	.2byte ITEM_NONE
	release
	end

MauvilleCity_Mart_EventScript_ExpertM:: @ 821111C
	msgbox MauvilleCity_Mart_Text_ItemsToTemporarilyElevateStats, MSGBOX_NPC
	end

MauvilleCity_Mart_EventScript_Man:: @ 8211125
	msgbox MauvilleCity_Mart_Text_DecisionsDetermineBattle, MSGBOX_NPC
	end

MauvilleCity_Mart_EventScript_Woman:: @ 8211125
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_TM58, MauvilleCity_Mart_ReceivedSkyDrop
	msgbox MauvilleCity_Mart_Text_DoYouLikeScaryRides, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq MauvilleCity_Mart_EventScript_Yes
	compare VAR_RESULT, NO
	goto_if_eq MauvilleCity_Mart_EventScript_No
	end

MauvilleCity_Mart_EventScript_Yes::
	msgbox MauvilleCity_Mart_Text_YouCool, MSGBOX_DEFAULT
	giveitem ITEM_TM58
	compare VAR_RESULT, FALSE
	goto_if_eq Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_TM58
	goto MauvilleCity_Mart_ReceivedSkyDrop
	release
	end

MauvilleCity_Mart_ReceivedSkyDrop::
	msgbox MauvilleCity_Mart_Text_SkyDropIsSick, MSGBOX_DEFAULT
	release
	end

MauvilleCity_Mart_EventScript_No::
	msgbox MauvilleCity_Mart_Text_Lame, MSGBOX_DEFAULT
	release
	end

MauvilleCity_Mart_Text_ItemsToTemporarilyElevateStats: @ 821112E
	.string "There are items that temporarily\n"
	.string "elevate the stats of Pokémon.\p"
	.string "The ones I know you use in battle\n"
	.string "are X Attack and X Defend…\p"
	.string "I do believe that there are others\n"
	.string "like them.$"

MauvilleCity_Mart_Text_DecisionsDetermineBattle: @ 82111D8
	.string "Use a certain move, or use an item\n"
	.string "instead…\p"
	.string "The Trainer's decisions determine how\n"
	.string "battles turn out, I think.$"

MauvilleCity_Mart_Text_DoYouLikeScaryRides: @ 8204035
	.string "Hey, you. Do you like scary rides,\n"
	.string "you know, like a roller coaster?$"

MauvilleCity_Mart_Text_YouCool:
	.string "...Hey, you're cool.\n"
	.string "Like, you are totally my BFF.\p"
	.string "As a proof, or as a token, you know,\n"
	.string "I'm giving you my favourite TM.$"

MauvilleCity_Mart_Text_SkyDropIsSick:
	.string "Sky Drop is seriously sick.\n"
	.string "It totally messes with the inner ear!$"

MauvilleCity_Mart_Text_Lame:
	.string "...Tsk."
	.string "You're lame."
