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
	.2byte ITEM_TM40_AERIAL_ACE
	.2byte ITEM_TM42_FACADE
	.2byte ITEM_NONE
	release
	end

MauvilleCity_Mart_EventScript_ExpertM:: @ 821111C
	msgbox MauvilleCity_Mart_Text_ItemsToTemporarilyElevateStats, MSGBOX_NPC
	end

MauvilleCity_Mart_EventScript_Man:: @ 8211125
	msgbox MauvilleCity_Mart_Text_DecisionsDetermineBattle, MSGBOX_NPC
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

