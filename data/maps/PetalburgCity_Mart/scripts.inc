PetalburgCity_Mart_MapScripts:: @ 8207D68
	.byte 0

PetalburgCity_Mart_EventScript_207D69:: @ 8207D69
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

PetalburgCity_Mart_EventScript_207DD6:: @ 8207DD6
	msgbox PetalburgCity_Mart_Text_207DF1, MSGBOX_NPC
	end

PetalburgCity_Mart_EventScript_207DDF:: @ 8207DDF
	msgbox PetalburgCity_Mart_Text_207E60, MSGBOX_NPC
	end

PetalburgCity_Mart_EventScript_207DE8:: @ 8207DE8
	msgbox PetalburgCity_Mart_Text_207EB0, MSGBOX_NPC
	end

PetalburgCity_Mart_Text_207DF1: @ 8207DF1
	.string "Even if a Pokémon is weak now,\n"
	.string "it will grow stronger.\p"
	.string "The most important thing is love!\n"
	.string "Love for your Pokémon!$"

PetalburgCity_Mart_Text_207E60: @ 8207E60
	.string "Do you use Repel?\n"
	.string "It keeps Pokémon away, so it's\l"
	.string "useful when you're in a hurry.$"

PetalburgCity_Mart_Text_207EB0: @ 8207EB0
	.string "Do you have any Antidotes with\n"
	.string "you?\p"
	.string "If you walk around with a poisoned\n"
	.string "Pokémon, it will lose HP until it faints.\l"
	.string "Take some Antidotes with you.$"

