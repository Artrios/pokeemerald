MossdeepCity_Mart_MapScripts:: @ 82223C7
	.byte 0

MossdeepCity_Mart_EventScript_Clerk:: @ 82223C8
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

MossdeepCity_Mart_EventScript_Woman:: @ 82223F6
	msgbox MossdeepCity_Mart_Text_ReviveIsFantastic, MSGBOX_NPC
	end

MossdeepCity_Mart_EventScript_Boy:: @ 82223FF
	msgbox MossdeepCity_Mart_Text_MaxRepelLastsLongest, MSGBOX_NPC
	end

MossdeepCity_Mart_EventScript_Sailor:: @ 8222408
	msgbox MossdeepCity_Mart_Text_NetAndDiveBallsRare, MSGBOX_NPC
	end

MossdeepCity_Mart_EventScript_WomanTM60:: @ 8211125
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_TM60, MossdeepCity_Mart_ReceivedQuash
	msgbox MossdeepCity_Mart_Text_WantATM, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq MossdeepCity_Mart_EventScript_Yes
	compare VAR_RESULT, NO
	goto_if_eq MossdeepCity_Mart_EventScript_No
	end

MossdeepCity_Mart_EventScript_Yes::
	giveitem ITEM_TM60
	compare VAR_RESULT, FALSE
	goto_if_eq Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_TM60
	goto MossdeepCity_Mart_ReceivedQuash
	release
	end

MauvilleCity_Mart_ReceivedQuash::
	msgbox MossdeepCity_Mart_Text_Sweetie, MSGBOX_DEFAULT
	release
	end

MossdeepCity_Mart_EventScript_No::
	msgbox MauvilleCity_Mart_Text_ISee, MSGBOX_DEFAULT
	release
	end

MossdeepCity_Mart_Text_ReviveIsFantastic: @ 8222411
	.string "Revive is fantastic!\p"
	.string "Give it to a fainted Pokémon,\n"
	.string "and the Pokémon will arise.\p"
	.string "But be careful, Revive doesn't restore\n"
	.string "the used-up PP of moves.$"

MossdeepCity_Mart_Text_MaxRepelLastsLongest: @ 82224A0
	.string "Max Repel keeps all weak Pokémon away.\p"
	.string "Out of all the Repel sprays, it lasts\n"
	.string "the longest.$"

MossdeepCity_Mart_Text_NetAndDiveBallsRare: @ 82224FA
	.string "The Net and Dive Balls are rare Poké\n"
	.string "Balls that are only made in Mossdeep.\p"
	.string "A Net Ball is effective against\n"
	.string "Bug-type and Water-type Pokémon.\p"
	.string "A Dive Ball works best on Pokémon\n"
	.string "at the bottom of the sea.$"

MossdeepCity_Mart_Text_WantATM:
	.string "Hey, you!\n"
	.string "Quick question! Do you want a TM?$"

MossdeepCity_Mart_Text_Sweetie:
	.string "That's the TM that my sweetie\n"
	.string "gave me when I proposed.\p"
	.string "But I'm not a Trainer, so I don't\n"
	.string "know how to use TMs. Hahaha!$"

MauvilleCity_Mart_Text_ISee:
	.string "I see.\n"
	.string "If you want to get it, let me know.$"
