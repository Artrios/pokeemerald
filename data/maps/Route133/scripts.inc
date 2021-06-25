Route133_MapScripts:: @ 81F75A2
	.byte 0

Route133_EventScript_Franklin:: @ 81F75A3
	trainerbattle_single TRAINER_FRANKLIN, Route133_Text_FranklinIntro, Route133_Text_FranklinDefeat
	msgbox Route133_Text_FranklinPostBattle, MSGBOX_AUTOCLOSE
	end

Route133_EventScript_Debra:: @ 81F75BA
	trainerbattle_single TRAINER_DEBRA, Route133_Text_DebraIntro, Route133_Text_DebraDefeat
	msgbox Route133_Text_DebraPostBattle, MSGBOX_AUTOCLOSE
	end

Route133_EventScript_Linda:: @ 81F75D1
	trainerbattle_single TRAINER_LINDA, Route133_Text_LindaIntro, Route133_Text_LindaDefeat
	msgbox Route133_Text_LindaPostBattle, MSGBOX_AUTOCLOSE
	end

Route133_EventScript_Warren:: @ 81F75E8
	trainerbattle_single TRAINER_WARREN, Route133_Text_WarrenIntro, Route133_Text_WarrenDefeat
	msgbox Route133_Text_WarrenPostBattle, MSGBOX_AUTOCLOSE
	end

Route133_EventScript_Beck:: @ 81F75FF
	trainerbattle_single TRAINER_BECK, Route133_Text_BeckIntro, Route133_Text_BeckDefeat
	msgbox Route133_Text_BeckPostBattle, MSGBOX_AUTOCLOSE
	end

Route133_EventScript_Mollie:: @ 81F7616
	trainerbattle_single TRAINER_MOLLIE, Route133_Text_MollieIntro, Route133_Text_MollieDefeat
	msgbox Route133_Text_MolliePostBattle, MSGBOX_AUTOCLOSE
	end

Route133_EventScript_Conor:: @ 81F762D
	trainerbattle_single TRAINER_CONOR, Route133_Text_ConorIntro, Route133_Text_ConorDefeat
	msgbox Route133_Text_ConorPostBattle, MSGBOX_AUTOCLOSE
	end

Route133_EventScript_Psychic:: @ 8211125
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_TM77, Route133_ReceivedPsychUp
	msgbox Route133_Text_Hello, MSGBOX_DEFAULT
	giveitem ITEM_TM77
	compare VAR_RESULT, FALSE
	goto_if_eq Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_TM77
	goto Route133_ReceivedPsychUp
	release
	end

Route133_ReceivedPsychUp::
	msgbox Route133_Text_Phew, MSGBOX_DEFAULT
	release
	end

Route133_Text_Hello:
	.string "Why, hello, hellooo, little visiting\n"
	.string "Trainer! Greeting to you out here in\l"
	.string "the big blue!\p"
	.string "Now, I know you know that a Psychic\n"
	.string "Trainer like me can see riiiiiight\l"
	.string "through any foe.\p"
	.string "I can tell exactly how strong you\n"
	.string "are and how a battle between us\l"
	.string "would go...\l"
	.string "just by lookin' at you! It's eerie,\l"
	.string "right?\p"
	.string "Now stand back and watch as I\n"
	.string "battle you within my mind!\p"
	.string "...\p"
	.string "Yowza! You did me right in!\n"
	.string "Take this, and have mercy!$"

Route133_Text_Phew:
	.string "Phew... That was bad. So eerie.\p"
	.string "I couldn't even get a hit on you!\n"
	.string "Seriously, tell me: What are you?$"