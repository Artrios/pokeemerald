LavaridgeTown_Mart_MapScripts:: @ 81FF9CD
	.byte 0

LavaridgeTown_Mart_EventScript_1FF9CE:: @ 81FF9CE
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

LavaridgeTown_Mart_EventScript_1FF9FE:: @ 81FF9FE
	msgbox LavaridgeTown_Mart_Text_1FFA10, MSGBOX_NPC
	end

LavaridgeTown_Mart_EventScript_1FFA07:: @ 81FFA07
	msgbox LavaridgeTown_Mart_Text_1FFA83, MSGBOX_NPC
	end

LavaridgeTown_Mart_Text_1FFA10: @ 81FFA10
	.string "Use X Speed to add to a Pokémon's\n"
	.string "Speed in battle.\p"
	.string "That will help it get in the first\n"
	.string "strike--a decided advantage!$"

LavaridgeTown_Mart_Text_1FFA83: @ 81FFA83
	.string "On Mt. Chimney's peak, there's a local\n"
	.string "specialty that you can buy only there.\p"
	.string "Give it to a Pokémon--it will be elated.$"

