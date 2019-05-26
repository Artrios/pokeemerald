SootopolisCity_Mart_MapScripts:: @ 8226794
	.byte 0

SootopolisCity_Mart_EventScript_226795::
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

SootopolisCity_Mart_EventScript_2267C2:: @ 82267C2
	lock
	faceplayer
	compare VAR_SKY_PILLAR_STATE, 2
	goto_if_ge SootopolisCity_Mart_EventScript_2267E2
	goto_if_unset FLAG_KYOGRE_ESCAPED_SEAFLOOR_CAVERN, SootopolisCity_Mart_EventScript_2267E2
	msgbox SootopolisCity_Mart_Text_22685D, MSGBOX_DEFAULT
	release
	end

SootopolisCity_Mart_EventScript_2267E2:: @ 82267E2
	msgbox SootopolisCity_Mart_Text_226816, MSGBOX_DEFAULT
	release
	end

SootopolisCity_Mart_EventScript_2267EC:: @ 82267EC
	lock
	faceplayer
	compare VAR_SKY_PILLAR_STATE, 2
	goto_if_ge SootopolisCity_Mart_EventScript_22680C
	goto_if_unset FLAG_KYOGRE_ESCAPED_SEAFLOOR_CAVERN, SootopolisCity_Mart_EventScript_22680C
	msgbox SootopolisCity_Mart_Text_226928, MSGBOX_DEFAULT
	release
	end

SootopolisCity_Mart_EventScript_22680C:: @ 822680C
	msgbox SootopolisCity_Mart_Text_2268AF, MSGBOX_DEFAULT
	release
	end

SootopolisCity_Mart_Text_226816: @ 8226816
	.string "PP UP is great!\p"
	.string "It raises the POWER POINTS, the PP,\n"
	.string "of a POKéMON move.$"

SootopolisCity_Mart_Text_22685D: @ 822685D
	.string "What…\n"
	.string "What is happening?\p"
	.string "I really want to know, but it's too\n"
	.string "scary to go outside.$"

SootopolisCity_Mart_Text_2268AF: @ 82268AF
	.string "Do you know FULL RESTORE?\p"
	.string "Full restoration of HP!\n"
	.string "Eradication of all status problems!\p"
	.string "It's truly an item of your dreams!$"

SootopolisCity_Mart_Text_226928: @ 8226928
	.string "This weather…\n"
	.string "Did something awaken?$"

