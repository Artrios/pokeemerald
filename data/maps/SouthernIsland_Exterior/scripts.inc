.set LOCALID_SAILOR, 1
.set LOCALID_SS_TIDAL, 2

SouthernIsland_Exterior_MapScripts:: @ 82429C8
	map_script MAP_SCRIPT_ON_TRANSITION, SouthernIsland_Exterior_OnTransition
	.byte 0

SouthernIsland_Exterior_OnTransition: @ 82429CE
	setflag FLAG_LANDMARK_SOUTHERN_ISLAND
	end

SouthernIsland_Exterior_EventScript_Sailor:: @ 82429D2
	lock
	faceplayer
	msgbox EventTicket_Text_SouthernIslandSailBack, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq SouthernIsland_Exterior_EventScript_AsYouLike
	msgbox EventTicket_Text_SailHome, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_WalkInPlaceFastestDown
	waitmovement 0
	delay 30
	hideobjectat LOCALID_SAILOR, MAP_SOUTHERN_ISLAND_EXTERIOR
	setvar VAR_0x8004, LOCALID_SS_TIDAL
	call Common_EventScript_FerryDepartIsland
	warp MAP_LILYCOVE_CITY_HARBOR, 255, 8, 11
	waitstate
	release
	end

SouthernIsland_Exterior_EventScript_AsYouLike:: @ 8242A17
	msgbox EventTicket_Text_AsYouLike, MSGBOX_DEFAULT
	release
	end

Ferry_EventScript_DepartIslandSouth:: @ 8242A21
	applymovement OBJ_EVENT_ID_PLAYER, Ferry_EventScript_DepartIslandBoardSouth
	waitmovement 0
	return

Ferry_EventScript_DepartIslandWest:: @ 8242A2C
	applymovement OBJ_EVENT_ID_PLAYER, Ferry_EventScript_DepartIslandBoardWest
	waitmovement 0
	return

Ferry_EventScript_DepartIslandBoardSouth: @ 8242A37
	walk_down
	step_end

Ferry_EventScript_DepartIslandBoardWest: @ 8242A39
	walk_left
	walk_in_place_fastest_down
	step_end

SouthernIsland_Exterior_EventScript_Sign:: @ 8242A3C
	msgbox SouthernIsland_Exterior_Text_Sign, MSGBOX_SIGN
	end

SouthernIsland_Exterior_EventScript_Steven:: @ 81F3E8E
	lockall
	setvar VAR_0x8008, 0
	faceplayer
	goto SouthernIsland_Exterior_EventScript_StevenEvent
	end

SouthernIsland_Exterior_EventScript_StevenTrigger:: @ 81F3E8E
	lockall
	setvar VAR_0x8008, 1
	call SouthernIsland_Exterior_EventScript_StevenTurn0
	goto SouthernIsland_Exterior_EventScript_StevenEvent
	end

SouthernIsland_Exterior_EventScript_StevenEvent:: @ 81F3E8E
	lockall
	msgbox SouthernIsland_Exterior_Text_StevenArrives0, MSGBOX_DEFAULT
	call SouthernIsland_Exterior_EventScript_StevenTurn1
	msgbox SouthernIsland_Exterior_Text_StevenArrives1, MSGBOX_DEFAULT
	compare VAR_0x8008, 0
	call_if_eq SouthernIsland_Exterior_EventScript_StevenTurn3
	compare VAR_0x8008, 1
	call_if_eq SouthernIsland_Exterior_EventScript_StevenTurn2
	msgbox SouthernIsland_Exterior_Text_StevenArrives2, MSGBOX_DEFAULT
	closemessage
	call SouthernIsland_Exterior_EventScript_StevenLeaves
	removeobject 3
	setvar VAR_ROUTE118_STATE, 1
	release
	end

SouthernIsland_Exterior_EventScript_StevenTurn0:: @ 81F3F28
	applymovement OBJ_EVENT_ID_PLAYER, SouthernIsland_Exterior_Movement_PlayerTurnsRight
	applymovement 3, SouthernIsland_Exterior_Movement_PlayerTurnsLeft
	waitmovement 0
	return

SouthernIsland_Exterior_EventScript_StevenTurn1:: @ 81F3F28
	applymovement 3, SouthernIsland_Exterior_Movement_PlayerTurnsUp
	waitmovement 0
	return

SouthernIsland_Exterior_EventScript_StevenTurn2:: @ 81F3F28
	applymovement 3, SouthernIsland_Exterior_Movement_PlayerTurnsLeft
	waitmovement 0
	return

SouthernIsland_Exterior_EventScript_StevenTurn3:: @ 81F3F28
	applymovement 3, SouthernIsland_Exterior_Movement_PlayerTurnsDown
	waitmovement 0
	return

SouthernIsland_Exterior_EventScript_StevenLeaves:: @ 81F3F28
	applymovement 3, SouthernIsland_Exterior_Movement_StevenLeaves
	waitmovement 0
	return

SouthernIsland_Exterior_Movement_PlayerTurnsRight::
	walk_in_place_fastest_right
	step_end

SouthernIsland_Exterior_Movement_PlayerTurnsLeft::
	walk_in_place_fastest_left
	step_end

SouthernIsland_Exterior_Movement_PlayerTurnsUp::
	walk_in_place_fastest_up
	step_end

SouthernIsland_Exterior_Movement_PlayerTurnsDown::
	walk_in_place_fastest_down
	step_end

SouthernIsland_Exterior_Movement_StevenLeaves::
	walk_up
	walk_up
	walk_up
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	step_end

SouthernIsland_Exterior_EventScript_Trigger0:: @ 81F3E8E
	lockall
	msgbox SouthernIsland_Exterior_Text_Voice0, MSGBOX_DEFAULT
	closemessage
	playse SE_PIN
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_ExclamationMark
	delay 60
	msgbox SouthernIsland_Exterior_Text_Voice1, MSGBOX_DEFAULT
	setvar VAR_ROUTE118_STATE, 2
	release
	end

SouthernIsland_Exterior_Text_StevenArrives0::
	.string "Steven: We've arrived.\p"
	.string "But where did Latias fly off to?$"

SouthernIsland_Exterior_Text_StevenArrives1::
	.string "...\p"
	.string "I don't know what it is...\p"
	.string "Not quite unease, not quite fear...\p"
	.string "But there is something...unsettling...\p"
	.string "It is an odd feeling, as though I am\n"
	.string "walking in a dream-or an illusion.$"

SouthernIsland_Exterior_Text_StevenArrives2::
	.string "I once heard an odd fable.\p"
	.string "It was said that the two Pokémon that\n"
	.string "live on this island guard some secret\l"
	.string "about that great form of Evolution:\l"
	.string "Mega Evolution.\p"
	.string "Could it be that some dangerous element\n"
	.string "is here because of that?\p"
	.string "...\p"
	.string "Come, we should be going.$"

SouthernIsland_Exterior_Text_Voice0::
	.string "(Th- wh- mem-de\n"
	.string "se- to carv- th-rts...)$"

SouthernIsland_Exterior_Text_Voice1::
	.string "(Those whose mememories fade\n"
	.string "seek to carve them in their hearts...)$"
