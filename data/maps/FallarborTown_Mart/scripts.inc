FallarborTown_Mart_MapScripts:: @ 81FFCBE
	.byte 0

FallarborTown_Mart_EventScript_Clerk:: @ 81FFCBF
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

FallarborTown_Mart_EventScript_Clerk2:: 
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart FallarborTown_Mart_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
FallarborTown_Mart_Pokemart: @ 81FFCD8
	.2byte ITEM_QUICK_BALL
	.2byte ITEM_DUSK_BALL
	.2byte ITEM_DIVE_BALL
	.2byte ITEM_NONE
	release
	end

FallarborTown_Mart_EventScript_Woman:: @ 81FFCF4
	msgbox FallarborTown_Mart_Text_DecidingSkittyEvolve, MSGBOX_NPC
	end

FallarborTown_Mart_EventScript_PokefanM:: @ 81FFCFD
	msgbox FallarborTown_Mart_Text_SellNuggetIFound, MSGBOX_NPC
	end

FallarborTown_Mart_EventScript_Skitty:: @ 81FFD06
	lock
	faceplayer
	waitse
	playmoncry SPECIES_SKITTY, 0
	msgbox FallarborTown_Mart_Text_Skitty, MSGBOX_DEFAULT
	waitmoncry
	release
	end

FallarborTown_Mart_Text_DecidingSkittyEvolve: @ 81FFD19
	.string "I'm having a hard time deciding if I\n"
	.string "should make my Skitty evolve or not.\p"
	.string "I only have to use this Moon Stone,\n"
	.string "but it's so hard to decide…\p"
	.string "If I make it evolve, it will become\n"
	.string "much stronger.\p"
	.string "But it will look so different, too.$"

FallarborTown_Mart_Text_Skitty: @ 81FFDFA
	.string "Skitty: Miyao?$"

FallarborTown_Mart_Text_SellNuggetIFound: @ 81FFE09
	.string "This Nugget I found here…\n"
	.string "I suppose I'll have to sell it, seeing\l"
	.string "as how it has no other use.$"

