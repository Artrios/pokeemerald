EventScript_RepelWoreOff:: @ 82A4B2A
	compare VAR_REPEL_TYPE, 1
	goto_if_eq EventScript_RepelWoreOff_AskAnother
	compare VAR_REPEL_TYPE, 2
	goto_if_eq EventScript_SuperRepelWoreOff_AskAnother
	compare VAR_REPEL_TYPE, 3
	goto_if_eq EventScript_MaxRepelWoreOff_AskAnother
	goto EventScript_RepelWoreOff_MsgBox

EventScript_RepelWoreOff_AskAnother:
	checkitem ITEM_REPEL, 1
	compare VAR_RESULT, 0
	goto_if_eq EventScript_RepelWoreOff_MsgBox
	msgbox Text_RepelAskAnother, MSGBOX_YESNO
	compare VAR_RESULT, 0
	goto_if_eq EventScript_RepelWoreOff_End
	removeitem ITEM_REPEL, 1
	setvar VAR_REPEL_STEP_COUNT, 0x64
	playse SE_REPEL
	msgbox Text_UsedARepel, MSGBOX_SIGN
	end
	
EventScript_SuperRepelWoreOff_AskAnother:
	checkitem ITEM_SUPER_REPEL, 1
	compare VAR_RESULT, 0
	goto_if_eq EventScript_RepelWoreOff_MsgBox
	msgbox Text_RepelAskAnother, MSGBOX_YESNO
	compare VAR_RESULT, 0
	goto_if_eq EventScript_RepelWoreOff_End
	removeitem ITEM_SUPER_REPEL, 1
	setvar VAR_REPEL_STEP_COUNT, 0xC8
	playse SE_REPEL
	msgbox Text_UsedASuperRepel, MSGBOX_SIGN
	end
	
EventScript_MaxRepelWoreOff_AskAnother:
	checkitem ITEM_MAX_REPEL, 1
	compare VAR_RESULT, 0
	goto_if_eq EventScript_RepelWoreOff_MsgBox
	msgbox Text_RepelAskAnother, MSGBOX_YESNO
	compare VAR_RESULT, 0
	goto_if_eq EventScript_RepelWoreOff_End
	removeitem ITEM_MAX_REPEL, 1
	setvar VAR_REPEL_STEP_COUNT, 0xFA
	playse SE_REPEL
	msgbox Text_UsedAMaxRepel, MSGBOX_SIGN
	end
	
EventScript_RepelWoreOff_MsgBox:
	msgbox Text_RepelWoreOff, MSGBOX_SIGN
	release
	end

EventScript_RepelWoreOff_End:
	closemessage
	end

Text_UsedARepel:
	.string "{PLAYER} used the\nRepel.$"	

Text_UsedASuperRepel:
	.string "{PLAYER} used the\nSuper Repel.$"	

Text_UsedAMaxRepel:
	.string "{PLAYER} used the\nMax Repel.$"	

Text_RepelWoreOff: @ 82A4B33
	.string "Repel's effect wore off…$"

Text_RepelAskAnother:
	.string "Repel's effect wore off…\n"
	.string "Use another?$"