OldaleTown_Mart_MapScripts:: @ 81FC23F
	.byte 0

OldaleTown_Mart_EventScript_1FC240:: @ 81FC240
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

OldaleTown_Mart_EventScript_1FC28A:: @ 81FC28A
	lock
	faceplayer
	goto_if_set FLAG_ADVENTURE_STARTED, OldaleTown_Mart_EventScript_1FC29F
	msgbox OldaleTown_Mart_Text_1FC2B2, MSGBOX_DEFAULT
	release
	end

OldaleTown_Mart_EventScript_1FC29F:: @ 81FC29F
	msgbox OldaleTown_Mart_Text_1FC2F3, MSGBOX_DEFAULT
	release
	end

OldaleTown_Mart_EventScript_1FC2A9:: @ 81FC2A9
	msgbox OldaleTown_Mart_Text_1FC338, MSGBOX_NPC
	end

OldaleTown_Mart_Text_1FC2B2: @ 81FC2B2
	.string "The clerk says they're all sold out.\n"
	.string "I can't buy any Poké Balls.$"

OldaleTown_Mart_Text_1FC2F3: @ 81FC2F3
	.string "I'm going to buy a bunch of Poké Balls\n"
	.string "and catch a bunch of Pokémon!$"

OldaleTown_Mart_Text_1FC338: @ 81FC338
	.string "If a Pokémon gets hurt and loses its HP\n"
	.string "and faints, it won't be able to battle.\p"
	.string "To prevent your Pokémon from fainting,\n"
	.string "restore its HP with a Potion.$"

