FortreeCity_Mart_MapScripts:: @ 8217665
	.byte 0

FortreeCity_Mart_EventScript_217666:: @ 8217666
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

FortreeCity_Mart_EventScript_217698:: @ 8217698
	msgbox FortreeCity_Mart_Text_2176B3, MSGBOX_NPC
	end

FortreeCity_Mart_EventScript_2176A1:: @ 82176A1
	msgbox FortreeCity_Mart_Text_217715, MSGBOX_NPC
	end

FortreeCity_Mart_EventScript_2176AA:: @ 82176AA
	msgbox FortreeCity_Mart_Text_21778E, MSGBOX_NPC
	end

FortreeCity_Mart_Text_2176B3: @ 82176B3
	.string "SUPER REPEL lasts a long time,\n"
	.string "and it gets the job done.\p"
	.string "It's much better than an ordinary\n"
	.string "REPEL.$"

FortreeCity_Mart_Text_217715: @ 8217715
	.string "I always stock up on more items than\n"
	.string "I'm sure I'll need.\p"
	.string "You never know what might happen.\n"
	.string "Better to be safe than sorry!$"

FortreeCity_Mart_Text_21778E: @ 821778E
	.string "A RARE CANDY makes a POKéMON grow\n"
	.string "immediately by one level.$"

