SouthernIsland_Interior_MapScripts:: @ 8242A45
	map_script MAP_SCRIPT_ON_RESUME, SouthernIsland_Interior_OnResume
	map_script MAP_SCRIPT_ON_TRANSITION, SouthernIsland_Interior_OnTransition
	.byte 0

SouthernIsland_Interior_OnResume: @ 8242A50
	call_if_set FLAG_SYS_CTRL_OBJ_DELETE, SouthernIsland_Interior_EventScript_TryRemoveLati
	end

SouthernIsland_Interior_EventScript_TryRemoveLati:: @ 8242A5A
	specialvar VAR_RESULT, GetBattleOutcome
	compare VAR_RESULT, B_OUTCOME_CAUGHT
	goto_if_ne Common_EventScript_NopReturn
	removeobject 2
	return

SouthernIsland_Interior_OnTransition: @ 8242A6E
	compare VAR_ROAMER_POKEMON, 0
	call_if_eq SouthernIsland_Interior_EventScript_SetUpLatios
	compare VAR_ROAMER_POKEMON, 0
	call_if_ne SouthernIsland_Interior_EventScript_SetUpLatias
	call SouthernIsland_Interior_EventScript_SetUpPlayerGfx
	end

SouthernIsland_Interior_EventScript_SetUpLatios:: @ 8242A8A
	setvar VAR_OBJ_GFX_ID_1, OBJ_EVENT_GFX_LATIOS
	setvar VAR_TEMP_4, SPECIES_LATIOS
	return

SouthernIsland_Interior_EventScript_SetUpLatias:: @ 8242A95
	setvar VAR_OBJ_GFX_ID_1, OBJ_EVENT_GFX_LATIAS
	setvar VAR_TEMP_4, SPECIES_LATIAS
	return

SouthernIsland_Interior_EventScript_SetUpPlayerGfx:: @ 8242AA0
	checkplayergender
	compare VAR_RESULT, MALE
	goto_if_eq SouthernIsland_Interior_EventScript_SetBrendanGfx
	compare VAR_RESULT, FEMALE
	goto_if_eq SouthernIsland_Interior_EventScript_SetMayGfx
	end

SouthernIsland_Interior_EventScript_SetBrendanGfx:: @ 8242AB8
	setvar VAR_OBJ_GFX_ID_0, OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL
	return

SouthernIsland_Interior_EventScript_SetMayGfx:: @ 8242ABE
	setvar VAR_OBJ_GFX_ID_0, OBJ_EVENT_GFX_RIVAL_MAY_NORMAL
	return

SouthernIsland_Interior_EventScript_TryLatiEncounter:: @ 8242AC4
	lockall
	setvar VAR_0x8008, 12
	goto SouthernIsland_Interior_EventScript_Lati
	end

SouthernIsland_Interior_EventScript_Lati:: @ 8242AD0
	goto_if_set FLAG_TEMP_2, SouthernIsland_Interior_EventScript_Sign
	goto_if_set FLAG_DEFEATED_LATIAS_OR_LATIOS, SouthernIsland_Interior_EventScript_Sign
	goto_if_set FLAG_CAUGHT_LATIAS_OR_LATIOS, SouthernIsland_Interior_EventScript_Sign
	goto_if_unset FLAG_ENABLE_SHIP_SOUTHERN_ISLAND, SouthernIsland_Interior_EventScript_Sign
	setflag FLAG_ENCOUNTERED_LATIAS_OR_LATIOS
	setflag FLAG_TEMP_2
	special SpawnCameraObject
	applymovement OBJ_EVENT_ID_CAMERA, SouthernIsland_Interior_Movement_CameraPanUp
	waitmovement 0
	delay 50
	waitse
	playmoncry VAR_TEMP_4, 0
	delay 30
	waitmoncry
	addobject 2
	delay 30
	applymovement OBJ_EVENT_ID_CAMERA, SouthernIsland_Interior_Movement_CameraPanDown
	applymovement 2, SouthernIsland_Interior_Movement_LatiApproach
	waitmovement 0
	delay 50
	special RemoveCameraObject
	setvar VAR_LAST_TALKED, 2
	compare VAR_ROAMER_POKEMON, 0
	call_if_eq SouthernIsland_Interior_EventScript_SetLatiosBattleVars
	compare VAR_ROAMER_POKEMON, 0
	call_if_ne SouthernIsland_Interior_EventScript_SetLatiasBattleVars
	setflag FLAG_SYS_CTRL_OBJ_DELETE
	special BattleSetup_StartLatiBattle
	waitstate
	clearflag FLAG_SYS_CTRL_OBJ_DELETE
	specialvar VAR_RESULT, GetBattleOutcome
	compare VAR_RESULT, B_OUTCOME_WON
	goto_if_eq SouthernIsland_Interior_EventScript_LatiDefeated
	compare VAR_RESULT, B_OUTCOME_RAN
	goto_if_eq SouthernIsland_Interior_EventScript_RanFromLati
	compare VAR_RESULT, B_OUTCOME_PLAYER_TELEPORTED
	goto_if_eq SouthernIsland_Interior_EventScript_RanFromLati
	setflag FLAG_CAUGHT_LATIAS_OR_LATIOS
	releaseall
	end

SouthernIsland_Interior_EventScript_LatiDefeated:: @ 8242B81
	setflag FLAG_DEFEATED_LATIAS_OR_LATIOS
	copyvar VAR_0x8004, VAR_TEMP_4
	goto Common_EventScript_LegendaryFlewAway
	end

SouthernIsland_Interior_EventScript_RanFromLati:: @ 8242B8F
	copyvar VAR_0x8004, VAR_TEMP_4
	goto Common_EventScript_LegendaryFlewAway
	end

SouthernIsland_Interior_EventScript_Sign:: @ 8242B9A
	msgbox SouthernIsland_Interior_Text_Sign, MSGBOX_DEFAULT
	releaseall
	end

SouthernIsland_Interior_EventScript_SetLatiosBattleVars:: @ 8242BA4
	setvar VAR_0x8004, SPECIES_LATIOS
	setvar VAR_0x8005, 50 @ level
	setvar VAR_0x8006, ITEM_SOUL_DEW
	special CreateObedientEnemyMon
	return

SouthernIsland_Interior_EventScript_SetLatiasBattleVars:: @ 8242BB7
	setvar VAR_0x8004, SPECIES_LATIAS
	setvar VAR_0x8005, 50 @ level
	setvar VAR_0x8006, ITEM_SOUL_DEW
	special CreateObedientEnemyMon
	return

SouthernIsland_Interior_Movement_CameraPanUp: @ 8242BCA
	walk_up
	walk_up
	walk_up
	step_end

SouthernIsland_Interior_Movement_CameraPanDown: @ 8242BCE
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	walk_down
	walk_down
	walk_down
	walk_in_place_fastest_up
	step_end

SouthernIsland_Interior_Movement_LatiApproach: @ 8242BDA
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	delay_16
	delay_16
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

SouthernIsland_Interior_EventScript_Steven::
	faceplayer
	msgbox SouthernIsland_Interior_Text_Steven7, MSGBOX_DEFAULT
	closemessage
	end

SouthernIsland_Interior_TriggerScript_EnterLeft::
	lockall
	applymovement OBJ_EVENT_ID_PLAYER, SouthernIsland_Interior_Movement_EnterLeft
	waitmovement 0
	goto SouthernIsland_Interior_TriggerScript_EnterMain
	end

SouthernIsland_Interior_TriggerScript_EnterRight::
	lockall
	applymovement OBJ_EVENT_ID_PLAYER, SouthernIsland_Interior_Movement_EnterRight
	waitmovement 0
	goto SouthernIsland_Interior_TriggerScript_EnterMain
	end

SouthernIsland_Interior_TriggerScript_EnterMain::
	msgbox SouthernIsland_Interior_Text_Steven0, MSGBOX_DEFAULT
	closemessage
	playmoncry SPECIES_LATIOS, 0
	delay 90
	addobject 5
	addobject 4
	playbgm MUS_MGM0, 0
	delay 65
	applymovement 4, SouthernIsland_Interior_Movement_Magma
	applymovement 5, SouthernIsland_Interior_Movement_Magma
	waitmovement 0
	applymovement 3, SouthernIsland_Interior_Movement_PlayerTurnsDown
	applymovement OBJ_EVENT_ID_PLAYER, SouthernIsland_Interior_Movement_PlayerTurnsDown
	msgbox SouthernIsland_Interior_Text_Steven1, MSGBOX_DEFAULT
	closemessage
	playse SE_PIN
	applymovement 4, Common_Movement_ExclamationMark
	delay 30
	applymovement 4, SouthernIsland_Interior_Movement_PlayerTurnsLeft
	waitmovement 0
	msgbox SouthernIsland_Interior_Text_Magma0, MSGBOX_DEFAULT
	applymovement 4, SouthernIsland_Interior_Movement_PlayerTurnsUp
	msgbox SouthernIsland_Interior_Text_Magma1, MSGBOX_DEFAULT
	msgbox SouthernIsland_Interior_Text_Magma2, MSGBOX_YESNO
	compare VAR_RESULT, YES
	call_if_eq SouthernIsland_Interior_EventScript_Serious
	compare VAR_RESULT, NO
	call_if_eq SouthernIsland_Interior_EventScript_ThatsRight
	applymovement 3, SouthernIsland_Interior_Movement_PlayerTurnsDown
	msgbox SouthernIsland_Interior_Text_Steven3, MSGBOX_DEFAULT
	closemessage
	multi_fixed_2_vs_2 TRAINER_COURTNEY, Enemy2DefeatText, TRAINER_GRUNT_SOUTHERNISLAND, Enemy1DefeatText, TRAINER_STEVEN_SOUTHERNISLAND, TRAINER_BACK_PIC_STEVEN
	specialvar VAR_RESULT, GetBattleOutcome
    compare VAR_RESULT, B_OUTCOME_LOST
    goto_if_eq Script_PlayerLostTheFight
	msgbox SouthernIsland_Interior_Text_Magma3, MSGBOX_DEFAULT
	closemessage
	applymovement 5, SouthernIsland_Interior_Movement_Magma2
	delay 30
	applymovement 4, SouthernIsland_Interior_Movement_PlayerTurnsLeft
	delay 30
	applymovement 4, SouthernIsland_Interior_Movement_PlayerTurnsDown
	delay 30
	msgbox SouthernIsland_Interior_Text_Magma4, MSGBOX_DEFAULT
	closemessage
	applymovement 4, SouthernIsland_Interior_Movement_Magma3
	waitmovement 0
	removeobject 5
	removeobject 4
	applymovement 3, SouthernIsland_Interior_Movement_PlayerTurnsLeft
	applymovement OBJ_EVENT_ID_PLAYER, SouthernIsland_Interior_Movement_PlayerTurnsRight
	waitmovement 0
	msgbox SouthernIsland_Interior_Text_Steven5, MSGBOX_DEFAULT
	playmoncry SPECIES_LATIOS, 0
	waitmoncry
	playmoncry SPECIES_LATIAS, 0
	waitmoncry
	applymovement 3, SouthernIsland_Interior_Movement_PlayerTurnsUp
	applymovement OBJ_EVENT_ID_PLAYER, SouthernIsland_Interior_Movement_PlayerTurnsUp
	msgbox SouthernIsland_Interior_Text_Steven6, MSGBOX_DEFAULT
	setvar VAR_ROUTE118_STATE, 3
	closemessage
	release
	end

SouthernIsland_Interior_EventScript_Serious::
	applymovement 3, SouthernIsland_Interior_Movement_PlayerTurnsLeft
	msgbox SouthernIsland_Interior_Text_Steven2, MSGBOX_DEFAULT
	closemessage
	return

SouthernIsland_Interior_EventScript_ThatsRight::
	applymovement 3, SouthernIsland_Interior_Movement_PlayerTurnsLeft
	msgbox SouthernIsland_Interior_Text_Steven4, MSGBOX_DEFAULT
	closemessage
	return

SouthernIsland_Interior_EventScript_Latias:: @ 8242AD0
	lockall
	playmoncry SPECIES_LATIAS, 0
	compare VAR_ROUTE118_STATE, 3
	call_if_eq SouthernIsland_Interior_EventScript_LatiasQuestion
	releaseall
	end

SouthernIsland_Interior_EventScript_LatiasQuestion:: @ 8242AD0
	msgbox SouthernIsland_Interior_Text_WantLatias, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq SouthernIsland_Interior_EventScript_ChoseLatias
	closemessage
	return

SouthernIsland_Interior_EventScript_ChoseLatias::
	setvar VAR_TEMP_1, SPECIES_LATIAS
	bufferspeciesname 0, SPECIES_LATIAS
	playfanfare MUS_FANFA4
	msgbox Text_ReceivedLatias, MSGBOX_DEFAULT
	waitfanfare
	givemon SPECIES_LATIAS, 30, ITEM_LATIASITE
	compare VAR_RESULT, 0
	call_if_eq SouthernIsland_Interior_EventScript_SendLatiasParty
	compare VAR_RESULT, 1
	call_if_eq SouthernIsland_Interior_EventScript_SendLatiasPC
	goto Common_EventScript_NoMoreRoomForPokemon
	end

SouthernIsland_Interior_EventScript_Latios:: @ 8242AD0
	lockall
	playmoncry SPECIES_LATIOS, 0
	compare VAR_ROUTE118_STATE, 3
	goto_if_eq SouthernIsland_Interior_EventScript_LatiosQuestion
	releaseall
	end

SouthernIsland_Interior_EventScript_LatiosQuestion:: @ 8242AD0
	msgbox SouthernIsland_Interior_Text_WantLatios, MSGBOX_YESNO
	compare VAR_RESULT, YES
	call_if_eq SouthernIsland_Interior_EventScript_ChoseLatios
	closemessage
	return

SouthernIsland_Interior_EventScript_ChoseLatios::
	setvar VAR_TEMP_1, SPECIES_LATIOS
	bufferspeciesname 0, SPECIES_LATIOS
	playfanfare MUS_FANFA4
	msgbox Text_ReceivedLatios, MSGBOX_DEFAULT
	waitfanfare
	givemon SPECIES_LATIOS, 30, ITEM_LATIOSITE
	compare VAR_RESULT, 0
	call_if_eq SouthernIsland_Interior_EventScript_SendLatiosParty
	compare VAR_RESULT, 1
	call_if_eq SouthernIsland_Interior_EventScript_SendLatiosPC
	goto Common_EventScript_NoMoreRoomForPokemon
	end

SouthernIsland_Interior_EventScript_SendLatiasParty:: @ 8222895
	removeobject 2
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq SouthernIsland_Interior_EventScript_RecievedLatias
	call Common_EventScript_GetGiftMonPartySlot
	call Common_EventScript_NameReceivedPartyMon
	goto SouthernIsland_Interior_EventScript_RecievedLatias
	end

SouthernIsland_Interior_EventScript_SendLatiasPC::
	removeobject 2
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq SouthernIsland_Interior_EventScript_RecievedLatias
	call Common_EventScript_NameReceivedBoxMon
	goto SouthernIsland_Interior_EventScript_RecievedLatias
	end

SouthernIsland_Interior_EventScript_RecievedLatias::
	call Common_EventScript_TransferredToPC
	closemessage
	applymovement 3, SouthernIsland_Interior_Movement_StevenMove1
	waitmovement 0
	applymovement OBJ_EVENT_ID_PLAYER, SouthernIsland_Interior_Movement_PlayerTurnsRight
	msgbox SouthernIsland_Interior_Text_Return1, MSGBOX_DEFAULT
	giveitem ITEM_MEGA_BRACELET
	bufferspeciesname 0, SPECIES_LATIAS
	msgbox SouthernIsland_Interior_Text_Return2, MSGBOX_DEFAULT
	clearflag FLAG_HIDE_ROUTE_118_STEVEN2
	warp MAP_ROUTE118, 255, 43, 11
	end

SouthernIsland_Interior_EventScript_SendLatiosParty:: @ 8222895
	removeobject 1
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq SouthernIsland_Interior_EventScript_RecievedLatios
	call Common_EventScript_GetGiftMonPartySlot
	call Common_EventScript_NameReceivedPartyMon
	goto SouthernIsland_Interior_EventScript_RecievedLatios
	end

SouthernIsland_Interior_EventScript_SendLatiosPC::
	removeobject 1
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq SouthernIsland_Interior_EventScript_RecievedLatios
	call Common_EventScript_NameReceivedBoxMon
	goto SouthernIsland_Interior_EventScript_RecievedLatios
	end

SouthernIsland_Interior_EventScript_RecievedLatios::
	call Common_EventScript_TransferredToPC
	closemessage
	applymovement 3, SouthernIsland_Interior_Movement_StevenMove2
	waitmovement 0
	applymovement OBJ_EVENT_ID_PLAYER, SouthernIsland_Interior_Movement_PlayerTurnsLeft
	msgbox SouthernIsland_Interior_Text_Return1, MSGBOX_DEFAULT
	giveitem ITEM_MEGA_BRACELET
	bufferspeciesname 0, SPECIES_LATIOS
	msgbox SouthernIsland_Interior_Text_Return2, MSGBOX_DEFAULT
	clearflag FLAG_HIDE_ROUTE_118_STEVEN2
	warp MAP_ROUTE118, 255, 43, 11
	end

Text_ReceivedLatias::
	.string "{PLAYER} received a Latias!$"

Text_ReceivedLatios::
	.string "{PLAYER} received a Latios!$"

SouthernIsland_Interior_Movement_EnterLeft::
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

SouthernIsland_Interior_Movement_EnterRight::
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_left
	walk_up
	walk_up
	step_end

SouthernIsland_Interior_Movement_StevenMove1::
	walk_up
	walk_up
	walk_left
	step_end

SouthernIsland_Interior_Movement_StevenMove2::
	walk_up
	walk_up
	walk_in_place_right
	step_end

SouthernIsland_Interior_Movement_Magma::
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

SouthernIsland_Interior_Movement_Magma2::
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	step_end

SouthernIsland_Interior_Movement_Magma3::
	walk_fastest_down
	walk_fastest_down
	walk_fastest_down
	walk_fastest_down
	walk_fastest_down
	step_end

SouthernIsland_Interior_Movement_PlayerTurnsLeft::
	walk_in_place_left
	step_end

SouthernIsland_Interior_Movement_PlayerTurnsUp::
	walk_in_place_up
	step_end

SouthernIsland_Interior_Movement_PlayerTurnsDown::
	walk_in_place_down
	step_end

SouthernIsland_Interior_Movement_PlayerTurnsRight::
	walk_in_place_right
	step_end

Script_PlayerLostTheFight::
	msgbox Text_PlayerLostTheFight, MSGBOX_DEFAULT
	closemessage
	release
	end

Enemy1DefeatText::
	.string "F.$"

Enemy2DefeatText::
	.string "Big F.$"

Text_PlayerLostTheFight::
	.string "{PLAYER}{KUN} lost the battle.$"

SouthernIsland_Interior_Text_Steven0::
	.string "Latios...\n"
	.string "It's beautiful...\p"
	.string "If I were to describe such beauty in\n"
	.string "terms of a stone, it would be a Water\l"
	.string "Stone.\p"
	.string "No...It goes beyond that...\n"
	.string "Isn't its brilliance like that of a\l"
	.string "Dawn Stone?\p"
	.string "It is as fantastic as a Steel-type\n"
	.string "Pokémon...$"

SouthernIsland_Interior_Text_Steven1::
	.string "Hm?\p"
	.string "Team Magma was it?\p"
	.string "So Latias and Latios noticed their\n"
	.string "coming, and that is why they called on\l"
	.string "us.\p"
	.string "Magma Admin: ...\p"
	.string "Who are you?$"

SouthernIsland_Interior_Text_Steven2::
	.string "It's good that you feel up to joking, but\n"
	.string "this is no laughing matter.\p"
	.string "I'll take part in this battle as well.$"

SouthernIsland_Interior_Text_Magma0::
	.string "Magma Grunt: Hah?!\p"
	.string "M-Mistress Courtney!\p"
	.string "That pipsqueak over there is the kid\n"
	.string "that I heard keeps getting in our way!\p"
	.string "But that enviably unruffled-looking\n"
	.string "fellow there with him...\p"
	.string "Hm. I feel like I ought to know him from\n"
	.string "somewhere...\l"
	.string "Maybe he's a contest star?$"

SouthernIsland_Interior_Text_Magma1::
	.string "Courtney: Yes.\p"
	.string "For the time being...get away from me.\p"
	.string "The Pokémon behind you...\p"
	.string "Their Mega Stones...\p"
	.string "No more...no less.\p"
	.string "That is what...\p"
	.string "That's what we of Team Magma seek.$"

SouthernIsland_Interior_Text_Magma2::
	.string "How about it?\n"
	.string "Will you get away from me...\l"
	.string "or not?$"

SouthernIsland_Interior_Text_Magma3::
	.string "Magma Grunt: Puyah?!\p"
	.string "As anticipated.\p"
	.string "Courtney: Unanticipated.\p"
	.string "You.\n"
	.string "Target lock...completed.\p"
	.string "Commencing...experiment.\p"
	.string "You. Forever.\n"
	.string "Aha...\p"
	.string "Returning home.$"

SouthernIsland_Interior_Text_Magma4::
	.string "W-wait for me!\n"
	.string "Courtne-e-e-y!$"

SouthernIsland_Interior_Text_Steven5::
	.string "Hah...\p"
	.string "Good work, {PLAYER}{KUN}.\p"
	.string "You performed spectacularly.\p"
	.string "We were able to protect both these\n"
	.string "wondrous Pokémon and their Mega Stones,\l"
	.string "thanks to you.$"

SouthernIsland_Interior_Text_Steven6::
	.string "And it looks like as though they may\n"
	.string "have also recognised you as a trainer\l"
	.string "worthy of their trust.\p"
	.string "Look. They want to help you in some way.\p"
	.string "I believe they want you to bring one of\n"
	.string "them on your journey.\p"
	.string "Go on. Choose one.$"

SouthernIsland_Interior_Text_Steven7::
	.string "Go on. Choose one.$"

SouthernIsland_Interior_Text_Steven3::
	.string "Let's protect Latias and Latios from\n"
	.string "these beasts together.\p"
	.string "We cannot allow them or even a Mega Stone\n"
	.string "to fall into the hands of such villains!\p"
	.string "Courtney: ...\p"
	.string "Well then...\p"
	.string "Deleting...$"

SouthernIsland_Interior_Text_Steven4::
	.string "That's right.\n"
	.string "Full points for the correct answer!\p"
	.string "I'll take part in this battle as well.$"

SouthernIsland_Interior_Text_Return1::
	.string "You should take this from me...$"

SouthernIsland_Interior_Text_Return2::
	.string "With the Mega Bracelet that I just\n"
	.string "gave you and the Mega Stone that a\l"
	.string "Pokémon may hold, your Pokémon will be\l"
	.string "able to achieve a powerful boost, quite\l"
	.string "unlike any usual form of Evolution...\l"
	.string "but only if you are bound by great\l"
	.string "affection.\p"
	.string "You will be able to achieve Mega\n"
	.string "Evolution.\p"
	.string "I suggest you try it with {STR_VAR_1} the\n"
	.string "next time that you happen to have the\l"
	.string "chance in battle.\p"
	.string "Mega Evolution is a phenomenon that\n"
	.string "has long been shrouded in mystery.\p"
	.string "What is a Mega Stone?\n"
	.string "What is a Key Stone?\p"
	.string "The process by which bonds between\n"
	.string "Pokémon and people manifest in tangible\l"
	.string "power... How do we explain that?\p"
	.string "I hope the Mega Bracelet I just gave\n"
	.string "you might bring us one step closer to\l"
	.string "finding the answer to these questions.\p"
	.string "If that settles that...\n"
	.string "We should be heading back.$"

SouthernIsland_Interior_Text_WantLatias::
	.string "Want Latias to join the party?$"

SouthernIsland_Interior_Text_WantLatios::
	.string "Want Latios to join the party?$"
