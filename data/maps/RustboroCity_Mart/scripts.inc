RustboroCity_Mart_MapScripts:: @ 8214F05
	.byte 0

RustboroCity_Mart_EventScript_Clerk:: @ 8214F06
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

RustboroCity_Mart_EventScript_Clerk2:: @ 8214F48
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart RustboroCity_Mart_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
RustboroCity_Mart_Pokemart: @ 8214F58
	.2byte ITEM_REPEAT_BALL
	.2byte ITEM_TIMER_BALL
	.2byte ITEM_NET_BALL
	.2byte ITEM_NONE
	release
	end

RustboroCity_Mart_EventScript_PokefanF:: @ 8214F74
	msgbox RustboroCity_Mart_Text_BuyingHealsInCaseOfShroomish, MSGBOX_NPC
	end

RustboroCity_Mart_EventScript_Boy:: @ 8214F7D
	msgbox RustboroCity_Mart_Text_ShouldBuySuperPotionsInstead, MSGBOX_NPC
	end

RustboroCity_Mart_EventScript_BugCatcher:: @ 8214F86
	msgbox RustboroCity_Mart_Text_GettingEscapeRopeJustInCase, MSGBOX_NPC
	end

RustboroCity_Mart_Text_BuyingHealsInCaseOfShroomish: @ 8214F8F
	.string "I'm buying some Parlyz Heals and\n"
	.string "Antidotes.\p"
	.string "Just in case I run into Shroomish\n"
	.string "in Petalburg Woods.$"

RustboroCity_Mart_Text_ShouldBuySuperPotionsInstead: @ 8214FF1
	.string "My Pokémon evolved.\n"
	.string "It has a lot of HP now.\p"
	.string "I should buy Super Potions for it\n"
	.string "instead of ordinary Potions.$"

RustboroCity_Mart_Text_GettingEscapeRopeJustInCase: @ 821505C
	.string "I'm getting an Escape Rope just in\n"
	.string "case I get lost in a cave.\p"
	.string "I just need to use it to get back to\n"
	.string "the entrance.$"

