SlateportCity_Mart_MapScripts:: @ 820DC48
	.byte 0

SlateportCity_Mart_EventScript_20DC49::
	lock
	faceplayer
	message gUnknown_08272A21
	waitmessage
	goto_if_set FLAG_DEFEATED_PETALBURG_GYM, Pokemart_5_Gym_Setup
	goto_if_set FLAG_DEFEATED_LAVARIDGE_GYM, Pokemart_4_Gym_Setup
	goto_if_set FLAG_DEFEATED_MAUVILLE_GYM, Pokemart_3_Gym_Setup
	goto_if_set FLAG_DEFEATED_DEWFORD_GYM, Pokemart_2_Gym_Setup
	goto_if_set FLAG_DEFEATED_RUSTBORO_GYM, Pokemart_1_Gym_Setup
	goto_if_set FLAG_ADVENTURE_STARTED, Pokemart_Have_Pokedex_Setup
	pokemart Pokemart_No_Pokedex
	msgbox gUnknown_08272A3F, MSGBOX_DEFAULT
	release
	end

SlateportCity_Mart_EventScript_2::
	lock
	faceplayer
	message gUnknown_08272A21
	waitmessage
	pokemart SlateportCity_Mart_Pokemart_20DC60
	msgbox gUnknown_08272A3F, MSGBOX_DEFAULT
	release
	end

SlateportCity_Mart_Pokemart_20DC60:
	.2byte ITEM_X_SPEED
	.2byte ITEM_X_ATTACK
	.2byte ITEM_X_DEFEND
	.2byte ITEM_GUARD_SPEC
	.2byte ITEM_DIRE_HIT
	.2byte ITEM_X_ACCURACY
	.2byte ITEM_X_SPECIAL
	.2byte ITEM_NONE
	release
	end

SlateportCity_Mart_EventScript_20DC76:: @ 820DC76
	msgbox SlateportCity_Mart_Text_20DC88, MSGBOX_NPC
	end

SlateportCity_Mart_EventScript_20DC7F:: @ 820DC7F
	msgbox SlateportCity_Mart_Text_20DCF8, MSGBOX_NPC
	end

SlateportCity_Mart_Text_20DC88: @ 820DC88
	.string "The Market does have some interesting\n"
	.string "merchandise.\p"
	.string "But there are some items you can only\n"
	.string "get at a Pokémon Mart.$"

SlateportCity_Mart_Text_20DCF8: @ 820DCF8
	.string "A Great Ball is better than a Poké Ball\n"
	.string "at catching Pokémon.\p"
	.string "With this, I should be able to get that\n"
	.string "elusive Pokémon…$"

