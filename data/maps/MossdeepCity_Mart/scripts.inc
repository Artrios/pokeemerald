MossdeepCity_Mart_MapScripts:: @ 82223C7
	.byte 0

MossdeepCity_Mart_EventScript_2223C8:: @ 82223C8
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

MossdeepCity_Mart_EventScript_2223F6:: @ 82223F6
	msgbox MossdeepCity_Mart_Text_222411, MSGBOX_NPC
	end

MossdeepCity_Mart_EventScript_2223FF:: @ 82223FF
	msgbox MossdeepCity_Mart_Text_2224A0, MSGBOX_NPC
	end

MossdeepCity_Mart_EventScript_222408:: @ 8222408
	msgbox MossdeepCity_Mart_Text_2224FA, MSGBOX_NPC
	end

MossdeepCity_Mart_Text_222411: @ 8222411
	.string "Revive is fantastic!\p"
	.string "Give it to a fainted Pokémon,\n"
	.string "and the Pokémon will arise.\p"
	.string "But be careful, Revive doesn't restore\n"
	.string "the used-up PP of moves.$"

MossdeepCity_Mart_Text_2224A0: @ 82224A0
	.string "Max Repel keeps all weak Pokémon away.\p"
	.string "Out of all the Repel sprays, it lasts\n"
	.string "the longest.$"

MossdeepCity_Mart_Text_2224FA: @ 82224FA
	.string "The Net and Dive Balls are rare Poké\n"
	.string "Balls that are only made in Mossdeep.\p"
	.string "A Net Ball is effective against\n"
	.string "Bug-type and Water-type Pokémon.\p"
	.string "A Dive Ball works best on Pokémon\n"
	.string "at the bottom of the sea.$"

