VerdanturfTown_Mart_MapScripts:: @ 8202586
	.byte 0

VerdanturfTown_Mart_EventScript_202587::
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

VerdanturfTown_Mart_EventScript_2::
	lock
	faceplayer
	message gUnknown_08272A21
	waitmessage
	pokemart VerdanturfTown_Mart_Pokemart_2
	msgbox gUnknown_08272A3F, MSGBOX_DEFAULT
	release
	end

VerdanturfTown_Mart_Pokemart_2:
	.2byte ITEM_X_SPEED
	.2byte ITEM_X_ATTACK
	.2byte ITEM_X_DEFENSE
	.2byte ITEM_GUARD_SPEC
	.2byte ITEM_DIRE_HIT
	.2byte ITEM_X_ACCURACY
	.2byte ITEM_X_SPECIAL
	.2byte ITEM_NONE
	release
	end

VerdanturfTown_Mart_EventScript_2025BA:: @ 82025BA
	msgbox VerdanturfTown_Mart_Text_2025D5, MSGBOX_NPC
	end

VerdanturfTown_Mart_EventScript_2025C3:: @ 82025C3
	msgbox VerdanturfTown_Mart_Text_20264C, MSGBOX_NPC
	end

VerdanturfTown_Mart_EventScript_2025CC:: @ 82025CC
	msgbox VerdanturfTown_Mart_Text_2026C9, MSGBOX_NPC
	end

VerdanturfTown_Mart_Text_2025D5: @ 82025D5
	.string "For any POKéMON match, X SPECIAL\n"
	.string "is crucial.\p"
	.string "It jacks up the power of some moves\n"
	.string "even though it's only for one battle.$"

VerdanturfTown_Mart_Text_20264C: @ 820264C
	.string "They don't seem to sell any winning\n"
	.string "strategy guides for the BATTLE TENT…\p"
	.string "It seems one must rely on one's\n"
	.string "own wits after all…$"

VerdanturfTown_Mart_Text_2026C9: @ 82026C9
	.string "The NEST BALL works better on\n"
	.string "weakened POKéMON.\p"
	.string "VERDANTURF is the only place you can\n"
	.string "buy it.$"

