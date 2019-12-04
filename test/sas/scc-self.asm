	cpu 8086
	org 0x100
Start:
	XOR	BP, BP
	MOV	WORD [_HeapStart], ProgramEnd
	MOV	AX, 0x81
	PUSH	AX
	MOV	AL, [0x80]
	PUSH	AX
	PUSH	AX
	JMP	_CallMain

_DosCall:
	PUSH	BP
	MOV	BP, SP
	MOV	BX, [BP+4]
	MOV	AX, [BX]
	MOV	BX, [BP+6]
	MOV	CX, [BP+8]
	MOV	DX, [BP+10]
	INT	0x21
	MOV	BX, [BP+4]
	MOV	[BX], AX
	MOV	AX, 0
	SBB	AX, AX
	POP	BP
	RET

_OutFile:
	DW	0
_LineBuf:
	DW	0
_TempBuf:
	DW	0
_InFile:
	DW	0
_InBuf:
	DW	0
_InBufCnt:
	DW	0
_InBufSize:
	DW	0
_UngottenChar:
	DW	0
_Line:
	DW	1
_TokenType:
	DW	0
_TokenNumVal:
	DW	0
_IdBuffer:
	DW	0
_IdBufferIndex:
	DW	0
_IdOffset:
	DW	0
_IdCount:
	DW	0
_VarDeclId:
	DW	0
_VarDeclType:
	DW	0
_VarDeclOffset:
	DW	0
_Scopes:
	DW	0
_ScopesCount:
	DW	0
_LocalOffset:
	DW	0
_LocalLabelCounter:
	DW	0
_ReturnLabel:
	DW	0
_BCStackLevel:
	DW	0
_BreakLabel:
	DW	0
_ContinueLabel:
	DW	0
_CurrentType:
	DW	0
_CurrentVal:
	DW	0
_ReturnUsed:
	DW	0
_PendingPushAx:
	DW	0
_IsDeadCode:
	DW	0
_IsDigit:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	CMP	AX, 48
	JNL	.L1
	MOV	AX, 0
	JMP	.L2
.L1:
	MOV	AX, [BP+4]
	CMP	AX, 57
	MOV	AX, 1
	JNG	.L3
	DEC	AL
.L3:
.L2:
	JMP	.L0
.L0:
	POP	BP
	RET
_IsAlpha:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	CMP	AX, 65
	JNL	.L1
	MOV	AX, 0
	JMP	.L2
.L1:
	MOV	AX, [BP+4]
	CMP	AX, 90
	MOV	AX, 1
	JNG	.L3
	DEC	AL
.L3:
.L2:
	AND	AX, AX
	JZ	.L4
	JMP	.L5
.L4:
	MOV	AX, [BP+4]
	CMP	AX, 97
	JNL	.L6
	MOV	AX, 0
	JMP	.L7
.L6:
	MOV	AX, [BP+4]
	CMP	AX, 122
	MOV	AX, 1
	JNG	.L8
	DEC	AL
.L8:
.L7:
.L5:
	JMP	.L0
.L0:
	POP	BP
	RET
_PutStr:
	PUSH	BP
	MOV	BP, SP
.L1:
	MOV	AX, [BP+4]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	JNZ	.L2
	JMP	.L3
.L2:
	ADD	SP, -8
	MOV	AX, [BP+4]
	PUSH	AX
	INC	AX
	MOV	[BP+4], AX
	POP	AX
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	MOV	[BP-8], AX
	CALL	_putchar
	ADD	SP, 8
	JMP	.L1
.L3:
.L0:
	POP	BP
	RET
_CopyStr:
	PUSH	BP
	MOV	BP, SP
.L1:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	JNZ	.L2
	JMP	.L3
.L2:
	MOV	AX, [BP+4]
	PUSH	AX
	INC	AX
	MOV	[BP+4], AX
	POP	AX
	PUSH	AX
	MOV	AX, [BP+6]
	PUSH	AX
	INC	AX
	MOV	[BP+6], AX
	POP	AX
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	POP	BX
	MOV	[BX], AL
	JMP	.L1
.L3:
	MOV	AX, [BP+4]
	JMP	.L0
.L0:
	POP	BP
	RET
_StrLen:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, 0
	PUSH	AX	; [BP-2] = l
.L1:
	MOV	AX, [BP+4]
	PUSH	AX
	INC	AX
	MOV	[BP+4], AX
	POP	AX
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	JNZ	.L2
	JMP	.L3
.L2:
	MOV	AX, [BP-2]
	INC	AX
	MOV	[BP-2], AX
	JMP	.L1
.L3:
	MOV	AX, [BP-2]
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_OutputStr:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	AX, [_OutFile]
	MOV	[BP-8], AX
	MOV	AX, [BP+4]
	MOV	[BP-6], AX
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-16], AX
	CALL	_StrLen
	ADD	SP, 8
	MOV	[BP-4], AX
	CALL	_write
	ADD	SP, 8
.L0:
	POP	BP
	RET
_VSPrintf:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ch
.L1:
	MOV	AX, [BP+6]
	PUSH	AX
	INC	AX
	MOV	[BP+6], AX
	POP	AX
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	MOV	[BP-2], AL
	AND	AX, AX
	JNZ	.L2
	JMP	.L3
.L2:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 37
	JNZ	.L4
	JMP	.L5
.L4:
	MOV	AX, [BP+4]
	PUSH	AX
	INC	AX
	MOV	[BP+4], AX
	POP	AX
	PUSH	AX
	MOV	AL, [BP-2]
	CBW
	POP	BX
	MOV	[BX], AL
	JMP	.L1
.L5:
.L6:
	MOV	AX, [BP+6]
	PUSH	AX
	INC	AX
	MOV	[BP+6], AX
	POP	AX
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	MOV	[BP-2], AL
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 115
	JZ	.L7
	JMP	.L8
.L7:
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-10], AX
	MOV	AX, [BP+8]
	ADD	AX, 2
	MOV	[BP+8], AX
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[BP-8], AX
	CALL	_CopyStr
	ADD	SP, 8
	MOV	[BP+4], AX
	JMP	.L9
.L8:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 99
	JZ	.L10
	JMP	.L11
.L10:
	MOV	AX, [BP+4]
	PUSH	AX
	INC	AX
	MOV	[BP+4], AX
	POP	AX
	PUSH	AX
	MOV	AX, [BP+8]
	ADD	AX, 2
	MOV	[BP+8], AX
	MOV	BX, AX
	MOV	AX, [BX]
	POP	BX
	MOV	[BX], AL
	JMP	.L12
.L11:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 43
	JZ	.L16
	MOV	AX, 0
	JMP	.L17
.L16:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	CMP	AX, 100
	MOV	AX, 1
	JZ	.L18
	DEC	AL
.L18:
.L17:
	AND	AX, AX
	JZ	.L19
	JMP	.L20
.L19:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 100
	MOV	AX, 1
	JZ	.L21
	DEC	AL
.L21:
.L20:
	AND	AX, AX
	JNZ	.L13
	JMP	.L14
.L13:
	PUSH	AX	; [BP-4] = buf
	PUSH	AX	; [BP-6] = n
	PUSH	AX	; [BP-8] = s
	PUSH	AX	; [BP-10] = always
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 43
	JZ	.L22
	JMP	.L23
.L22:
	MOV	AX, [BP+6]
	INC	AX
	MOV	[BP+6], AX
	MOV	WORD [BP-10], 1
	JMP	.L24
.L23:
	MOV	WORD [BP-10], 0
.L24:
	MOV	AX, [BP+8]
	ADD	AX, 2
	MOV	[BP+8], AX
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[BP-6], AX
	MOV	WORD [BP-8], 0
	MOV	AX, [BP-6]
	CMP	AX, 0
	JL	.L25
	JMP	.L26
.L25:
	MOV	WORD [BP-8], 1
	MOV	AX, [BP-6]
	NEG	AX
	MOV	[BP-6], AX
	JMP	.L27
.L26:
.L27:
	MOV	AX, [_TempBuf]
	ADD	AX, 32
	MOV	[BP-4], AX
	MOV	AX, [BP-4]
	DEC	AX
	MOV	[BP-4], AX
	MOV	BX, AX
	MOV	BYTE [BX], 0
.L28:
	JMP	.L29
.L29:
	MOV	AX, [BP-4]
	DEC	AX
	MOV	[BP-4], AX
	PUSH	AX
	MOV	AX, [BP-6]
	MOV	CX, 10
	CWD
	IDIV	CX
	MOV	AX, DX
	ADD	AX, 48
	POP	BX
	MOV	[BX], AL
	MOV	AX, [BP-6]
	MOV	CX, 10
	CWD
	IDIV	CX
	MOV	[BP-6], AX
	MOV	AX, [BP-6]
	AND	AX, AX
	JZ	.L31
	JMP	.L32
.L31:
	JMP	.L30
.L32:
.L33:
	JMP	.L28
.L30:
	MOV	AX, [BP-8]
	AND	AX, AX
	JNZ	.L35
	JMP	.L36
.L35:
	MOV	AX, [BP-4]
	DEC	AX
	MOV	[BP-4], AX
	MOV	BX, AX
	MOV	BYTE [BX], 45
	JMP	.L37
.L36:
	MOV	AX, [BP-10]
	AND	AX, AX
	JNZ	.L38
	JMP	.L39
.L38:
	MOV	AX, [BP-4]
	DEC	AX
	MOV	[BP-4], AX
	MOV	BX, AX
	MOV	BYTE [BX], 43
	JMP	.L40
.L39:
.L40:
.L37:
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-18], AX
	MOV	AX, [BP-4]
	MOV	[BP-16], AX
	CALL	_CopyStr
	ADD	SP, 8
	MOV	[BP+4], AX
	ADD	SP, 8
	JMP	.L15
.L14:
	JMP	.L42
.L41:	DB 'Invalid format string', 0
.L42:
	ADD	SP, -8
	MOV	AX, .L41
	MOV	[BP-10], AX
	CALL	_PutStr
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-10], 1
	CALL	_exit
	ADD	SP, 8
.L15:
.L12:
.L9:
	JMP	.L1
.L3:
	MOV	AX, [BP+4]
	MOV	BX, AX
	MOV	BYTE [BX], 0
	MOV	AX, [BP+4]
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_Printf:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = vl
	LEA	AX, [BP+4]
	MOV	[BP-2], AX
	ADD	SP, -8
	MOV	AX, [_LineBuf]
	MOV	[BP-10], AX
	MOV	AX, [BP+4]
	MOV	[BP-8], AX
	MOV	AX, [BP-2]
	MOV	[BP-6], AX
	CALL	_VSPrintf
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [_LineBuf]
	MOV	[BP-10], AX
	CALL	_PutStr
	ADD	SP, 8
	ADD	SP, 2
.L0:
	POP	BP
	RET
_StrEqual:
	PUSH	BP
	MOV	BP, SP
.L1:
	MOV	AX, [BP+4]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	JNZ	.L4
	JMP	.L5
.L4:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
.L5:
	AND	AX, AX
	JNZ	.L6
	JMP	.L7
.L6:
	MOV	AX, [BP+4]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	PUSH	AX
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	POP	CX
	XCHG	AX, CX
	CMP	AX, CX
	MOV	AX, 1
	JZ	.L8
	DEC	AL
.L8:
.L7:
	AND	AX, AX
	JNZ	.L2
	JMP	.L3
.L2:
	MOV	AX, [BP+4]
	INC	AX
	MOV	[BP+4], AX
	MOV	AX, [BP+6]
	INC	AX
	MOV	[BP+6], AX
	JMP	.L1
.L3:
	MOV	AX, [BP+4]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	JZ	.L10
	MOV	AX, 0
	JMP	.L11
.L10:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	MOV	AX, 1
	JZ	.L13
	DEC	AL
.L13:
.L11:
	JMP	.L0
.L0:
	POP	BP
	RET
_Fatal:
	PUSH	BP
	MOV	BP, SP
	JMP	.L2
.L1:	DB 'In line %d: %s', 10, 0
.L2:
	ADD	SP, -8
	MOV	AX, .L1
	MOV	[BP-8], AX
	MOV	AX, [_Line]
	MOV	[BP-6], AX
	MOV	AX, [BP+4]
	MOV	[BP-4], AX
	CALL	_Printf
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-8], 1
	CALL	_exit
	ADD	SP, 8
.L0:
	POP	BP
	RET
_Check:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	AND	AX, AX
	JZ	.L1
	JMP	.L2
.L1:
	JMP	.L6
.L5:	DB 'Check failed', 0
.L6:
	ADD	SP, -8
	MOV	AX, .L5
	MOV	[BP-8], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L3
.L2:
.L3:
.L0:
	POP	BP
	RET
_IdText:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	AX, [BP+4]
	CMP	AX, 0
	JNL	.L1
	MOV	AX, 0
	JMP	.L2
.L1:
	MOV	AX, [BP+4]
	PUSH	AX
	MOV	AX, [_IdCount]
	POP	CX
	XCHG	AX, CX
	CMP	AX, CX
	MOV	AX, 1
	JL	.L3
	DEC	AL
.L3:
.L2:
	MOV	[BP-8], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_IdBuffer]
	PUSH	AX
	MOV	AX, [_IdOffset]
	PUSH	AX
	MOV	AX, [BP+4]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	POP	CX
	ADD	AX, CX
	JMP	.L0
.L0:
	POP	BP
	RET
_RawEmit:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	AX, [_PendingPushAx]
	AND	AX, AX
	MOV	AX, 1
	JZ	.L2
	DEC	AL
.L2:
	MOV	[BP-8], AX
	CALL	_Check
	ADD	SP, 8
	PUSH	AX	; [BP-2] = vl
	LEA	AX, [BP+4]
	MOV	[BP-2], AX
	ADD	SP, -8
	MOV	AX, [_LineBuf]
	MOV	[BP-10], AX
	MOV	AX, [BP+4]
	MOV	[BP-8], AX
	MOV	AX, [BP-2]
	MOV	[BP-6], AX
	CALL	_VSPrintf
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [_LineBuf]
	MOV	[BP-10], AX
	CALL	_OutputStr
	ADD	SP, 8
	ADD	SP, 2
.L0:
	POP	BP
	RET
_Emit:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [_IsDeadCode]
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	JMP	.L0
.L2:
.L3:
	MOV	AX, [_PendingPushAx]
	AND	AX, AX
	JNZ	.L4
	JMP	.L5
.L4:
	JMP	.L8
.L7:	DB 9, 'PUSH', 9, 'AX', 10, 0
.L8:
	ADD	SP, -8
	MOV	AX, .L7
	MOV	[BP-8], AX
	CALL	_OutputStr
	ADD	SP, 8
	MOV	WORD [_PendingPushAx], 0
	JMP	.L6
.L5:
.L6:
	PUSH	AX	; [BP-2] = vl
	PUSH	AX	; [BP-4] = dest
	MOV	AX, [_LineBuf]
	MOV	[BP-4], AX
	MOV	AX, [BP-4]
	PUSH	AX
	INC	AX
	MOV	[BP-4], AX
	POP	AX
	MOV	BX, AX
	MOV	BYTE [BX], 9
	LEA	AX, [BP+4]
	MOV	[BP-2], AX
	ADD	SP, -8
	MOV	AX, [BP-4]
	MOV	[BP-12], AX
	MOV	AX, [BP+4]
	MOV	[BP-10], AX
	MOV	AX, [BP-2]
	MOV	[BP-8], AX
	CALL	_VSPrintf
	ADD	SP, 8
	MOV	[BP-4], AX
	MOV	AX, [BP-4]
	PUSH	AX
	INC	AX
	MOV	[BP-4], AX
	POP	AX
	MOV	BX, AX
	MOV	BYTE [BX], 10
	MOV	AX, [BP-4]
	PUSH	AX
	INC	AX
	MOV	[BP-4], AX
	POP	AX
	MOV	BX, AX
	MOV	BYTE [BX], 0
	ADD	SP, -8
	MOV	AX, [_LineBuf]
	MOV	[BP-12], AX
	CALL	_OutputStr
	ADD	SP, 8
	ADD	SP, 4
.L0:
	POP	BP
	RET
_EmitLocalLabel:
	PUSH	BP
	MOV	BP, SP
	JMP	.L2
.L1:	DB '.L%d:', 10, 0
.L2:
	ADD	SP, -8
	MOV	AX, .L1
	MOV	[BP-8], AX
	MOV	AX, [BP+4]
	MOV	[BP-6], AX
	CALL	_RawEmit
	ADD	SP, 8
	MOV	WORD [_IsDeadCode], 0
.L0:
	POP	BP
	RET
_EmitGlobalLabel:
	PUSH	BP
	MOV	BP, SP
	JMP	.L2
.L1:	DB '_%s:', 10, 0
.L2:
	ADD	SP, -8
	MOV	AX, .L1
	MOV	[BP-8], AX
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-16], AX
	CALL	_IdText
	ADD	SP, 8
	MOV	[BP-6], AX
	CALL	_RawEmit
	ADD	SP, 8
	MOV	WORD [_IsDeadCode], 0
.L0:
	POP	BP
	RET
_EmitJmp:
	PUSH	BP
	MOV	BP, SP
	MOV	WORD [_PendingPushAx], 0
	MOV	AX, [_IsDeadCode]
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	JMP	.L0
.L2:
.L3:
	JMP	.L5
.L4:	DB 'JMP', 9, '.L%d', 0
.L5:
	ADD	SP, -8
	MOV	AX, .L4
	MOV	[BP-8], AX
	MOV	AX, [BP+4]
	MOV	[BP-6], AX
	CALL	_Emit
	ADD	SP, 8
	MOV	WORD [_IsDeadCode], 1
.L0:
	POP	BP
	RET
_EmitLoadAx:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	CMP	AX, 1
	JZ	.L1
	JMP	.L2
.L1:
	MOV	WORD [BP+4], 76
	JMP	.L3
.L2:
	MOV	WORD [BP+4], 88
.L3:
	MOV	AX, [BP+6]
	CMP	AX, 128
	JZ	.L4
	JMP	.L5
.L4:
	JMP	.L8
.L7:	DB 'MOV', 9, 'A%c, [BP%+d]', 0
.L8:
	ADD	SP, -8
	MOV	AX, .L7
	MOV	[BP-8], AX
	MOV	AX, [BP+4]
	MOV	[BP-6], AX
	MOV	AX, [BP+8]
	MOV	[BP-4], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L6
.L5:
	MOV	AX, [BP+6]
	CMP	AX, 192
	JZ	.L9
	JMP	.L10
.L9:
	JMP	.L13
.L12:	DB 'MOV', 9, 'A%c, [_%s]', 0
.L13:
	ADD	SP, -8
	MOV	AX, .L12
	MOV	[BP-8], AX
	MOV	AX, [BP+4]
	MOV	[BP-6], AX
	ADD	SP, -8
	MOV	AX, [BP+8]
	MOV	[BP-16], AX
	CALL	_IdText
	ADD	SP, 8
	MOV	[BP-4], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L11
.L10:
	JMP	.L15
.L14:	DB 'MOV', 9, 'A%c, [BX]', 0
.L15:
	ADD	SP, -8
	MOV	AX, .L14
	MOV	[BP-8], AX
	MOV	AX, [BP+4]
	MOV	[BP-6], AX
	CALL	_Emit
	ADD	SP, 8
.L11:
.L6:
	MOV	AX, [BP+4]
	CMP	AX, 76
	JZ	.L16
	JMP	.L17
.L16:
	JMP	.L20
.L19:	DB 'CBW', 0
.L20:
	ADD	SP, -8
	MOV	AX, .L19
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L18
.L17:
.L18:
.L0:
	POP	BP
	RET
_EmitStoreAx:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	CMP	AX, 1
	JZ	.L1
	JMP	.L2
.L1:
	MOV	WORD [BP+4], 76
	JMP	.L3
.L2:
	MOV	WORD [BP+4], 88
.L3:
	MOV	AX, [BP+6]
	CMP	AX, 128
	JZ	.L4
	JMP	.L5
.L4:
	JMP	.L8
.L7:	DB 'MOV', 9, '[BP%+d], A%c', 0
.L8:
	ADD	SP, -8
	MOV	AX, .L7
	MOV	[BP-8], AX
	MOV	AX, [BP+8]
	MOV	[BP-6], AX
	MOV	AX, [BP+4]
	MOV	[BP-4], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L6
.L5:
	MOV	AX, [BP+6]
	CMP	AX, 192
	JZ	.L9
	JMP	.L10
.L9:
	JMP	.L13
.L12:	DB 'MOV', 9, '[_%s], A%c', 0
.L13:
	ADD	SP, -8
	MOV	AX, .L12
	MOV	[BP-8], AX
	ADD	SP, -8
	MOV	AX, [BP+8]
	MOV	[BP-16], AX
	CALL	_IdText
	ADD	SP, 8
	MOV	[BP-6], AX
	MOV	AX, [BP+4]
	MOV	[BP-4], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L11
.L10:
	ADD	SP, -8
	MOV	AX, [BP+6]
	AND	AX, AX
	MOV	AX, 1
	JZ	.L15
	DEC	AL
.L15:
	MOV	[BP-8], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L17
.L16:	DB 'MOV', 9, '[BX], A%c', 0
.L17:
	ADD	SP, -8
	MOV	AX, .L16
	MOV	[BP-8], AX
	MOV	AX, [BP+4]
	MOV	[BP-6], AX
	CALL	_Emit
	ADD	SP, 8
.L11:
.L6:
.L0:
	POP	BP
	RET
_EmitStoreConst:
	PUSH	BP
	MOV	BP, SP
	JMP	.L2
.L1:	DB 'WORD', 0
.L2:
	MOV	AX, .L1
	PUSH	AX	; [BP-2] = SizeText
	MOV	AX, [BP+4]
	CMP	AX, 1
	JZ	.L3
	JMP	.L4
.L3:
	JMP	.L7
.L6:	DB 'BYTE', 0
.L7:
	MOV	AX, .L6
	MOV	[BP-2], AX
	JMP	.L5
.L4:
.L5:
	MOV	AX, [BP+6]
	CMP	AX, 128
	JZ	.L8
	JMP	.L9
.L8:
	JMP	.L12
.L11:	DB 'MOV', 9, '%s [BP%+d], %d', 0
.L12:
	ADD	SP, -8
	MOV	AX, .L11
	MOV	[BP-10], AX
	MOV	AX, [BP-2]
	MOV	[BP-8], AX
	MOV	AX, [BP+8]
	MOV	[BP-6], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-4], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L10
.L9:
	MOV	AX, [BP+6]
	CMP	AX, 192
	JZ	.L13
	JMP	.L14
.L13:
	JMP	.L17
.L16:	DB 'MOV', 9, '%s [_%s], %d', 0
.L17:
	ADD	SP, -8
	MOV	AX, .L16
	MOV	[BP-10], AX
	MOV	AX, [BP-2]
	MOV	[BP-8], AX
	ADD	SP, -8
	MOV	AX, [BP+8]
	MOV	[BP-18], AX
	CALL	_IdText
	ADD	SP, 8
	MOV	[BP-6], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-4], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L15
.L14:
	ADD	SP, -8
	MOV	AX, [BP+6]
	AND	AX, AX
	MOV	AX, 1
	JZ	.L19
	DEC	AL
.L19:
	MOV	[BP-10], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L21
.L20:	DB 'MOV', 9, '%s [BX], %d', 0
.L21:
	ADD	SP, -8
	MOV	AX, .L20
	MOV	[BP-10], AX
	MOV	AX, [BP-2]
	MOV	[BP-8], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-6], AX
	CALL	_Emit
	ADD	SP, 8
.L15:
.L10:
	ADD	SP, 2
.L0:
	POP	BP
	RET
_EmitAdjSp:
	PUSH	BP
	MOV	BP, SP
	JMP	.L2
.L1:	DB 'ADD', 9, 'SP, %d', 0
.L2:
	ADD	SP, -8
	MOV	AX, .L1
	MOV	[BP-8], AX
	MOV	AX, [BP+4]
	MOV	[BP-6], AX
	CALL	_Emit
	ADD	SP, 8
.L0:
	POP	BP
	RET
_GetChar:
	PUSH	BP
	MOV	BP, SP
	MOV	AL, [_UngottenChar]
	CBW
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	PUSH	AX	; [BP-2] = ch
	MOV	AL, [_UngottenChar]
	CBW
	MOV	[BP-2], AL
	MOV	BYTE [_UngottenChar], 0
	MOV	AL, [BP-2]
	CBW
	JMP	.L0
.L2:
.L3:
	MOV	AX, [_InBufCnt]
	PUSH	AX
	MOV	AX, [_InBufSize]
	POP	CX
	XCHG	AX, CX
	CMP	AX, CX
	JZ	.L4
	JMP	.L5
.L4:
	MOV	WORD [_InBufCnt], 0
	ADD	SP, -8
	MOV	AX, [_InFile]
	MOV	[BP-8], AX
	MOV	AX, [_InBuf]
	MOV	[BP-6], AX
	MOV	WORD [BP-4], 1024
	CALL	_read
	ADD	SP, 8
	MOV	[_InBufSize], AX
	MOV	AX, [_InBufSize]
	AND	AX, AX
	JZ	.L7
	JMP	.L8
.L7:
	MOV	AX, 0
	JMP	.L0
.L8:
.L9:
	JMP	.L6
.L5:
.L6:
	MOV	AX, [_InBuf]
	PUSH	AX
	MOV	AX, [_InBufCnt]
	PUSH	AX
	INC	AX
	MOV	[_InBufCnt], AX
	POP	AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_UnGetChar:
	PUSH	BP
	MOV	BP, SP
	MOV	AL, [BP+4]
	CBW
	MOV	[_UngottenChar], AL
.L0:
	POP	BP
	RET
_TryGetChar:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ch2
	CALL	_GetChar
	MOV	[BP-2], AL
	PUSH	AX
	MOV	AL, [BP+4]
	CBW
	POP	CX
	CMP	AX, CX
	JNZ	.L1
	JMP	.L2
.L1:
	ADD	SP, -8
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-10], AX
	CALL	_UnGetChar
	ADD	SP, 8
	MOV	AX, 0
	JMP	.L0
.L2:
.L3:
	MOV	AX, 1
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_SkipLine:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ch
.L1:
	CALL	_GetChar
	MOV	[BP-2], AX
	CMP	AX, 10
	JNZ	.L4
	MOV	AX, 0
	JMP	.L5
.L4:
	MOV	AX, [BP-2]
.L5:
	AND	AX, AX
	JNZ	.L2
	JMP	.L3
.L2:
	JMP	.L1
.L3:
	MOV	AX, [_Line]
	INC	AX
	MOV	[_Line], AX
	ADD	SP, 2
.L0:
	POP	BP
	RET
_SkipWhitespace:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ch
.L1:
	CALL	_GetChar
	MOV	[BP-2], AL
	CMP	AX, 32
	JNG	.L2
	JMP	.L3
.L2:
	MOV	AL, [BP-2]
	CBW
	AND	AX, AX
	JZ	.L4
	JMP	.L5
.L4:
	JMP	.L0
.L5:
.L6:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 10
	JZ	.L8
	JMP	.L9
.L8:
	MOV	AX, [_Line]
	INC	AX
	MOV	[_Line], AX
	JMP	.L10
.L9:
.L10:
	JMP	.L1
.L3:
	ADD	SP, -8
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-10], AX
	CALL	_UnGetChar
	ADD	SP, 8
	ADD	SP, 2
.L0:
	MOV	SP, BP
	POP	BP
	RET
_Unescape:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ch
	CALL	_GetChar
	MOV	[BP-2], AL
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 110
	JZ	.L1
	JMP	.L2
.L1:
	MOV	AX, 10
	JMP	.L0
.L2:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 114
	JZ	.L4
	JMP	.L5
.L4:
	MOV	AX, 13
	JMP	.L0
.L5:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 116
	JZ	.L7
	JMP	.L8
.L7:
	MOV	AX, 9
	JMP	.L0
.L8:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 39
	JZ	.L10
	JMP	.L11
.L10:
	MOV	AX, 39
	JMP	.L0
.L11:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 34
	JZ	.L13
	JMP	.L14
.L13:
	MOV	AX, 34
	JMP	.L0
.L14:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 92
	JZ	.L16
	JMP	.L17
.L16:
	MOV	AX, 92
	JMP	.L0
.L17:
	JMP	.L20
.L19:	DB 'TODO: Escaped character literal \%c', 10, 0
.L20:
	ADD	SP, -8
	MOV	AX, .L19
	MOV	[BP-10], AX
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-8], AX
	CALL	_Printf
	ADD	SP, 8
	JMP	.L22
.L21:	DB 'Unsupported character literal', 0
.L22:
	ADD	SP, -8
	MOV	AX, .L21
	MOV	[BP-10], AX
	CALL	_Fatal
	ADD	SP, 8
.L18:
.L15:
.L12:
.L9:
.L6:
.L3:
	ADD	SP, 2
.L0:
	MOV	SP, BP
	POP	BP
	RET
_GetStringLiteral:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	MOV	[_TokenNumVal], AX
	MOV	AX, [_IsDeadCode]
	PUSH	AX	; [BP-2] = WasDeadCode
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	PUSH	AX	; [BP-4] = JL
	ADD	SP, -8
	MOV	AX, [BP-4]
	MOV	[BP-12], AX
	CALL	_EmitJmp
	ADD	SP, 8
	JMP	.L2
.L1:	DB '.L%d:', 9, 'DB ', 0
.L2:
	ADD	SP, -8
	MOV	AX, .L1
	MOV	[BP-12], AX
	MOV	AX, [_TokenNumVal]
	MOV	[BP-10], AX
	CALL	_RawEmit
	ADD	SP, 8
	MOV	AX, [BP-2]
	MOV	[_IsDeadCode], AX
	PUSH	AX	; [BP-6] = ch
	MOV	AX, 0
	PUSH	AX	; [BP-8] = open
.L3:
	JMP	.L4
.L4:
.L6:
	CALL	_GetChar
	MOV	[BP-6], AL
	CMP	AX, 34
	JNZ	.L7
	JMP	.L8
.L7:
	MOV	AL, [BP-6]
	CBW
	CMP	AX, 92
	JZ	.L9
	JMP	.L10
.L9:
	CALL	_Unescape
	MOV	[BP-6], AL
	JMP	.L11
.L10:
.L11:
	MOV	AL, [BP-6]
	CBW
	AND	AX, AX
	JZ	.L12
	JMP	.L13
.L12:
	JMP	.L17
.L16:	DB 'Unterminated string literal', 0
.L17:
	ADD	SP, -8
	MOV	AX, .L16
	MOV	[BP-16], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L14
.L13:
.L14:
	MOV	AL, [BP-6]
	CBW
	CMP	AX, 32
	JNL	.L21
	MOV	AX, 1
	JMP	.L22
.L21:
	MOV	AL, [BP-6]
	CBW
	CMP	AX, 39
	MOV	AX, 1
	JZ	.L23
	DEC	AL
.L23:
.L22:
	AND	AX, AX
	JNZ	.L18
	JMP	.L19
.L18:
	MOV	AX, [BP-8]
	AND	AX, AX
	JNZ	.L24
	JMP	.L25
.L24:
	JMP	.L28
.L27:	DB 39, ', ', 0
.L28:
	ADD	SP, -8
	MOV	AX, .L27
	MOV	[BP-16], AX
	CALL	_RawEmit
	ADD	SP, 8
	MOV	WORD [BP-8], 0
	JMP	.L26
.L25:
.L26:
	JMP	.L30
.L29:	DB '%d, ', 0
.L30:
	ADD	SP, -8
	MOV	AX, .L29
	MOV	[BP-16], AX
	MOV	AL, [BP-6]
	CBW
	MOV	[BP-14], AX
	CALL	_RawEmit
	ADD	SP, 8
	JMP	.L20
.L19:
	MOV	AX, [BP-8]
	AND	AX, AX
	JZ	.L31
	JMP	.L32
.L31:
	MOV	WORD [BP-8], 1
	JMP	.L36
.L35:	DB 39, 0
.L36:
	ADD	SP, -8
	MOV	AX, .L35
	MOV	[BP-16], AX
	CALL	_RawEmit
	ADD	SP, 8
	JMP	.L33
.L32:
.L33:
	JMP	.L38
.L37:	DB '%c', 0
.L38:
	ADD	SP, -8
	MOV	AX, .L37
	MOV	[BP-16], AX
	MOV	AL, [BP-6]
	CBW
	MOV	[BP-14], AX
	CALL	_RawEmit
	ADD	SP, 8
.L20:
	JMP	.L6
.L8:
	CALL	_SkipWhitespace
	CALL	_GetChar
	MOV	[BP-6], AL
	MOV	AL, [BP-6]
	CBW
	CMP	AX, 34
	JNZ	.L39
	JMP	.L40
.L39:
	ADD	SP, -8
	MOV	AL, [BP-6]
	CBW
	MOV	[BP-16], AX
	CALL	_UnGetChar
	ADD	SP, 8
	JMP	.L5
.L40:
.L41:
	JMP	.L3
.L5:
	MOV	AX, [BP-8]
	AND	AX, AX
	JNZ	.L42
	JMP	.L43
.L42:
	JMP	.L46
.L45:	DB 39, ', ', 0
.L46:
	ADD	SP, -8
	MOV	AX, .L45
	MOV	[BP-16], AX
	CALL	_RawEmit
	ADD	SP, 8
	JMP	.L44
.L43:
.L44:
	JMP	.L48
.L47:	DB '0', 10, 0
.L48:
	ADD	SP, -8
	MOV	AX, .L47
	MOV	[BP-16], AX
	CALL	_RawEmit
	ADD	SP, 8
	JMP	.L50
.L49:	DB '.L%d:', 10, 0
.L50:
	ADD	SP, -8
	MOV	AX, .L49
	MOV	[BP-16], AX
	MOV	AX, [BP-4]
	MOV	[BP-14], AX
	CALL	_RawEmit
	ADD	SP, 8
	ADD	SP, 8
.L0:
	POP	BP
	RET
_GetToken:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ch
	CALL	_SkipWhitespace
	CALL	_GetChar
	MOV	[BP-2], AL
	MOV	AL, [BP-2]
	CBW
	AND	AX, AX
	JZ	.L1
	JMP	.L2
.L1:
	MOV	WORD [_TokenType], 0
	JMP	.L0
.L2:
.L3:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 35
	JZ	.L5
	JMP	.L6
.L5:
	CALL	_SkipLine
	CALL	_GetToken
	JMP	.L0
.L6:
	ADD	SP, -8
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-10], AX
	CALL	_IsDigit
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L8
	JMP	.L9
.L8:
	MOV	WORD [_TokenNumVal], 0
	MOV	AX, 10
	PUSH	AX	; [BP-4] = base
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 48
	JZ	.L11
	JMP	.L12
.L11:
	MOV	WORD [BP-4], 8
	CALL	_GetChar
	MOV	[BP-2], AL
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 120
	JNZ	.L17
	MOV	AX, 1
	JMP	.L18
.L17:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 88
	MOV	AX, 1
	JZ	.L19
	DEC	AL
.L19:
.L18:
	AND	AX, AX
	JNZ	.L14
	JMP	.L15
.L14:
	MOV	WORD [BP-4], 16
	CALL	_GetChar
	MOV	[BP-2], AL
	JMP	.L16
.L15:
.L16:
	JMP	.L13
.L12:
.L13:
.L20:
	JMP	.L21
.L21:
	ADD	SP, -8
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-12], AX
	CALL	_IsDigit
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L23
	JMP	.L24
.L23:
	MOV	AX, [BP-2]
	SUB	AX, 48
	MOV	[BP-2], AL
	JMP	.L25
.L24:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 65
	JNL	.L29
	MOV	AX, 0
	JMP	.L30
.L29:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 70
	MOV	AX, 1
	JNG	.L31
	DEC	AL
.L31:
.L30:
	AND	AX, AX
	JNZ	.L26
	JMP	.L27
.L26:
	MOV	AX, [BP-2]
	SUB	AX, 55
	MOV	[BP-2], AL
	JMP	.L28
.L27:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 97
	JNL	.L35
	MOV	AX, 0
	JMP	.L36
.L35:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 102
	MOV	AX, 1
	JNG	.L37
	DEC	AL
.L37:
.L36:
	AND	AX, AX
	JNZ	.L32
	JMP	.L33
.L32:
	MOV	AX, [BP-2]
	SUB	AX, 87
	MOV	[BP-2], AL
	JMP	.L34
.L33:
.L34:
.L28:
.L25:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 0
	JNL	.L41
	MOV	AX, 1
	JMP	.L42
.L41:
	MOV	AL, [BP-2]
	CBW
	PUSH	AX
	MOV	AX, [BP-4]
	POP	CX
	XCHG	AX, CX
	CMP	AX, CX
	MOV	AX, 1
	JNL	.L43
	DEC	AL
.L43:
.L42:
	AND	AX, AX
	JNZ	.L38
	JMP	.L39
.L38:
	JMP	.L22
.L39:
.L40:
	MOV	AX, [_TokenNumVal]
	PUSH	AX
	MOV	AX, [BP-4]
	POP	CX
	IMUL	CX
	PUSH	AX
	MOV	AL, [BP-2]
	CBW
	POP	CX
	ADD	AX, CX
	MOV	[_TokenNumVal], AX
	CALL	_GetChar
	MOV	[BP-2], AL
	JMP	.L20
.L22:
	ADD	SP, -8
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-12], AX
	CALL	_UnGetChar
	ADD	SP, 8
	MOV	WORD [_TokenType], 1
	ADD	SP, 2
	JMP	.L10
.L9:
	ADD	SP, -8
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-10], AX
	CALL	_IsAlpha
	ADD	SP, 8
	AND	AX, AX
	JZ	.L47
	JMP	.L48
.L47:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 95
	MOV	AX, 1
	JZ	.L49
	DEC	AL
.L49:
.L48:
	AND	AX, AX
	JNZ	.L44
	JMP	.L45
.L44:
	PUSH	AX	; [BP-4] = id
	PUSH	AX	; [BP-6] = pc
	PUSH	AX	; [BP-8] = start
	MOV	AX, [_IdBuffer]
	PUSH	AX
	MOV	AX, [_IdBufferIndex]
	POP	CX
	ADD	AX, CX
	MOV	[BP-6], AX
	MOV	[BP-8], AX
	MOV	AX, [BP-6]
	PUSH	AX
	INC	AX
	MOV	[BP-6], AX
	POP	AX
	PUSH	AX
	MOV	AL, [BP-2]
	CBW
	POP	BX
	MOV	[BX], AL
.L50:
	JMP	.L51
.L51:
	CALL	_GetChar
	MOV	[BP-2], AL
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 95
	JNZ	.L56
	MOV	AX, 0
	JMP	.L57
.L56:
	ADD	SP, -8
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-16], AX
	CALL	_IsDigit
	ADD	SP, 8
	AND	AX, AX
	MOV	AX, 1
	JZ	.L59
	DEC	AL
.L59:
.L57:
	AND	AX, AX
	JNZ	.L60
	JMP	.L61
.L60:
	ADD	SP, -8
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-16], AX
	CALL	_IsAlpha
	ADD	SP, 8
	AND	AX, AX
	MOV	AX, 1
	JZ	.L63
	DEC	AL
.L63:
.L61:
	AND	AX, AX
	JNZ	.L53
	JMP	.L54
.L53:
	JMP	.L52
.L54:
.L55:
	MOV	AX, [BP-6]
	PUSH	AX
	INC	AX
	MOV	[BP-6], AX
	POP	AX
	PUSH	AX
	MOV	AL, [BP-2]
	CBW
	POP	BX
	MOV	[BX], AL
	JMP	.L50
.L52:
	MOV	AX, [BP-6]
	PUSH	AX
	INC	AX
	MOV	[BP-6], AX
	POP	AX
	MOV	BX, AX
	MOV	BYTE [BX], 0
	ADD	SP, -8
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-16], AX
	CALL	_UnGetChar
	ADD	SP, 8
	MOV	WORD [_TokenType], -1
	MOV	WORD [BP-4], 0
.L64:
	MOV	AX, [BP-4]
	PUSH	AX
	MOV	AX, [_IdCount]
	POP	CX
	XCHG	AX, CX
	CMP	AX, CX
	JL	.L65
	JMP	.L66
.L67:
	MOV	AX, [BP-4]
	INC	AX
	MOV	[BP-4], AX
	JMP	.L64
.L65:
	ADD	SP, -8
	ADD	SP, -8
	MOV	AX, [BP-4]
	MOV	[BP-24], AX
	CALL	_IdText
	ADD	SP, 8
	MOV	[BP-16], AX
	MOV	AX, [BP-8]
	MOV	[BP-14], AX
	CALL	_StrEqual
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L68
	JMP	.L69
.L68:
	MOV	AX, [BP-4]
	MOV	[_TokenType], AX
	JMP	.L66
.L69:
.L70:
	JMP	.L67
.L66:
	MOV	AX, [_TokenType]
	CMP	AX, 0
	JL	.L71
	JMP	.L72
.L71:
	ADD	SP, -8
	MOV	AX, [_IdCount]
	CMP	AX, 350
	MOV	AX, 1
	JL	.L74
	DEC	AL
.L74:
	MOV	[BP-16], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_IdCount]
	PUSH	AX
	INC	AX
	MOV	[_IdCount], AX
	POP	AX
	MOV	[_TokenType], AX
	MOV	AX, [_IdOffset]
	PUSH	AX
	MOV	AX, [_TokenType]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	PUSH	AX
	MOV	AX, [_IdBufferIndex]
	POP	BX
	MOV	[BX], AX
	MOV	AX, [BP-6]
	PUSH	AX
	MOV	AX, [BP-8]
	POP	CX
	XCHG	AX, CX
	SUB	AX, CX
	MOV	CX, AX
	MOV	AX, [_IdBufferIndex]
	ADD	AX, CX
	MOV	[_IdBufferIndex], AX
	JMP	.L73
.L72:
.L73:
	MOV	AX, [_TokenType]
	ADD	AX, 46
	MOV	[_TokenType], AX
	ADD	SP, 6
	JMP	.L46
.L45:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 39
	JZ	.L75
	JMP	.L76
.L75:
	CALL	_GetChar
	MOV	[BP-2], AL
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 92
	JZ	.L78
	JMP	.L79
.L78:
	CALL	_Unescape
	MOV	[_TokenNumVal], AX
	JMP	.L80
.L79:
	MOV	AL, [BP-2]
	CBW
	MOV	[_TokenNumVal], AX
.L80:
	CALL	_GetChar
	CMP	AX, 39
	JNZ	.L81
	JMP	.L82
.L81:
	JMP	.L85
.L84:	DB 'Invalid character literal', 0
.L85:
	ADD	SP, -8
	MOV	AX, .L84
	MOV	[BP-10], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L83
.L82:
.L83:
	MOV	WORD [_TokenType], 1
	JMP	.L77
.L76:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 34
	JZ	.L86
	JMP	.L87
.L86:
	CALL	_GetStringLiteral
	MOV	WORD [_TokenType], 2
	JMP	.L88
.L87:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 40
	JZ	.L89
	JMP	.L90
.L89:
	MOV	WORD [_TokenType], 3
	JMP	.L91
.L90:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 41
	JZ	.L92
	JMP	.L93
.L92:
	MOV	WORD [_TokenType], 4
	JMP	.L94
.L93:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 123
	JZ	.L95
	JMP	.L96
.L95:
	MOV	WORD [_TokenType], 5
	JMP	.L97
.L96:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 125
	JZ	.L98
	JMP	.L99
.L98:
	MOV	WORD [_TokenType], 6
	JMP	.L100
.L99:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 91
	JZ	.L101
	JMP	.L102
.L101:
	MOV	WORD [_TokenType], 7
	JMP	.L103
.L102:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 93
	JZ	.L104
	JMP	.L105
.L104:
	MOV	WORD [_TokenType], 8
	JMP	.L106
.L105:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 44
	JZ	.L107
	JMP	.L108
.L107:
	MOV	WORD [_TokenType], 9
	JMP	.L109
.L108:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 59
	JZ	.L110
	JMP	.L111
.L110:
	MOV	WORD [_TokenType], 10
	JMP	.L112
.L111:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 126
	JZ	.L113
	JMP	.L114
.L113:
	MOV	WORD [_TokenType], 43
	JMP	.L115
.L114:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 61
	JZ	.L116
	JMP	.L117
.L116:
	MOV	WORD [_TokenType], 11
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L119
	JMP	.L120
.L119:
	MOV	WORD [_TokenType], 12
	JMP	.L121
.L120:
.L121:
	JMP	.L118
.L117:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 33
	JZ	.L122
	JMP	.L123
.L122:
	MOV	WORD [_TokenType], 13
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L125
	JMP	.L126
.L125:
	MOV	WORD [_TokenType], 14
	JMP	.L127
.L126:
.L127:
	JMP	.L124
.L123:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 60
	JZ	.L128
	JMP	.L129
.L128:
	MOV	WORD [_TokenType], 15
	ADD	SP, -8
	MOV	WORD [BP-10], 60
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L131
	JMP	.L132
.L131:
	MOV	WORD [_TokenType], 31
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L134
	JMP	.L135
.L134:
	MOV	WORD [_TokenType], 38
	JMP	.L136
.L135:
.L136:
	JMP	.L133
.L132:
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L137
	JMP	.L138
.L137:
	MOV	WORD [_TokenType], 16
	JMP	.L139
.L138:
.L139:
.L133:
	JMP	.L130
.L129:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 62
	JZ	.L140
	JMP	.L141
.L140:
	MOV	WORD [_TokenType], 17
	ADD	SP, -8
	MOV	WORD [BP-10], 62
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L143
	JMP	.L144
.L143:
	MOV	WORD [_TokenType], 32
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L146
	JMP	.L147
.L146:
	MOV	WORD [_TokenType], 39
	JMP	.L148
.L147:
.L148:
	JMP	.L145
.L144:
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L149
	JMP	.L150
.L149:
	MOV	WORD [_TokenType], 18
	JMP	.L151
.L150:
.L151:
.L145:
	JMP	.L142
.L141:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 38
	JZ	.L152
	JMP	.L153
.L152:
	MOV	WORD [_TokenType], 26
	ADD	SP, -8
	MOV	WORD [BP-10], 38
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L155
	JMP	.L156
.L155:
	MOV	WORD [_TokenType], 29
	JMP	.L157
.L156:
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L158
	JMP	.L159
.L158:
	MOV	WORD [_TokenType], 40
	JMP	.L160
.L159:
.L160:
.L157:
	JMP	.L154
.L153:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 124
	JZ	.L161
	JMP	.L162
.L161:
	MOV	WORD [_TokenType], 28
	ADD	SP, -8
	MOV	WORD [BP-10], 124
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L164
	JMP	.L165
.L164:
	MOV	WORD [_TokenType], 30
	JMP	.L166
.L165:
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L167
	JMP	.L168
.L167:
	MOV	WORD [_TokenType], 42
	JMP	.L169
.L168:
.L169:
.L166:
	JMP	.L163
.L162:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 94
	JZ	.L170
	JMP	.L171
.L170:
	MOV	WORD [_TokenType], 27
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L173
	JMP	.L174
.L173:
	MOV	WORD [_TokenType], 41
	JMP	.L175
.L174:
.L175:
	JMP	.L172
.L171:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 43
	JZ	.L176
	JMP	.L177
.L176:
	MOV	WORD [_TokenType], 19
	ADD	SP, -8
	MOV	WORD [BP-10], 43
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L179
	JMP	.L180
.L179:
	MOV	WORD [_TokenType], 20
	JMP	.L181
.L180:
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L182
	JMP	.L183
.L182:
	MOV	WORD [_TokenType], 33
	JMP	.L184
.L183:
.L184:
.L181:
	JMP	.L178
.L177:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 45
	JZ	.L185
	JMP	.L186
.L185:
	MOV	WORD [_TokenType], 21
	ADD	SP, -8
	MOV	WORD [BP-10], 45
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L188
	JMP	.L189
.L188:
	MOV	WORD [_TokenType], 22
	JMP	.L190
.L189:
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L191
	JMP	.L192
.L191:
	MOV	WORD [_TokenType], 34
	JMP	.L193
.L192:
.L193:
.L190:
	JMP	.L187
.L186:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 42
	JZ	.L194
	JMP	.L195
.L194:
	MOV	WORD [_TokenType], 23
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L197
	JMP	.L198
.L197:
	MOV	WORD [_TokenType], 35
	JMP	.L199
.L198:
.L199:
	JMP	.L196
.L195:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 47
	JZ	.L200
	JMP	.L201
.L200:
	ADD	SP, -8
	MOV	WORD [BP-10], 47
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L203
	JMP	.L204
.L203:
	CALL	_SkipLine
	CALL	_GetToken
	JMP	.L0
.L204:
.L205:
	MOV	WORD [_TokenType], 24
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L206
	JMP	.L207
.L206:
	MOV	WORD [_TokenType], 36
	JMP	.L208
.L207:
.L208:
	JMP	.L202
.L201:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 37
	JZ	.L209
	JMP	.L210
.L209:
	MOV	WORD [_TokenType], 25
	ADD	SP, -8
	MOV	WORD [BP-10], 61
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L212
	JMP	.L213
.L212:
	MOV	WORD [_TokenType], 37
	JMP	.L214
.L213:
.L214:
	JMP	.L211
.L210:
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 46
	JZ	.L215
	JMP	.L216
.L215:
	MOV	WORD [_TokenType], 44
	ADD	SP, -8
	MOV	WORD [BP-10], 46
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L218
	JMP	.L219
.L218:
	ADD	SP, -8
	MOV	WORD [BP-10], 46
	CALL	_TryGetChar
	ADD	SP, 8
	AND	AX, AX
	JZ	.L221
	JMP	.L222
.L221:
	JMP	.L226
.L225:	DB 'Invalid token ..', 0
.L226:
	ADD	SP, -8
	MOV	AX, .L225
	MOV	[BP-10], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L223
.L222:
.L223:
	MOV	WORD [_TokenType], 45
	JMP	.L220
.L219:
.L220:
	JMP	.L217
.L216:
	JMP	.L228
.L227:	DB 'Unknown token start ', 39, '%c', 39, ' (%d)', 10, 0
.L228:
	ADD	SP, -8
	MOV	AX, .L227
	MOV	[BP-10], AX
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-8], AX
	MOV	AL, [BP-2]
	CBW
	MOV	[BP-6], AX
	CALL	_Printf
	ADD	SP, 8
	JMP	.L230
.L229:	DB 'Unknown token encountered', 0
.L230:
	ADD	SP, -8
	MOV	AX, .L229
	MOV	[BP-10], AX
	CALL	_Fatal
	ADD	SP, 8
.L217:
.L211:
.L202:
.L196:
.L187:
.L178:
.L172:
.L163:
.L154:
.L142:
.L130:
.L124:
.L118:
.L115:
.L112:
.L109:
.L106:
.L103:
.L100:
.L97:
.L94:
.L91:
.L88:
.L77:
.L46:
.L10:
.L7:
	ADD	SP, 2
.L0:
	MOV	SP, BP
	POP	BP
	RET
_OperatorPrecedence:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	CMP	AX, 23
	JNZ	.L4
	MOV	AX, 1
	JMP	.L5
.L4:
	MOV	AX, [BP+4]
	CMP	AX, 24
	MOV	AX, 1
	JZ	.L6
	DEC	AL
.L6:
.L5:
	AND	AX, AX
	JZ	.L7
	JMP	.L8
.L7:
	MOV	AX, [BP+4]
	CMP	AX, 25
	MOV	AX, 1
	JZ	.L9
	DEC	AL
.L9:
.L8:
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	MOV	AX, 3
	JMP	.L0
.L2:
	MOV	AX, [BP+4]
	CMP	AX, 19
	JNZ	.L13
	MOV	AX, 1
	JMP	.L14
.L13:
	MOV	AX, [BP+4]
	CMP	AX, 21
	MOV	AX, 1
	JZ	.L15
	DEC	AL
.L15:
.L14:
	AND	AX, AX
	JNZ	.L10
	JMP	.L11
.L10:
	MOV	AX, 4
	JMP	.L0
.L11:
	MOV	AX, [BP+4]
	CMP	AX, 31
	JNZ	.L19
	MOV	AX, 1
	JMP	.L20
.L19:
	MOV	AX, [BP+4]
	CMP	AX, 32
	MOV	AX, 1
	JZ	.L21
	DEC	AL
.L21:
.L20:
	AND	AX, AX
	JNZ	.L16
	JMP	.L17
.L16:
	MOV	AX, 5
	JMP	.L0
.L17:
	MOV	AX, [BP+4]
	CMP	AX, 15
	JNZ	.L25
	MOV	AX, 1
	JMP	.L26
.L25:
	MOV	AX, [BP+4]
	CMP	AX, 16
	MOV	AX, 1
	JZ	.L27
	DEC	AL
.L27:
.L26:
	AND	AX, AX
	JZ	.L28
	JMP	.L29
.L28:
	MOV	AX, [BP+4]
	CMP	AX, 17
	MOV	AX, 1
	JZ	.L30
	DEC	AL
.L30:
.L29:
	AND	AX, AX
	JZ	.L31
	JMP	.L32
.L31:
	MOV	AX, [BP+4]
	CMP	AX, 18
	MOV	AX, 1
	JZ	.L33
	DEC	AL
.L33:
.L32:
	AND	AX, AX
	JNZ	.L22
	JMP	.L23
.L22:
	MOV	AX, 6
	JMP	.L0
.L23:
	MOV	AX, [BP+4]
	CMP	AX, 12
	JNZ	.L37
	MOV	AX, 1
	JMP	.L38
.L37:
	MOV	AX, [BP+4]
	CMP	AX, 14
	MOV	AX, 1
	JZ	.L39
	DEC	AL
.L39:
.L38:
	AND	AX, AX
	JNZ	.L34
	JMP	.L35
.L34:
	MOV	AX, 7
	JMP	.L0
.L35:
	MOV	AX, [BP+4]
	CMP	AX, 26
	JZ	.L40
	JMP	.L41
.L40:
	MOV	AX, 8
	JMP	.L0
.L41:
	MOV	AX, [BP+4]
	CMP	AX, 27
	JZ	.L43
	JMP	.L44
.L43:
	MOV	AX, 9
	JMP	.L0
.L44:
	MOV	AX, [BP+4]
	CMP	AX, 28
	JZ	.L46
	JMP	.L47
.L46:
	MOV	AX, 10
	JMP	.L0
.L47:
	MOV	AX, [BP+4]
	CMP	AX, 29
	JZ	.L49
	JMP	.L50
.L49:
	MOV	AX, 11
	JMP	.L0
.L50:
	MOV	AX, [BP+4]
	CMP	AX, 30
	JZ	.L52
	JMP	.L53
.L52:
	MOV	AX, 12
	JMP	.L0
.L53:
	MOV	AX, [BP+4]
	CMP	AX, 11
	JNZ	.L58
	MOV	AX, 1
	JMP	.L59
.L58:
	MOV	AX, [BP+4]
	CMP	AX, 33
	MOV	AX, 1
	JZ	.L60
	DEC	AL
.L60:
.L59:
	AND	AX, AX
	JZ	.L61
	JMP	.L62
.L61:
	MOV	AX, [BP+4]
	CMP	AX, 34
	MOV	AX, 1
	JZ	.L63
	DEC	AL
.L63:
.L62:
	AND	AX, AX
	JZ	.L64
	JMP	.L65
.L64:
	MOV	AX, [BP+4]
	CMP	AX, 35
	MOV	AX, 1
	JZ	.L66
	DEC	AL
.L66:
.L65:
	AND	AX, AX
	JZ	.L67
	JMP	.L68
.L67:
	MOV	AX, [BP+4]
	CMP	AX, 36
	MOV	AX, 1
	JZ	.L69
	DEC	AL
.L69:
.L68:
	AND	AX, AX
	JZ	.L70
	JMP	.L71
.L70:
	MOV	AX, [BP+4]
	CMP	AX, 37
	MOV	AX, 1
	JZ	.L72
	DEC	AL
.L72:
.L71:
	AND	AX, AX
	JZ	.L73
	JMP	.L74
.L73:
	MOV	AX, [BP+4]
	CMP	AX, 38
	MOV	AX, 1
	JZ	.L75
	DEC	AL
.L75:
.L74:
	AND	AX, AX
	JZ	.L76
	JMP	.L77
.L76:
	MOV	AX, [BP+4]
	CMP	AX, 39
	MOV	AX, 1
	JZ	.L78
	DEC	AL
.L78:
.L77:
	AND	AX, AX
	JZ	.L79
	JMP	.L80
.L79:
	MOV	AX, [BP+4]
	CMP	AX, 40
	MOV	AX, 1
	JZ	.L81
	DEC	AL
.L81:
.L80:
	AND	AX, AX
	JZ	.L82
	JMP	.L83
.L82:
	MOV	AX, [BP+4]
	CMP	AX, 41
	MOV	AX, 1
	JZ	.L84
	DEC	AL
.L84:
.L83:
	AND	AX, AX
	JZ	.L85
	JMP	.L86
.L85:
	MOV	AX, [BP+4]
	CMP	AX, 42
	MOV	AX, 1
	JZ	.L87
	DEC	AL
.L87:
.L86:
	AND	AX, AX
	JNZ	.L55
	JMP	.L56
.L55:
	MOV	AX, 14
	JMP	.L0
.L56:
	MOV	AX, [BP+4]
	CMP	AX, 9
	JZ	.L88
	JMP	.L89
.L88:
	MOV	AX, 15
	JMP	.L0
.L89:
	MOV	AX, 100
	JMP	.L0
.L90:
.L57:
.L54:
.L51:
.L48:
.L45:
.L42:
.L36:
.L24:
.L18:
.L12:
.L3:
.L0:
	POP	BP
	RET
_Unexpected:
	PUSH	BP
	MOV	BP, SP
	JMP	.L2
.L1:	DB 'token type %d', 10, 0
.L2:
	ADD	SP, -8
	MOV	AX, .L1
	MOV	[BP-8], AX
	MOV	AX, [_TokenType]
	MOV	[BP-6], AX
	CALL	_Printf
	ADD	SP, 8
	JMP	.L4
.L3:	DB 'Unexpected token', 0
.L4:
	ADD	SP, -8
	MOV	AX, .L3
	MOV	[BP-8], AX
	CALL	_Fatal
	ADD	SP, 8
.L0:
	POP	BP
	RET
_Accept:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [_TokenType]
	PUSH	AX
	MOV	AX, [BP+4]
	POP	CX
	XCHG	AX, CX
	CMP	AX, CX
	JZ	.L1
	JMP	.L2
.L1:
	CALL	_GetToken
	MOV	AX, 1
	JMP	.L0
.L2:
.L3:
	MOV	AX, 0
	JMP	.L0
.L0:
	POP	BP
	RET
_Expect:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-8], AX
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JZ	.L1
	JMP	.L2
.L1:
	JMP	.L6
.L5:	DB 'Token type %d expected got ', 0
.L6:
	ADD	SP, -8
	MOV	AX, .L5
	MOV	[BP-8], AX
	MOV	AX, [BP+4]
	MOV	[BP-6], AX
	CALL	_Printf
	ADD	SP, 8
	CALL	_Unexpected
	JMP	.L3
.L2:
.L3:
.L0:
	POP	BP
	RET
_ExpectId:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = id
	MOV	AX, [_TokenType]
	MOV	[BP-2], AX
	MOV	AX, [BP-2]
	CMP	AX, 63
	JNL	.L1
	JMP	.L2
.L1:
	CALL	_GetToken
	MOV	AX, [BP-2]
	SUB	AX, 46
	JMP	.L0
.L2:
.L3:
	JMP	.L5
.L4:	DB 'Expected identifier got ', 0
.L5:
	ADD	SP, -8
	MOV	AX, .L4
	MOV	[BP-10], AX
	CALL	_Printf
	ADD	SP, 8
	CALL	_Unexpected
	ADD	SP, 2
.L0:
	MOV	SP, BP
	POP	BP
	RET
_IsTypeStart:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [_TokenType]
	CMP	AX, 48
	JNZ	.L1
	MOV	AX, 1
	JMP	.L2
.L1:
	MOV	AX, [_TokenType]
	CMP	AX, 57
	MOV	AX, 1
	JZ	.L3
	DEC	AL
.L3:
.L2:
	AND	AX, AX
	JZ	.L4
	JMP	.L5
.L4:
	MOV	AX, [_TokenType]
	CMP	AX, 47
	MOV	AX, 1
	JZ	.L6
	DEC	AL
.L6:
.L5:
	AND	AX, AX
	JZ	.L7
	JMP	.L8
.L7:
	MOV	AX, [_TokenType]
	CMP	AX, 54
	MOV	AX, 1
	JZ	.L9
	DEC	AL
.L9:
.L8:
	AND	AX, AX
	JZ	.L10
	JMP	.L11
.L10:
	MOV	AX, [_TokenType]
	CMP	AX, 59
	MOV	AX, 1
	JZ	.L12
	DEC	AL
.L12:
.L11:
	JMP	.L0
.L0:
	POP	BP
	RET
_LvalToRval:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [_CurrentType]
	AND	AX, 4
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	MOV	AX, [_CurrentType]
	AND	AX, 192
	PUSH	AX	; [BP-2] = loc
	MOV	AX, [_CurrentType]
	AND	AX, -197
	MOV	[_CurrentType], AX
	MOV	AX, 2
	PUSH	AX	; [BP-4] = sz
	MOV	AX, [_CurrentType]
	CMP	AX, 2
	JZ	.L4
	JMP	.L5
.L4:
	MOV	WORD [BP-4], 1
	MOV	WORD [_CurrentType], 3
	JMP	.L6
.L5:
.L6:
	MOV	AX, [BP-2]
	AND	AX, AX
	JZ	.L7
	JMP	.L8
.L7:
	JMP	.L12
.L11:	DB 'MOV', 9, 'BX, AX', 0
.L12:
	ADD	SP, -8
	MOV	AX, .L11
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L9
.L8:
.L9:
	ADD	SP, -8
	MOV	AX, [BP-4]
	MOV	[BP-12], AX
	MOV	AX, [BP-2]
	MOV	[BP-10], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-8], AX
	CALL	_EmitLoadAx
	ADD	SP, 8
	ADD	SP, 4
	JMP	.L3
.L2:
.L3:
.L0:
	POP	BP
	RET
_DoIncDec:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = Amm
	MOV	AX, [_CurrentType]
	CMP	AX, 2
	JNZ	.L4
	MOV	AX, 1
	JMP	.L5
.L4:
	MOV	AX, [_CurrentType]
	CMP	AX, 3
	MOV	AX, 1
	JZ	.L6
	DEC	AL
.L6:
.L5:
	AND	AX, AX
	JZ	.L7
	JMP	.L8
.L7:
	MOV	AX, [_CurrentType]
	CMP	AX, 18
	MOV	AX, 1
	JZ	.L9
	DEC	AL
.L9:
.L8:
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	MOV	WORD [BP-2], 1
	JMP	.L3
.L2:
	MOV	WORD [BP-2], 2
.L3:
	MOV	AX, [BP+4]
	CMP	AX, 20
	JZ	.L10
	JMP	.L11
.L10:
	MOV	AX, [BP-2]
	CMP	AX, 1
	JZ	.L13
	JMP	.L14
.L13:
	JMP	.L17
.L16:	DB 'INC', 9, 'AX', 0
.L17:
	ADD	SP, -8
	MOV	AX, .L16
	MOV	[BP-10], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L15
.L14:
	JMP	.L19
.L18:	DB 'ADD', 9, 'AX, %d', 0
.L19:
	ADD	SP, -8
	MOV	AX, .L18
	MOV	[BP-10], AX
	MOV	AX, [BP-2]
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
.L15:
	JMP	.L12
.L11:
	ADD	SP, -8
	MOV	AX, [BP+4]
	CMP	AX, 22
	MOV	AX, 1
	JZ	.L20
	DEC	AL
.L20:
	MOV	[BP-10], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [BP-2]
	CMP	AX, 1
	JZ	.L21
	JMP	.L22
.L21:
	JMP	.L25
.L24:	DB 'DEC', 9, 'AX', 0
.L25:
	ADD	SP, -8
	MOV	AX, .L24
	MOV	[BP-10], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L23
.L22:
	JMP	.L27
.L26:	DB 'SUB', 9, 'AX, %d', 0
.L27:
	ADD	SP, -8
	MOV	AX, .L26
	MOV	[BP-10], AX
	MOV	AX, [BP-2]
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
.L23:
.L12:
	ADD	SP, 2
.L0:
	POP	BP
	RET
_DoIncDecOp:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	AND	AX, 4
	MOV	[BP-8], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_CurrentType]
	AND	AX, -5
	MOV	[_CurrentType], AX
	MOV	AX, [_CurrentType]
	AND	AX, 192
	PUSH	AX	; [BP-2] = loc
	MOV	AX, [BP-2]
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	MOV	AX, [_CurrentType]
	AND	AX, -193
	MOV	[_CurrentType], AX
	JMP	.L3
.L2:
	JMP	.L5
.L4:	DB 'MOV', 9, 'BX, AX', 0
.L5:
	ADD	SP, -8
	MOV	AX, .L4
	MOV	[BP-10], AX
	CALL	_Emit
	ADD	SP, 8
.L3:
	MOV	AX, 2
	PUSH	AX	; [BP-4] = Sz
	MOV	AX, [_CurrentType]
	CMP	AX, 2
	JZ	.L6
	JMP	.L7
.L6:
	MOV	BYTE [BP-4], 1
	JMP	.L8
.L7:
.L8:
	ADD	SP, -8
	MOV	AL, [BP-4]
	CBW
	MOV	[BP-12], AX
	MOV	AX, [BP-2]
	MOV	[BP-10], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-8], AX
	CALL	_EmitLoadAx
	ADD	SP, 8
	MOV	AX, [BP+6]
	AND	AX, AX
	JNZ	.L9
	JMP	.L10
.L9:
	JMP	.L13
.L12:	DB 'PUSH', 9, 'AX', 0
.L13:
	ADD	SP, -8
	MOV	AX, .L12
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L11
.L10:
.L11:
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-12], AX
	CALL	_DoIncDec
	ADD	SP, 8
	ADD	SP, -8
	MOV	AL, [BP-4]
	CBW
	MOV	[BP-12], AX
	MOV	AX, [BP-2]
	MOV	[BP-10], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-8], AX
	CALL	_EmitStoreAx
	ADD	SP, 8
	MOV	AL, [BP-4]
	CBW
	CMP	AX, 1
	JZ	.L14
	JMP	.L15
.L14:
	MOV	WORD [_CurrentType], 3
	JMP	.L16
.L15:
.L16:
	MOV	AX, [BP+6]
	AND	AX, AX
	JNZ	.L17
	JMP	.L18
.L17:
	JMP	.L21
.L20:	DB 'POP', 9, 'AX', 0
.L21:
	ADD	SP, -8
	MOV	AX, .L20
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L19
.L18:
.L19:
	ADD	SP, 4
.L0:
	POP	BP
	RET
_Lookup:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = vd
	ADD	SP, -8
	MOV	AX, [_ScopesCount]
	MOV	[BP-10], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_Scopes]
	PUSH	AX
	MOV	AX, [_ScopesCount]
	SUB	AX, 1
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[BP-2], AX
.L1:
	MOV	AX, [BP-2]
	CMP	AX, 0
	JNL	.L2
	JMP	.L3
.L4:
	MOV	AX, [BP-2]
	PUSH	AX
	DEC	AX
	MOV	[BP-2], AX
	POP	AX
	JMP	.L1
.L2:
	MOV	AX, [_VarDeclId]
	PUSH	AX
	MOV	AX, [BP-2]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	PUSH	AX
	MOV	AX, [BP+4]
	POP	CX
	XCHG	AX, CX
	CMP	AX, CX
	JZ	.L5
	JMP	.L6
.L5:
	JMP	.L3
.L6:
.L7:
	JMP	.L4
.L3:
	MOV	AX, [BP-2]
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_ParsePrimaryExpression:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = id
	PUSH	AX	; [BP-4] = vd
	MOV	AX, [_TokenType]
	CMP	AX, 3
	JZ	.L1
	JMP	.L2
.L1:
	CALL	_GetToken
	CALL	_ParseExpr
	ADD	SP, -8
	MOV	WORD [BP-12], 4
	CALL	_Expect
	ADD	SP, 8
	JMP	.L3
.L2:
	MOV	AX, [_TokenType]
	CMP	AX, 1
	JZ	.L4
	JMP	.L5
.L4:
	MOV	WORD [_CurrentType], 67
	MOV	AX, [_TokenNumVal]
	MOV	[_CurrentVal], AX
	CALL	_GetToken
	JMP	.L6
.L5:
	MOV	AX, [_TokenType]
	CMP	AX, 2
	JZ	.L7
	JMP	.L8
.L7:
	JMP	.L11
.L10:	DB 'MOV', 9, 'AX, .L%d', 0
.L11:
	ADD	SP, -8
	MOV	AX, .L10
	MOV	[BP-12], AX
	MOV	AX, [_TokenNumVal]
	MOV	[BP-10], AX
	CALL	_Emit
	ADD	SP, 8
	MOV	WORD [_CurrentType], 18
	CALL	_GetToken
	JMP	.L9
.L8:
	MOV	AX, [_TokenType]
	CMP	AX, 60
	JNZ	.L15
	MOV	AX, 1
	JMP	.L16
.L15:
	MOV	AX, [_TokenType]
	CMP	AX, 61
	MOV	AX, 1
	JZ	.L17
	DEC	AL
.L17:
.L16:
	AND	AX, AX
	JZ	.L18
	JMP	.L19
.L18:
	MOV	AX, [_TokenType]
	CMP	AX, 62
	MOV	AX, 1
	JZ	.L20
	DEC	AL
.L20:
.L19:
	AND	AX, AX
	JNZ	.L12
	JMP	.L13
.L12:
	MOV	AX, [_TokenType]
	PUSH	AX	; [BP-6] = func
	CALL	_GetToken
	ADD	SP, -8
	MOV	WORD [BP-14], 3
	CALL	_Expect
	ADD	SP, 8
	CALL	_ExpectId
	MOV	[BP-2], AX
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-14], AX
	CALL	_Lookup
	ADD	SP, 8
	MOV	[BP-4], AX
	MOV	AX, [BP-4]
	CMP	AX, 0
	JNL	.L24
	MOV	AX, 1
	JMP	.L25
.L24:
	MOV	AX, [_VarDeclType]
	PUSH	AX
	MOV	AX, [BP-4]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	CMP	AX, 18
	MOV	AX, 1
	JNZ	.L26
	DEC	AL
.L26:
.L25:
	AND	AX, AX
	JNZ	.L21
	JMP	.L22
.L21:
	JMP	.L28
.L27:	DB 'Invalid va_list', 0
.L28:
	ADD	SP, -8
	MOV	AX, .L27
	MOV	[BP-14], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L23
.L22:
.L23:
	MOV	AX, [_VarDeclOffset]
	PUSH	AX
	MOV	AX, [BP-4]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	PUSH	AX	; [BP-8] = offset
	MOV	AX, [BP-6]
	CMP	AX, 60
	JZ	.L29
	JMP	.L30
.L29:
	ADD	SP, -8
	MOV	WORD [BP-16], 9
	CALL	_Expect
	ADD	SP, 8
	CALL	_ExpectId
	MOV	[BP-2], AX
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-16], AX
	CALL	_Lookup
	ADD	SP, 8
	MOV	[BP-4], AX
	MOV	AX, [BP-4]
	CMP	AX, 0
	JNL	.L35
	MOV	AX, 1
	JMP	.L36
.L35:
	MOV	AX, [_VarDeclOffset]
	PUSH	AX
	MOV	AX, [BP-4]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	AND	AX, AX
	MOV	AX, 1
	JZ	.L38
	DEC	AL
.L38:
.L36:
	AND	AX, AX
	JNZ	.L32
	JMP	.L33
.L32:
	JMP	.L40
.L39:	DB 'Invalid argument to va_start', 0
.L40:
	ADD	SP, -8
	MOV	AX, .L39
	MOV	[BP-16], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L34
.L33:
.L34:
	JMP	.L42
.L41:	DB 'LEA', 9, 'AX, [BP%+d]', 0
.L42:
	ADD	SP, -8
	MOV	AX, .L41
	MOV	[BP-16], AX
	MOV	AX, [_VarDeclOffset]
	PUSH	AX
	MOV	AX, [BP-4]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[BP-14], AX
	CALL	_Emit
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-16], 2
	MOV	WORD [BP-14], 128
	MOV	AX, [BP-8]
	MOV	[BP-12], AX
	CALL	_EmitStoreAx
	ADD	SP, 8
	MOV	WORD [_CurrentType], 0
	JMP	.L31
.L30:
	MOV	AX, [BP-6]
	CMP	AX, 61
	JZ	.L43
	JMP	.L44
.L43:
	MOV	WORD [_CurrentType], 0
	JMP	.L45
.L44:
	MOV	AX, [BP-6]
	CMP	AX, 62
	JZ	.L46
	JMP	.L47
.L46:
	ADD	SP, -8
	MOV	WORD [BP-16], 9
	CALL	_Expect
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-16], 2
	MOV	WORD [BP-14], 128
	MOV	AX, [BP-8]
	MOV	[BP-12], AX
	CALL	_EmitLoadAx
	ADD	SP, 8
	JMP	.L50
.L49:	DB 'ADD', 9, 'AX, 2', 0
.L50:
	ADD	SP, -8
	MOV	AX, .L49
	MOV	[BP-16], AX
	CALL	_Emit
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-16], 2
	MOV	WORD [BP-14], 128
	MOV	AX, [BP-8]
	MOV	[BP-12], AX
	CALL	_EmitStoreAx
	ADD	SP, 8
	CALL	_ParseDeclSpecs
	OR	AX, 4
	MOV	[_CurrentType], AX
	JMP	.L48
.L47:
.L48:
.L45:
.L31:
	ADD	SP, -8
	MOV	WORD [BP-16], 4
	CALL	_Expect
	ADD	SP, 8
	ADD	SP, 4
	JMP	.L14
.L13:
	CALL	_ExpectId
	MOV	[BP-2], AX
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-12], AX
	CALL	_Lookup
	ADD	SP, 8
	MOV	[BP-4], AX
	MOV	AX, [BP-4]
	CMP	AX, 0
	JL	.L51
	JMP	.L52
.L51:
	MOV	WORD [_CurrentType], 75
	MOV	AX, [BP-2]
	MOV	[_CurrentVal], AX
	JMP	.L53
.L52:
	MOV	AX, [_VarDeclType]
	PUSH	AX
	MOV	AX, [BP-4]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[_CurrentType], AX
	MOV	AX, [_VarDeclOffset]
	PUSH	AX
	MOV	AX, [BP-4]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[_CurrentVal], AX
	MOV	AX, [_CurrentType]
	AND	AX, 192
	AND	AX, AX
	JNZ	.L54
	JMP	.L55
.L54:
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 67
	MOV	AX, 1
	JZ	.L57
	DEC	AL
.L57:
	MOV	[BP-12], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L0
.L55:
.L56:
	MOV	AX, [_CurrentType]
	OR	AX, 4
	MOV	[_CurrentType], AX
	MOV	AX, [_CurrentVal]
	AND	AX, AX
	JNZ	.L58
	JMP	.L59
.L58:
	MOV	AX, [_CurrentType]
	OR	AX, 128
	MOV	[_CurrentType], AX
	JMP	.L60
.L59:
	MOV	AX, [BP-2]
	MOV	[_CurrentVal], AX
	MOV	AX, [_CurrentType]
	AND	AX, 8
	AND	AX, AX
	JNZ	.L61
	JMP	.L62
.L61:
	MOV	AX, [_CurrentType]
	AND	AX, -5
	OR	AX, 64
	MOV	[_CurrentType], AX
	JMP	.L63
.L62:
	MOV	AX, [_CurrentType]
	OR	AX, 192
	MOV	[_CurrentType], AX
.L63:
.L60:
.L53:
.L14:
.L9:
.L6:
.L3:
	ADD	SP, 4
.L0:
	MOV	SP, BP
	POP	BP
	RET
_GetJcc:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	CMP	AX, 4
	JZ	.L1
	JMP	.L2
.L1:
	JMP	.L5
.L4:	DB 'JZ', 0
.L5:
	MOV	AX, .L4
	JMP	.L0
.L2:
.L3:
	MOV	AX, [BP+4]
	CMP	AX, 5
	JZ	.L6
	JMP	.L7
.L6:
	JMP	.L10
.L9:	DB 'JNZ', 0
.L10:
	MOV	AX, .L9
	JMP	.L0
.L7:
.L8:
	MOV	AX, [BP+4]
	CMP	AX, 12
	JZ	.L11
	JMP	.L12
.L11:
	JMP	.L15
.L14:	DB 'JL', 0
.L15:
	MOV	AX, .L14
	JMP	.L0
.L12:
.L13:
	MOV	AX, [BP+4]
	CMP	AX, 13
	JZ	.L16
	JMP	.L17
.L16:
	JMP	.L20
.L19:	DB 'JNL', 0
.L20:
	MOV	AX, .L19
	JMP	.L0
.L17:
.L18:
	MOV	AX, [BP+4]
	CMP	AX, 14
	JZ	.L21
	JMP	.L22
.L21:
	JMP	.L25
.L24:	DB 'JNG', 0
.L25:
	MOV	AX, .L24
	JMP	.L0
.L22:
.L23:
	MOV	AX, [BP+4]
	CMP	AX, 15
	JZ	.L26
	JMP	.L27
.L26:
	JMP	.L30
.L29:	DB 'JG', 0
.L30:
	MOV	AX, .L29
	JMP	.L0
.L27:
.L28:
	JMP	.L32
.L31:	DB 'Not implemented', 0
.L32:
	ADD	SP, -8
	MOV	AX, .L31
	MOV	[BP-8], AX
	CALL	_Fatal
	ADD	SP, 8
.L0:
	POP	BP
	RET
_GetVal:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [_CurrentType]
	CMP	AX, 1
	JZ	.L1
	JMP	.L2
.L1:
	PUSH	AX	; [BP-2] = Lab
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	MOV	[BP-2], AX
	JMP	.L5
.L4:	DB 'MOV', 9, 'AX, 1', 0
.L5:
	ADD	SP, -8
	MOV	AX, .L4
	MOV	[BP-10], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L7
.L6:	DB '%s', 9, '.L%d', 0
.L7:
	ADD	SP, -8
	MOV	AX, .L6
	MOV	[BP-10], AX
	ADD	SP, -8
	MOV	AX, [_CurrentVal]
	MOV	[BP-18], AX
	CALL	_GetJcc
	ADD	SP, 8
	MOV	[BP-8], AX
	MOV	AX, [BP-2]
	MOV	[BP-6], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L9
.L8:	DB 'DEC', 9, 'AL', 0
.L9:
	ADD	SP, -8
	MOV	AX, .L8
	MOV	[BP-10], AX
	CALL	_Emit
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-10], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	MOV	WORD [_CurrentType], 3
	JMP	.L0
.L2:
.L3:
	MOV	AX, [_CurrentType]
	AND	AX, 192
	PUSH	AX	; [BP-2] = loc
	MOV	AX, [BP-2]
	AND	AX, AX
	JNZ	.L10
	JMP	.L11
.L10:
	ADD	SP, -8
	MOV	AX, [BP-2]
	CMP	AX, 64
	MOV	AX, 1
	JZ	.L13
	DEC	AL
.L13:
	MOV	[BP-10], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_CurrentType]
	AND	AX, -193
	MOV	[_CurrentType], AX
	JMP	.L15
.L14:	DB 'MOV', 9, 'AX, %d', 0
.L15:
	ADD	SP, -8
	MOV	AX, .L14
	MOV	[BP-10], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L12
.L11:
.L12:
	ADD	SP, 2
.L0:
	MOV	SP, BP
	POP	BP
	RET
_ParsePostfixExpression:
	PUSH	BP
	MOV	BP, SP
.L1:
	JMP	.L2
.L2:
	ADD	SP, -8
	MOV	WORD [BP-8], 3
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L4
	JMP	.L5
.L4:
	MOV	AX, [_CurrentType]
	AND	AX, 8
	AND	AX, AX
	JZ	.L7
	JMP	.L8
.L7:
	JMP	.L12
.L11:	DB 'Not a function', 0
.L12:
	ADD	SP, -8
	MOV	AX, .L11
	MOV	[BP-8], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L9
.L8:
.L9:
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	AND	AX, 192
	CMP	AX, 64
	MOV	AX, 1
	JZ	.L13
	DEC	AL
.L13:
	MOV	[BP-8], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_CurrentVal]
	PUSH	AX	; [BP-2] = FuncId
	MOV	AX, [_CurrentType]
	AND	AX, -201
	PUSH	AX	; [BP-4] = RetType
	MOV	AX, 0
	PUSH	AX	; [BP-6] = NumArgs
.L14:
	MOV	AX, [_TokenType]
	CMP	AX, 4
	JNZ	.L15
	JMP	.L16
.L15:
	MOV	AX, [BP-6]
	AND	AX, AX
	JZ	.L17
	JMP	.L18
.L17:
	ADD	SP, -8
	MOV	WORD [BP-14], -8
	CALL	_EmitAdjSp
	ADD	SP, 8
	MOV	AX, [_LocalOffset]
	SUB	AX, 8
	MOV	[_LocalOffset], AX
	JMP	.L19
.L18:
.L19:
	ADD	SP, -8
	MOV	AX, [BP-6]
	CMP	AX, 4
	MOV	AX, 1
	JL	.L21
	DEC	AL
.L21:
	MOV	[BP-14], AX
	CALL	_Check
	ADD	SP, 8
	CALL	_ParseAssignmentExpression
	CALL	_LvalToRval
	MOV	AX, [_LocalOffset]
	PUSH	AX
	MOV	AX, [BP-6]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	PUSH	AX	; [BP-8] = off
	MOV	AX, [_CurrentType]
	AND	AX, 192
	AND	AX, AX
	JNZ	.L22
	JMP	.L23
.L22:
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 67
	MOV	AX, 1
	JZ	.L25
	DEC	AL
.L25:
	MOV	[BP-16], AX
	CALL	_Check
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-16], 2
	MOV	WORD [BP-14], 128
	MOV	AX, [BP-8]
	MOV	[BP-12], AX
	CALL	_EmitStoreConst
	ADD	SP, 8
	JMP	.L24
.L23:
	CALL	_GetVal
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 3
	JNZ	.L26
	MOV	AX, 1
	JMP	.L27
.L26:
	MOV	AX, [_CurrentType]
	AND	AX, 48
.L27:
	MOV	[BP-16], AX
	CALL	_Check
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-16], 2
	MOV	WORD [BP-14], 128
	MOV	AX, [BP-8]
	MOV	[BP-12], AX
	CALL	_EmitStoreAx
	ADD	SP, 8
.L24:
	MOV	AX, [BP-6]
	INC	AX
	MOV	[BP-6], AX
	ADD	SP, -8
	MOV	WORD [BP-16], 9
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JZ	.L28
	JMP	.L29
.L28:
	ADD	SP, 2
	JMP	.L16
.L29:
.L30:
	ADD	SP, 2
	JMP	.L14
.L16:
	ADD	SP, -8
	MOV	WORD [BP-14], 4
	CALL	_Expect
	ADD	SP, 8
	JMP	.L33
.L32:	DB 'CALL', 9, '_%s', 0
.L33:
	ADD	SP, -8
	MOV	AX, .L32
	MOV	[BP-14], AX
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-22], AX
	CALL	_IdText
	ADD	SP, 8
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
	MOV	AX, [BP-6]
	AND	AX, AX
	JNZ	.L34
	JMP	.L35
.L34:
	ADD	SP, -8
	MOV	WORD [BP-14], 8
	CALL	_EmitAdjSp
	ADD	SP, 8
	MOV	AX, [_LocalOffset]
	ADD	AX, 8
	MOV	[_LocalOffset], AX
	JMP	.L36
.L35:
.L36:
	MOV	AX, [BP-4]
	MOV	[_CurrentType], AX
	MOV	AX, [_CurrentType]
	CMP	AX, 2
	JZ	.L37
	JMP	.L38
.L37:
	MOV	WORD [_CurrentType], 3
	JMP	.L39
.L38:
.L39:
	ADD	SP, 6
	JMP	.L6
.L5:
	ADD	SP, -8
	MOV	WORD [BP-8], 7
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L40
	JMP	.L41
.L40:
	CALL	_LvalToRval
	MOV	AX, [_CurrentType]
	AND	AX, 48
	AND	AX, AX
	JZ	.L43
	JMP	.L44
.L43:
	JMP	.L48
.L47:	DB 'Expected pointer', 0
.L48:
	ADD	SP, -8
	MOV	AX, .L47
	MOV	[BP-8], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L45
.L44:
.L45:
	MOV	AX, [_CurrentType]
	SUB	AX, 16
	PUSH	AX	; [BP-2] = AType
	MOV	AX, 0
	PUSH	AX	; [BP-4] = Double
	MOV	AX, [BP-2]
	CMP	AX, 2
	JNZ	.L49
	JMP	.L50
.L49:
	ADD	SP, -8
	MOV	AX, [BP-2]
	CMP	AX, 3
	JNZ	.L52
	MOV	AX, 1
	JMP	.L53
.L52:
	MOV	AX, [BP-2]
	AND	AX, 48
.L53:
	MOV	[BP-12], AX
	CALL	_Check
	ADD	SP, 8
	MOV	WORD [BP-4], 1
	JMP	.L51
.L50:
.L51:
	JMP	.L55
.L54:	DB 'PUSH', 9, 'AX', 0
.L55:
	ADD	SP, -8
	MOV	AX, .L54
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
	CALL	_ParseExpr
	ADD	SP, -8
	MOV	WORD [BP-12], 8
	CALL	_Expect
	ADD	SP, 8
	MOV	AX, [_CurrentType]
	CMP	AX, 67
	JZ	.L56
	JMP	.L57
.L56:
	MOV	AX, [BP-4]
	AND	AX, AX
	JNZ	.L59
	JMP	.L60
.L59:
	MOV	AX, [_CurrentVal]
	MOV	CX, 1
	SHL	AX, CL
	MOV	[_CurrentVal], AX
	JMP	.L61
.L60:
.L61:
	JMP	.L63
.L62:	DB 'POP', 9, 'AX', 0
.L63:
	ADD	SP, -8
	MOV	AX, .L62
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L65
.L64:	DB 'ADD', 9, 'AX, %d', 0
.L65:
	ADD	SP, -8
	MOV	AX, .L64
	MOV	[BP-12], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-10], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L58
.L57:
	CALL	_LvalToRval
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 3
	MOV	AX, 1
	JZ	.L66
	DEC	AL
.L66:
	MOV	[BP-12], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [BP-4]
	AND	AX, AX
	JNZ	.L67
	JMP	.L68
.L67:
	JMP	.L71
.L70:	DB 'ADD', 9, 'AX, AX', 0
.L71:
	ADD	SP, -8
	MOV	AX, .L70
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L69
.L68:
.L69:
	JMP	.L73
.L72:	DB 'POP', 9, 'CX', 0
.L73:
	ADD	SP, -8
	MOV	AX, .L72
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L75
.L74:	DB 'ADD', 9, 'AX, CX', 0
.L75:
	ADD	SP, -8
	MOV	AX, .L74
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
.L58:
	MOV	AX, [BP-2]
	OR	AX, 4
	MOV	[_CurrentType], AX
	ADD	SP, 4
	JMP	.L42
.L41:
	MOV	AX, [_TokenType]
	PUSH	AX	; [BP-2] = Op
	MOV	AX, [BP-2]
	CMP	AX, 20
	JNZ	.L79
	MOV	AX, 0
	JMP	.L80
.L79:
	MOV	AX, [BP-2]
	CMP	AX, 22
	MOV	AX, 1
	JNZ	.L81
	DEC	AL
.L81:
.L80:
	AND	AX, AX
	JNZ	.L76
	JMP	.L77
.L76:
	ADD	SP, 2
	JMP	.L3
.L77:
.L78:
	CALL	_GetToken
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-10], AX
	MOV	WORD [BP-8], 1
	CALL	_DoIncDecOp
	ADD	SP, 8
	ADD	SP, 2
.L42:
.L6:
	JMP	.L1
.L3:
.L0:
	POP	BP
	RET
_ParseUnaryExpression:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [_TokenType]
	PUSH	AX	; [BP-2] = Op
	MOV	AX, 0
	PUSH	AX	; [BP-4] = IsConst
	MOV	AX, [BP-2]
	CMP	AX, 20
	JNZ	.L4
	MOV	AX, 1
	JMP	.L5
.L4:
	MOV	AX, [BP-2]
	CMP	AX, 22
	MOV	AX, 1
	JZ	.L6
	DEC	AL
.L6:
.L5:
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	CALL	_GetToken
	CALL	_ParseUnaryExpression
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-12], AX
	MOV	WORD [BP-10], 0
	CALL	_DoIncDecOp
	ADD	SP, 8
	JMP	.L3
.L2:
	MOV	AX, [BP-2]
	CMP	AX, 26
	JNZ	.L10
	MOV	AX, 1
	JMP	.L11
.L10:
	MOV	AX, [BP-2]
	CMP	AX, 23
	MOV	AX, 1
	JZ	.L12
	DEC	AL
.L12:
.L11:
	AND	AX, AX
	JZ	.L13
	JMP	.L14
.L13:
	MOV	AX, [BP-2]
	CMP	AX, 19
	MOV	AX, 1
	JZ	.L15
	DEC	AL
.L15:
.L14:
	AND	AX, AX
	JZ	.L16
	JMP	.L17
.L16:
	MOV	AX, [BP-2]
	CMP	AX, 21
	MOV	AX, 1
	JZ	.L18
	DEC	AL
.L18:
.L17:
	AND	AX, AX
	JZ	.L19
	JMP	.L20
.L19:
	MOV	AX, [BP-2]
	CMP	AX, 43
	MOV	AX, 1
	JZ	.L21
	DEC	AL
.L21:
.L20:
	AND	AX, AX
	JZ	.L22
	JMP	.L23
.L22:
	MOV	AX, [BP-2]
	CMP	AX, 13
	MOV	AX, 1
	JZ	.L24
	DEC	AL
.L24:
.L23:
	AND	AX, AX
	JNZ	.L7
	JMP	.L8
.L7:
	CALL	_GetToken
	CALL	_ParseCastExpression
	MOV	AX, [_CurrentType]
	AND	AX, 192
	CMP	AX, 64
	JZ	.L25
	JMP	.L26
.L25:
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 67
	MOV	AX, 1
	JZ	.L28
	DEC	AL
.L28:
	MOV	[BP-12], AX
	CALL	_Check
	ADD	SP, 8
	MOV	WORD [BP-4], 1
	JMP	.L27
.L26:
	MOV	AX, [BP-2]
	CMP	AX, 26
	JNZ	.L29
	JMP	.L30
.L29:
	CALL	_LvalToRval
	JMP	.L31
.L30:
.L31:
.L27:
	MOV	AX, [BP-2]
	CMP	AX, 26
	JZ	.L32
	JMP	.L33
.L32:
	MOV	AX, [_CurrentType]
	AND	AX, 4
	AND	AX, AX
	JZ	.L35
	JMP	.L36
.L35:
	JMP	.L40
.L39:	DB 'Lvalue required for address-of operator', 0
.L40:
	ADD	SP, -8
	MOV	AX, .L39
	MOV	[BP-12], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L37
.L36:
.L37:
	MOV	AX, [_CurrentType]
	AND	AX, 192
	PUSH	AX	; [BP-6] = loc
	MOV	AX, [BP-6]
	AND	AX, AX
	JNZ	.L41
	JMP	.L42
.L41:
	MOV	AX, [BP-6]
	CMP	AX, 128
	JZ	.L44
	JMP	.L45
.L44:
	JMP	.L48
.L47:	DB 'LEA', 9, 'AX, [BP%+d]', 0
.L48:
	ADD	SP, -8
	MOV	AX, .L47
	MOV	[BP-14], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L46
.L45:
	ADD	SP, -8
	MOV	WORD [BP-14], 0
	CALL	_Check
	ADD	SP, 8
.L46:
	MOV	AX, [_CurrentType]
	AND	AX, -193
	MOV	[_CurrentType], AX
	JMP	.L43
.L42:
.L43:
	MOV	AX, [_CurrentType]
	AND	AX, -5
	ADD	AX, 16
	MOV	[_CurrentType], AX
	ADD	SP, 2
	JMP	.L34
.L33:
	MOV	AX, [BP-2]
	CMP	AX, 23
	JZ	.L49
	JMP	.L50
.L49:
	MOV	AX, [_CurrentType]
	AND	AX, 48
	AND	AX, AX
	JZ	.L52
	JMP	.L53
.L52:
	JMP	.L57
.L56:	DB 'Pointer required for dereference', 0
.L57:
	ADD	SP, -8
	MOV	AX, .L56
	MOV	[BP-12], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L54
.L53:
.L54:
	MOV	AX, [_CurrentType]
	SUB	AX, 16
	OR	AX, 4
	MOV	[_CurrentType], AX
	JMP	.L51
.L50:
	MOV	AX, [BP-2]
	CMP	AX, 19
	JZ	.L58
	JMP	.L59
.L58:
	ADD	SP, -8
	MOV	AX, [BP-4]
	AND	AX, AX
	JZ	.L61
	JMP	.L62
.L61:
	MOV	AX, [_CurrentType]
	CMP	AX, 3
	MOV	AX, 1
	JZ	.L63
	DEC	AL
.L63:
.L62:
	MOV	[BP-12], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L60
.L59:
	MOV	AX, [BP-2]
	CMP	AX, 21
	JZ	.L64
	JMP	.L65
.L64:
	MOV	AX, [BP-4]
	AND	AX, AX
	JNZ	.L67
	JMP	.L68
.L67:
	MOV	AX, [_CurrentVal]
	NEG	AX
	MOV	[_CurrentVal], AX
	JMP	.L69
.L68:
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 3
	MOV	AX, 1
	JZ	.L70
	DEC	AL
.L70:
	MOV	[BP-12], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L72
.L71:	DB 'NEG', 9, 'AX', 0
.L72:
	ADD	SP, -8
	MOV	AX, .L71
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
.L69:
	JMP	.L66
.L65:
	MOV	AX, [BP-2]
	CMP	AX, 43
	JZ	.L73
	JMP	.L74
.L73:
	MOV	AX, [BP-4]
	AND	AX, AX
	JNZ	.L76
	JMP	.L77
.L76:
	MOV	AX, [_CurrentVal]
	NOT	AX
	MOV	[_CurrentVal], AX
	JMP	.L78
.L77:
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 3
	MOV	AX, 1
	JZ	.L79
	DEC	AL
.L79:
	MOV	[BP-12], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L81
.L80:	DB 'NOT', 9, 'AX', 0
.L81:
	ADD	SP, -8
	MOV	AX, .L80
	MOV	[BP-12], AX
	CALL	_Emit
	ADD	SP, 8
.L78:
	JMP	.L75
.L74:
	MOV	AX, [BP-2]
	CMP	AX, 13
	JZ	.L82
	JMP	.L83
.L82:
	MOV	AX, [BP-4]
	AND	AX, AX
	JNZ	.L85
	JMP	.L86
.L85:
	MOV	AX, [_CurrentVal]
	AND	AX, AX
	MOV	AX, 1
	JZ	.L89
	DEC	AL
.L89:
	MOV	[_CurrentVal], AX
	JMP	.L87
.L86:
	PUSH	AX	; [BP-6] = Lab
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	MOV	[BP-6], AX
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 3
	JNZ	.L90
	MOV	AX, 1
	JMP	.L91
.L90:
	MOV	AX, [_CurrentType]
	AND	AX, 48
.L91:
	MOV	[BP-14], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L93
.L92:	DB 'AND', 9, 'AX, AX', 0
.L93:
	ADD	SP, -8
	MOV	AX, .L92
	MOV	[BP-14], AX
	CALL	_Emit
	ADD	SP, 8
	MOV	WORD [_CurrentType], 1
	MOV	WORD [_CurrentVal], 4
	ADD	SP, 2
.L87:
	JMP	.L84
.L83:
	CALL	_Unexpected
.L84:
.L75:
.L66:
.L60:
.L51:
.L34:
	JMP	.L9
.L8:
	MOV	AX, [BP-2]
	CMP	AX, 56
	JZ	.L94
	JMP	.L95
.L94:
	CALL	_GetToken
	ADD	SP, -8
	MOV	WORD [BP-12], 3
	CALL	_Expect
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-12], 47
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L97
	JMP	.L98
.L97:
	MOV	WORD [_CurrentVal], 1
	JMP	.L99
.L98:
	ADD	SP, -8
	MOV	WORD [BP-12], 54
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L100
	JMP	.L101
.L100:
	MOV	WORD [_CurrentVal], 2
	JMP	.L102
.L101:
	JMP	.L104
.L103:	DB 'sizeof not implemented for this type', 0
.L104:
	ADD	SP, -8
	MOV	AX, .L103
	MOV	[BP-12], AX
	CALL	_Fatal
	ADD	SP, 8
.L102:
.L99:
.L105:
	ADD	SP, -8
	MOV	WORD [BP-12], 23
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L106
	JMP	.L107
.L106:
	MOV	WORD [_CurrentVal], 2
	JMP	.L105
.L107:
	ADD	SP, -8
	MOV	WORD [BP-12], 4
	CALL	_Expect
	ADD	SP, 8
	MOV	WORD [_CurrentType], 67
	JMP	.L96
.L95:
	CALL	_ParsePrimaryExpression
	CALL	_ParsePostfixExpression
.L96:
.L9:
.L3:
	ADD	SP, 4
.L0:
	POP	BP
	RET
_ParseCastExpression:
	PUSH	BP
	MOV	BP, SP
	CALL	_ParseUnaryExpression
.L0:
	POP	BP
	RET
_RelOpToCC:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	CMP	AX, 15
	JZ	.L1
	JMP	.L2
.L1:
	MOV	AX, 12
	JMP	.L0
.L2:
	MOV	AX, [BP+4]
	CMP	AX, 16
	JZ	.L4
	JMP	.L5
.L4:
	MOV	AX, 14
	JMP	.L0
.L5:
	MOV	AX, [BP+4]
	CMP	AX, 17
	JZ	.L7
	JMP	.L8
.L7:
	MOV	AX, 15
	JMP	.L0
.L8:
	MOV	AX, [BP+4]
	CMP	AX, 18
	JZ	.L10
	JMP	.L11
.L10:
	MOV	AX, 13
	JMP	.L0
.L11:
	MOV	AX, [BP+4]
	CMP	AX, 12
	JZ	.L13
	JMP	.L14
.L13:
	MOV	AX, 4
	JMP	.L0
.L14:
	MOV	AX, [BP+4]
	CMP	AX, 14
	JZ	.L16
	JMP	.L17
.L16:
	MOV	AX, 5
	JMP	.L0
.L17:
.L18:
.L15:
.L12:
.L9:
.L6:
.L3:
	JMP	.L20
.L19:	DB 'Not implemented', 0
.L20:
	ADD	SP, -8
	MOV	AX, .L19
	MOV	[BP-8], AX
	CALL	_Fatal
	ADD	SP, 8
.L0:
	POP	BP
	RET
_IsRelOp:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	CMP	AX, 15
	JNZ	.L1
	MOV	AX, 1
	JMP	.L2
.L1:
	MOV	AX, [BP+4]
	CMP	AX, 16
	MOV	AX, 1
	JZ	.L3
	DEC	AL
.L3:
.L2:
	AND	AX, AX
	JZ	.L4
	JMP	.L5
.L4:
	MOV	AX, [BP+4]
	CMP	AX, 17
	MOV	AX, 1
	JZ	.L6
	DEC	AL
.L6:
.L5:
	AND	AX, AX
	JZ	.L7
	JMP	.L8
.L7:
	MOV	AX, [BP+4]
	CMP	AX, 18
	MOV	AX, 1
	JZ	.L9
	DEC	AL
.L9:
.L8:
	AND	AX, AX
	JZ	.L10
	JMP	.L11
.L10:
	MOV	AX, [BP+4]
	CMP	AX, 12
	MOV	AX, 1
	JZ	.L12
	DEC	AL
.L12:
.L11:
	AND	AX, AX
	JZ	.L13
	JMP	.L14
.L13:
	MOV	AX, [BP+4]
	CMP	AX, 14
	MOV	AX, 1
	JZ	.L15
	DEC	AL
.L15:
.L14:
	JMP	.L0
.L0:
	POP	BP
	RET
_RemoveAssign:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	CMP	AX, 33
	JZ	.L1
	JMP	.L2
.L1:
	MOV	AX, 19
	JMP	.L0
.L2:
.L3:
	MOV	AX, [BP+4]
	CMP	AX, 34
	JZ	.L4
	JMP	.L5
.L4:
	MOV	AX, 21
	JMP	.L0
.L5:
.L6:
	MOV	AX, [BP+4]
	CMP	AX, 35
	JZ	.L7
	JMP	.L8
.L7:
	MOV	AX, 23
	JMP	.L0
.L8:
.L9:
	MOV	AX, [BP+4]
	CMP	AX, 36
	JZ	.L10
	JMP	.L11
.L10:
	MOV	AX, 24
	JMP	.L0
.L11:
.L12:
	MOV	AX, [BP+4]
	CMP	AX, 37
	JZ	.L13
	JMP	.L14
.L13:
	MOV	AX, 25
	JMP	.L0
.L14:
.L15:
	MOV	AX, [BP+4]
	CMP	AX, 38
	JZ	.L16
	JMP	.L17
.L16:
	MOV	AX, 31
	JMP	.L0
.L17:
.L18:
	MOV	AX, [BP+4]
	CMP	AX, 39
	JZ	.L19
	JMP	.L20
.L19:
	MOV	AX, 32
	JMP	.L0
.L20:
.L21:
	MOV	AX, [BP+4]
	CMP	AX, 40
	JZ	.L22
	JMP	.L23
.L22:
	MOV	AX, 26
	JMP	.L0
.L23:
.L24:
	MOV	AX, [BP+4]
	CMP	AX, 41
	JZ	.L25
	JMP	.L26
.L25:
	MOV	AX, 27
	JMP	.L0
.L26:
.L27:
	MOV	AX, [BP+4]
	CMP	AX, 42
	JZ	.L28
	JMP	.L29
.L28:
	MOV	AX, 28
	JMP	.L0
.L29:
.L30:
	ADD	SP, -8
	MOV	WORD [BP-8], 0
	CALL	_Check
	ADD	SP, 8
.L0:
	POP	BP
	RET
_DoBinOp:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-8], AX
	CALL	_IsRelOp
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	JMP	.L5
.L4:	DB 'CMP', 9, 'AX, CX', 0
.L5:
	ADD	SP, -8
	MOV	AX, .L4
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	MOV	WORD [_CurrentType], 1
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-8], AX
	CALL	_RelOpToCC
	ADD	SP, 8
	MOV	[_CurrentVal], AX
	JMP	.L0
.L2:
.L3:
	MOV	AX, [BP+4]
	CMP	AX, 19
	JZ	.L6
	JMP	.L7
.L6:
	JMP	.L10
.L9:	DB 'ADD', 9, 'AX, CX', 0
.L10:
	ADD	SP, -8
	MOV	AX, .L9
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L8
.L7:
	MOV	AX, [BP+4]
	CMP	AX, 21
	JZ	.L11
	JMP	.L12
.L11:
	JMP	.L15
.L14:	DB 'SUB', 9, 'AX, CX', 0
.L15:
	ADD	SP, -8
	MOV	AX, .L14
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L13
.L12:
	MOV	AX, [BP+4]
	CMP	AX, 23
	JZ	.L16
	JMP	.L17
.L16:
	JMP	.L20
.L19:	DB 'IMUL', 9, 'CX', 0
.L20:
	ADD	SP, -8
	MOV	AX, .L19
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L18
.L17:
	MOV	AX, [BP+4]
	CMP	AX, 24
	JNZ	.L24
	MOV	AX, 1
	JMP	.L25
.L24:
	MOV	AX, [BP+4]
	CMP	AX, 25
	MOV	AX, 1
	JZ	.L26
	DEC	AL
.L26:
.L25:
	AND	AX, AX
	JNZ	.L21
	JMP	.L22
.L21:
	JMP	.L28
.L27:	DB 'CWD', 0
.L28:
	ADD	SP, -8
	MOV	AX, .L27
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L30
.L29:	DB 'IDIV', 9, 'CX', 0
.L30:
	ADD	SP, -8
	MOV	AX, .L29
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	MOV	AX, [BP+4]
	CMP	AX, 25
	JZ	.L31
	JMP	.L32
.L31:
	JMP	.L35
.L34:	DB 'MOV', 9, 'AX, DX', 0
.L35:
	ADD	SP, -8
	MOV	AX, .L34
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L33
.L32:
.L33:
	JMP	.L23
.L22:
	MOV	AX, [BP+4]
	CMP	AX, 26
	JZ	.L36
	JMP	.L37
.L36:
	JMP	.L40
.L39:	DB 'AND', 9, 'AX, CX', 0
.L40:
	ADD	SP, -8
	MOV	AX, .L39
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L38
.L37:
	MOV	AX, [BP+4]
	CMP	AX, 27
	JZ	.L41
	JMP	.L42
.L41:
	JMP	.L45
.L44:	DB 'XOR', 9, 'AX, CX', 0
.L45:
	ADD	SP, -8
	MOV	AX, .L44
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L43
.L42:
	MOV	AX, [BP+4]
	CMP	AX, 28
	JZ	.L46
	JMP	.L47
.L46:
	JMP	.L50
.L49:	DB 'OR', 9, 'AX, CX', 0
.L50:
	ADD	SP, -8
	MOV	AX, .L49
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L48
.L47:
	MOV	AX, [BP+4]
	CMP	AX, 31
	JZ	.L51
	JMP	.L52
.L51:
	JMP	.L55
.L54:	DB 'SHL', 9, 'AX, CL', 0
.L55:
	ADD	SP, -8
	MOV	AX, .L54
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L53
.L52:
	MOV	AX, [BP+4]
	CMP	AX, 32
	JZ	.L56
	JMP	.L57
.L56:
	JMP	.L60
.L59:	DB 'SAR', 9, 'AX, CL', 0
.L60:
	ADD	SP, -8
	MOV	AX, .L59
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L58
.L57:
	ADD	SP, -8
	MOV	WORD [BP-8], 0
	CALL	_Check
	ADD	SP, 8
.L58:
.L53:
.L48:
.L43:
.L38:
.L23:
.L18:
.L13:
.L8:
.L0:
	POP	BP
	RET
_DoRhsConstBinOp:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-8], AX
	CALL	_IsRelOp
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	JMP	.L5
.L4:	DB 'CMP', 9, 'AX, %d', 0
.L5:
	ADD	SP, -8
	MOV	AX, .L4
	MOV	[BP-8], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-6], AX
	CALL	_Emit
	ADD	SP, 8
	MOV	WORD [_CurrentType], 1
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-8], AX
	CALL	_RelOpToCC
	ADD	SP, 8
	MOV	[_CurrentVal], AX
	JMP	.L0
.L2:
.L3:
	MOV	AX, [BP+4]
	CMP	AX, 23
	JZ	.L9
	MOV	AX, 0
	JMP	.L10
.L9:
	MOV	AX, [_CurrentVal]
	CMP	AX, 2
	MOV	AX, 1
	JZ	.L11
	DEC	AL
.L11:
.L10:
	AND	AX, AX
	JNZ	.L6
	JMP	.L7
.L6:
	JMP	.L13
.L12:	DB 'ADD', 9, 'AX, AX', 0
.L13:
	ADD	SP, -8
	MOV	AX, .L12
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L0
.L7:
.L8:
	MOV	AX, 0
	PUSH	AX	; [BP-2] = Inst
	MOV	AX, [BP+4]
	CMP	AX, 19
	JZ	.L14
	JMP	.L15
.L14:
	JMP	.L18
.L17:	DB 'ADD', 0
.L18:
	MOV	AX, .L17
	MOV	[BP-2], AX
	JMP	.L16
.L15:
	MOV	AX, [BP+4]
	CMP	AX, 21
	JZ	.L19
	JMP	.L20
.L19:
	JMP	.L23
.L22:	DB 'SUB', 0
.L23:
	MOV	AX, .L22
	MOV	[BP-2], AX
	JMP	.L21
.L20:
	MOV	AX, [BP+4]
	CMP	AX, 26
	JZ	.L24
	JMP	.L25
.L24:
	JMP	.L28
.L27:	DB 'AND', 0
.L28:
	MOV	AX, .L27
	MOV	[BP-2], AX
	JMP	.L26
.L25:
	MOV	AX, [BP+4]
	CMP	AX, 27
	JZ	.L29
	JMP	.L30
.L29:
	JMP	.L33
.L32:	DB 'XOR', 0
.L33:
	MOV	AX, .L32
	MOV	[BP-2], AX
	JMP	.L31
.L30:
	MOV	AX, [BP+4]
	CMP	AX, 28
	JZ	.L34
	JMP	.L35
.L34:
	JMP	.L38
.L37:	DB 'OR', 0
.L38:
	MOV	AX, .L37
	MOV	[BP-2], AX
	JMP	.L36
.L35:
	JMP	.L40
.L39:	DB 'MOV', 9, 'CX, %d', 0
.L40:
	ADD	SP, -8
	MOV	AX, .L39
	MOV	[BP-10], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-10], AX
	CALL	_DoBinOp
	ADD	SP, 8
	JMP	.L0
.L36:
.L31:
.L26:
.L21:
.L16:
	JMP	.L42
.L41:	DB '%s', 9, 'AX, %d', 0
.L42:
	ADD	SP, -8
	MOV	AX, .L41
	MOV	[BP-10], AX
	MOV	AX, [BP-2]
	MOV	[BP-8], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-6], AX
	CALL	_Emit
	ADD	SP, 8
	ADD	SP, 2
.L0:
	MOV	SP, BP
	POP	BP
	RET
_DoConstBinOp:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	CMP	AX, 19
	JZ	.L1
	JMP	.L2
.L1:
	MOV	AX, [BP+6]
	PUSH	AX
	MOV	AX, [BP+8]
	POP	CX
	ADD	AX, CX
	JMP	.L0
.L2:
.L3:
	MOV	AX, [BP+4]
	CMP	AX, 21
	JZ	.L4
	JMP	.L5
.L4:
	MOV	AX, [BP+6]
	PUSH	AX
	MOV	AX, [BP+8]
	POP	CX
	XCHG	AX, CX
	SUB	AX, CX
	JMP	.L0
.L5:
.L6:
	MOV	AX, [BP+4]
	CMP	AX, 23
	JZ	.L7
	JMP	.L8
.L7:
	MOV	AX, [BP+6]
	PUSH	AX
	MOV	AX, [BP+8]
	POP	CX
	IMUL	CX
	JMP	.L0
.L8:
.L9:
	MOV	AX, [BP+4]
	CMP	AX, 24
	JZ	.L10
	JMP	.L11
.L10:
	MOV	AX, [BP+6]
	PUSH	AX
	MOV	AX, [BP+8]
	POP	CX
	XCHG	AX, CX
	CWD
	IDIV	CX
	JMP	.L0
.L11:
.L12:
	MOV	AX, [BP+4]
	CMP	AX, 25
	JZ	.L13
	JMP	.L14
.L13:
	MOV	AX, [BP+6]
	PUSH	AX
	MOV	AX, [BP+8]
	POP	CX
	XCHG	AX, CX
	CWD
	IDIV	CX
	MOV	AX, DX
	JMP	.L0
.L14:
.L15:
	MOV	AX, [BP+4]
	CMP	AX, 26
	JZ	.L16
	JMP	.L17
.L16:
	MOV	AX, [BP+6]
	PUSH	AX
	MOV	AX, [BP+8]
	POP	CX
	AND	AX, CX
	JMP	.L0
.L17:
.L18:
	MOV	AX, [BP+4]
	CMP	AX, 27
	JZ	.L19
	JMP	.L20
.L19:
	MOV	AX, [BP+6]
	PUSH	AX
	MOV	AX, [BP+8]
	POP	CX
	XOR	AX, CX
	JMP	.L0
.L20:
.L21:
	MOV	AX, [BP+4]
	CMP	AX, 28
	JZ	.L22
	JMP	.L23
.L22:
	MOV	AX, [BP+6]
	PUSH	AX
	MOV	AX, [BP+8]
	POP	CX
	OR	AX, CX
	JMP	.L0
.L23:
.L24:
	MOV	AX, [BP+4]
	CMP	AX, 31
	JZ	.L25
	JMP	.L26
.L25:
	MOV	AX, [BP+6]
	PUSH	AX
	MOV	AX, [BP+8]
	POP	CX
	XCHG	AX, CX
	SHL	AX, CL
	JMP	.L0
.L26:
.L27:
	MOV	AX, [BP+4]
	CMP	AX, 32
	JZ	.L28
	JMP	.L29
.L28:
	MOV	AX, [BP+6]
	PUSH	AX
	MOV	AX, [BP+8]
	POP	CX
	XCHG	AX, CX
	SAR	AX, CL
	JMP	.L0
.L29:
.L30:
	ADD	SP, -8
	MOV	WORD [BP-8], 0
	CALL	_Check
	ADD	SP, 8
.L0:
	POP	BP
	RET
_OpCommutes:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	CMP	AX, 19
	JNZ	.L1
	MOV	AX, 1
	JMP	.L2
.L1:
	MOV	AX, [BP+4]
	CMP	AX, 23
	MOV	AX, 1
	JZ	.L3
	DEC	AL
.L3:
.L2:
	AND	AX, AX
	JZ	.L4
	JMP	.L5
.L4:
	MOV	AX, [BP+4]
	CMP	AX, 11
	MOV	AX, 1
	JZ	.L6
	DEC	AL
.L6:
.L5:
	AND	AX, AX
	JZ	.L7
	JMP	.L8
.L7:
	MOV	AX, [BP+4]
	CMP	AX, 14
	MOV	AX, 1
	JZ	.L9
	DEC	AL
.L9:
.L8:
	AND	AX, AX
	JZ	.L10
	JMP	.L11
.L10:
	MOV	AX, [BP+4]
	CMP	AX, 26
	MOV	AX, 1
	JZ	.L12
	DEC	AL
.L12:
.L11:
	AND	AX, AX
	JZ	.L13
	JMP	.L14
.L13:
	MOV	AX, [BP+4]
	CMP	AX, 27
	MOV	AX, 1
	JZ	.L15
	DEC	AL
.L15:
.L14:
	AND	AX, AX
	JZ	.L16
	JMP	.L17
.L16:
	MOV	AX, [BP+4]
	CMP	AX, 28
	MOV	AX, 1
	JZ	.L18
	DEC	AL
.L18:
.L17:
	JMP	.L0
.L0:
	POP	BP
	RET
_ParseExpr1:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = LEnd
	PUSH	AX	; [BP-4] = Temp
	PUSH	AX	; [BP-6] = Op
	PUSH	AX	; [BP-8] = Prec
	PUSH	AX	; [BP-10] = IsAssign
	PUSH	AX	; [BP-12] = LhsType
	PUSH	AX	; [BP-14] = LhsVal
	PUSH	AX	; [BP-16] = LhsLoc
.L1:
	JMP	.L2
.L2:
	MOV	AX, [_TokenType]
	MOV	[BP-6], AX
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-24], AX
	CALL	_OperatorPrecedence
	ADD	SP, 8
	MOV	[BP-8], AX
	MOV	AX, [BP-8]
	PUSH	AX
	MOV	AX, [BP+4]
	POP	CX
	XCHG	AX, CX
	CMP	AX, CX
	JG	.L4
	JMP	.L5
.L4:
	JMP	.L3
.L5:
.L6:
	CALL	_GetToken
	MOV	AX, [BP-8]
	CMP	AX, 14
	MOV	AX, 1
	JZ	.L7
	DEC	AL
.L7:
	MOV	[BP-10], AX
	MOV	AX, [BP-10]
	AND	AX, AX
	JNZ	.L8
	JMP	.L9
.L8:
	MOV	AX, [_CurrentType]
	AND	AX, 4
	AND	AX, AX
	JZ	.L11
	JMP	.L12
.L11:
	JMP	.L16
.L15:	DB 'L-value required', 0
.L16:
	ADD	SP, -8
	MOV	AX, .L15
	MOV	[BP-24], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L13
.L12:
.L13:
	MOV	AX, [BP-6]
	CMP	AX, 11
	JNZ	.L17
	JMP	.L18
.L17:
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-24], AX
	CALL	_RemoveAssign
	ADD	SP, 8
	MOV	[BP-6], AX
	JMP	.L19
.L18:
.L19:
	MOV	AX, [_CurrentType]
	AND	AX, -5
	MOV	[_CurrentType], AX
	JMP	.L10
.L9:
	CALL	_LvalToRval
.L10:
	MOV	AX, [_CurrentType]
	MOV	[BP-12], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-14], AX
	MOV	AX, [BP-6]
	CMP	AX, 29
	JNZ	.L23
	MOV	AX, 1
	JMP	.L24
.L23:
	MOV	AX, [BP-6]
	CMP	AX, 30
	MOV	AX, 1
	JZ	.L25
	DEC	AL
.L25:
.L24:
	AND	AX, AX
	JNZ	.L20
	JMP	.L21
.L20:
	PUSH	AX	; [BP-18] = JText
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	MOV	[BP-4], AX
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	MOV	[BP-2], AX
	MOV	AX, [_CurrentType]
	CMP	AX, 1
	JZ	.L26
	JMP	.L27
.L26:
	MOV	AX, [BP-6]
	CMP	AX, 29
	JNZ	.L29
	JMP	.L30
.L29:
	MOV	AX, [_CurrentVal]
	XOR	AX, 1
	MOV	[_CurrentVal], AX
	JMP	.L31
.L30:
.L31:
	JMP	.L33
.L32:	DB '%s', 9, '.L%d', 0
.L33:
	ADD	SP, -8
	MOV	AX, .L32
	MOV	[BP-26], AX
	ADD	SP, -8
	MOV	AX, [_CurrentVal]
	MOV	[BP-34], AX
	CALL	_GetJcc
	ADD	SP, 8
	MOV	[BP-24], AX
	MOV	AX, [BP-4]
	MOV	[BP-22], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L35
.L34:	DB 'MOV', 9, 'AX, %d', 0
.L35:
	ADD	SP, -8
	MOV	AX, .L34
	MOV	[BP-26], AX
	MOV	AX, [BP-6]
	CMP	AX, 29
	MOV	AX, 1
	JNZ	.L36
	DEC	AL
.L36:
	MOV	[BP-24], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L28
.L27:
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 3
	MOV	AX, 1
	JZ	.L37
	DEC	AL
.L37:
	MOV	[BP-26], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L39
.L38:	DB 'AND', 9, 'AX, AX', 0
.L39:
	ADD	SP, -8
	MOV	AX, .L38
	MOV	[BP-26], AX
	CALL	_Emit
	ADD	SP, 8
	MOV	AX, [BP-6]
	CMP	AX, 29
	JZ	.L40
	JMP	.L41
.L40:
	JMP	.L44
.L43:	DB 'JNZ', 0
.L44:
	MOV	AX, .L43
	MOV	[BP-18], AX
	JMP	.L42
.L41:
	JMP	.L46
.L45:	DB 'JZ', 0
.L46:
	MOV	AX, .L45
	MOV	[BP-18], AX
.L42:
	JMP	.L48
.L47:	DB '%s', 9, '.L%d', 0
.L48:
	ADD	SP, -8
	MOV	AX, .L47
	MOV	[BP-26], AX
	MOV	AX, [BP-18]
	MOV	[BP-24], AX
	MOV	AX, [BP-4]
	MOV	[BP-22], AX
	CALL	_Emit
	ADD	SP, 8
.L28:
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-26], AX
	CALL	_EmitJmp
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-4]
	MOV	[BP-26], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	ADD	SP, 2
	JMP	.L22
.L21:
	MOV	WORD [BP-2], -1
	MOV	AX, [BP-12]
	AND	AX, 192
	AND	AX, AX
	JZ	.L49
	JMP	.L50
.L49:
	ADD	SP, -8
	MOV	AX, [_PendingPushAx]
	AND	AX, AX
	MOV	AX, 1
	JZ	.L54
	DEC	AL
.L54:
	MOV	[BP-24], AX
	CALL	_Check
	ADD	SP, 8
	MOV	WORD [_PendingPushAx], 1
	JMP	.L51
.L50:
.L51:
.L22:
	CALL	_ParseCastExpression
.L55:
	JMP	.L56
.L56:
	MOV	AX, [_TokenType]
	PUSH	AX	; [BP-18] = LookAheadOp
	ADD	SP, -8
	MOV	AX, [BP-18]
	MOV	[BP-26], AX
	CALL	_OperatorPrecedence
	ADD	SP, 8
	PUSH	AX	; [BP-20] = LookAheadPrecedence
	MOV	AX, [BP-20]
	PUSH	AX
	MOV	AX, [BP-8]
	POP	CX
	XCHG	AX, CX
	CMP	AX, CX
	JNG	.L61
	MOV	AX, 1
	JMP	.L62
.L61:
	MOV	AX, [BP-20]
	PUSH	AX
	MOV	AX, [BP-8]
	POP	CX
	XCHG	AX, CX
	CMP	AX, CX
	JZ	.L63
	MOV	AX, 0
	JMP	.L64
.L63:
	MOV	AX, [BP-20]
	CMP	AX, 14
	MOV	AX, 1
	JL	.L65
	DEC	AL
.L65:
.L64:
.L62:
	AND	AX, AX
	JNZ	.L58
	JMP	.L59
.L58:
	ADD	SP, 4
	JMP	.L57
.L59:
.L60:
	ADD	SP, -8
	MOV	AX, [BP-20]
	MOV	[BP-28], AX
	CALL	_ParseExpr1
	ADD	SP, 8
	ADD	SP, 4
	JMP	.L55
.L57:
	CALL	_LvalToRval
	MOV	AX, [BP-12]
	AND	AX, 192
	MOV	[BP-16], AX
	MOV	AX, [BP-12]
	AND	AX, -193
	MOV	[BP-12], AX
	MOV	AX, [BP-16]
	CMP	AX, 64
	JZ	.L69
	MOV	AX, 0
	JMP	.L70
.L69:
	MOV	AX, [_CurrentType]
	CMP	AX, 67
	MOV	AX, 1
	JZ	.L71
	DEC	AL
.L71:
.L70:
	AND	AX, AX
	JNZ	.L66
	JMP	.L67
.L66:
	ADD	SP, -8
	MOV	AX, [BP-12]
	CMP	AX, 3
	MOV	AX, 1
	JZ	.L72
	DEC	AL
.L72:
	MOV	[BP-24], AX
	CALL	_Check
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-24], AX
	MOV	AX, [BP-14]
	MOV	[BP-22], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-20], AX
	CALL	_DoConstBinOp
	ADD	SP, 8
	MOV	[_CurrentVal], AX
	JMP	.L1
.L67:
.L68:
	MOV	AX, [BP-2]
	CMP	AX, 0
	JNL	.L73
	JMP	.L74
.L73:
	CALL	_GetVal
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-24], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	JMP	.L75
.L74:
	MOV	AX, [BP-10]
	AND	AX, AX
	JNZ	.L76
	JMP	.L77
.L76:
	MOV	AX, 2
	PUSH	AX	; [BP-18] = Size
	MOV	AX, [BP-12]
	CMP	AX, 2
	JZ	.L79
	JMP	.L80
.L79:
	MOV	WORD [BP-18], 1
	JMP	.L81
.L80:
	ADD	SP, -8
	MOV	AX, [BP-12]
	CMP	AX, 3
	JNZ	.L82
	MOV	AX, 1
	JMP	.L83
.L82:
	MOV	AX, [BP-12]
	AND	AX, 48
.L83:
	MOV	[BP-26], AX
	CALL	_Check
	ADD	SP, 8
.L81:
	MOV	AX, [BP-16]
	AND	AX, AX
	JZ	.L84
	JMP	.L85
.L84:
	MOV	AX, [_PendingPushAx]
	AND	AX, AX
	JNZ	.L88
	JMP	.L89
.L88:
	MOV	WORD [_PendingPushAx], 0
	JMP	.L92
.L91:	DB 'MOV', 9, 'BX, AX', 0
.L92:
	ADD	SP, -8
	MOV	AX, .L91
	MOV	[BP-26], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L90
.L89:
	JMP	.L94
.L93:	DB 'POP', 9, 'BX', 0
.L94:
	ADD	SP, -8
	MOV	AX, .L93
	MOV	[BP-26], AX
	CALL	_Emit
	ADD	SP, 8
.L90:
	JMP	.L86
.L85:
	ADD	SP, -8
	MOV	AX, [BP-16]
	CMP	AX, 128
	JNZ	.L95
	MOV	AX, 1
	JMP	.L96
.L95:
	MOV	AX, [BP-16]
	CMP	AX, 192
	MOV	AX, 1
	JZ	.L97
	DEC	AL
.L97:
.L96:
	MOV	[BP-26], AX
	CALL	_Check
	ADD	SP, 8
.L86:
	MOV	AX, [BP-6]
	CMP	AX, 11
	JNZ	.L98
	JMP	.L99
.L98:
	ADD	SP, -8
	MOV	AX, [BP-12]
	CMP	AX, 3
	JNZ	.L101
	MOV	AX, 1
	JMP	.L102
.L101:
	MOV	AX, [BP-12]
	AND	AX, 18
.L102:
	MOV	[BP-26], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_CurrentType]
	CMP	AX, 67
	MOV	AX, 1
	JZ	.L103
	DEC	AL
.L103:
	MOV	[BP-4], AX
	MOV	AX, [BP-4]
	AND	AX, AX
	JZ	.L104
	JMP	.L105
.L104:
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 3
	MOV	AX, 1
	JZ	.L108
	DEC	AL
.L108:
	MOV	[BP-26], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L110
.L109:	DB 'MOV', 9, 'CX, AX', 0
.L110:
	ADD	SP, -8
	MOV	AX, .L109
	MOV	[BP-26], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L106
.L105:
.L106:
	ADD	SP, -8
	MOV	WORD [BP-26], 2
	MOV	AX, [BP-16]
	MOV	[BP-24], AX
	MOV	AX, [BP-14]
	MOV	[BP-22], AX
	CALL	_EmitLoadAx
	ADD	SP, 8
	MOV	AX, [BP-4]
	AND	AX, AX
	JNZ	.L111
	JMP	.L112
.L111:
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-26], AX
	CALL	_DoRhsConstBinOp
	ADD	SP, 8
	JMP	.L113
.L112:
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-26], AX
	CALL	_DoBinOp
	ADD	SP, 8
.L113:
	JMP	.L100
.L99:
	MOV	AX, [_CurrentType]
	CMP	AX, 67
	JZ	.L114
	JMP	.L115
.L114:
	ADD	SP, -8
	MOV	AX, [BP-18]
	MOV	[BP-26], AX
	MOV	AX, [BP-16]
	MOV	[BP-24], AX
	MOV	AX, [BP-14]
	MOV	[BP-22], AX
	CALL	_EmitStoreConst
	ADD	SP, 8
	ADD	SP, 2
	JMP	.L1
.L115:
	CALL	_GetVal
.L116:
.L100:
	ADD	SP, -8
	MOV	AX, [BP-18]
	MOV	[BP-26], AX
	MOV	AX, [BP-16]
	MOV	[BP-24], AX
	MOV	AX, [BP-14]
	MOV	[BP-22], AX
	CALL	_EmitStoreAx
	ADD	SP, 8
	ADD	SP, 2
	JMP	.L78
.L77:
	MOV	AX, [_CurrentType]
	CMP	AX, 67
	JZ	.L117
	JMP	.L118
.L117:
	ADD	SP, -8
	MOV	AX, [_PendingPushAx]
	MOV	[BP-24], AX
	CALL	_Check
	ADD	SP, 8
	MOV	WORD [_PendingPushAx], 0
	MOV	WORD [_CurrentType], 3
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-24], AX
	CALL	_DoRhsConstBinOp
	ADD	SP, 8
	JMP	.L119
.L118:
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 3
	JNZ	.L120
	MOV	AX, 1
	JMP	.L121
.L120:
	MOV	AX, [BP-6]
	CMP	AX, 21
	JZ	.L122
	MOV	AX, 0
	JMP	.L123
.L122:
	MOV	AX, [_CurrentType]
	AND	AX, 48
.L123:
.L121:
	MOV	[BP-24], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [BP-6]
	CMP	AX, 19
	JZ	.L127
	MOV	AX, 0
	JMP	.L128
.L127:
	MOV	AX, [BP-12]
	AND	AX, 48
.L128:
	AND	AX, AX
	JNZ	.L129
	JMP	.L130
.L129:
	MOV	AX, [BP-12]
	CMP	AX, 18
	MOV	AX, 1
	JNZ	.L131
	DEC	AL
.L131:
.L130:
	AND	AX, AX
	JNZ	.L124
	JMP	.L125
.L124:
	JMP	.L133
.L132:	DB 'ADD', 9, 'AX, AX', 0
.L133:
	ADD	SP, -8
	MOV	AX, .L132
	MOV	[BP-24], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L126
.L125:
.L126:
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-24], AX
	CALL	_OpCommutes
	ADD	SP, 8
	MOV	[BP-4], AX
	MOV	AX, [BP-16]
	CMP	AX, 64
	JZ	.L134
	JMP	.L135
.L134:
	ADD	SP, -8
	MOV	AX, [BP-12]
	CMP	AX, 3
	MOV	AX, 1
	JZ	.L137
	DEC	AL
.L137:
	MOV	[BP-24], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [BP-4]
	AND	AX, AX
	JNZ	.L138
	JMP	.L139
.L138:
	MOV	AX, [BP-14]
	MOV	[_CurrentVal], AX
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-24], AX
	CALL	_DoRhsConstBinOp
	ADD	SP, 8
	JMP	.L1
.L139:
	JMP	.L142
.L141:	DB 'MOV', 9, 'CX, %d', 0
.L142:
	ADD	SP, -8
	MOV	AX, .L141
	MOV	[BP-24], AX
	MOV	AX, [BP-14]
	MOV	[BP-22], AX
	CALL	_Emit
	ADD	SP, 8
.L140:
	JMP	.L136
.L135:
	ADD	SP, -8
	MOV	AX, [BP-12]
	CMP	AX, 3
	JNZ	.L143
	MOV	AX, 1
	JMP	.L144
.L143:
	MOV	AX, [BP-12]
	AND	AX, 48
.L144:
	MOV	[BP-24], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L146
.L145:	DB 'POP', 9, 'CX', 0
.L146:
	ADD	SP, -8
	MOV	AX, .L145
	MOV	[BP-24], AX
	CALL	_Emit
	ADD	SP, 8
.L136:
	MOV	AX, [BP-4]
	AND	AX, AX
	JZ	.L147
	JMP	.L148
.L147:
	JMP	.L152
.L151:	DB 'XCHG', 9, 'AX, CX', 0
.L152:
	ADD	SP, -8
	MOV	AX, .L151
	MOV	[BP-24], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L149
.L148:
.L149:
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-24], AX
	CALL	_DoBinOp
	ADD	SP, 8
.L119:
	MOV	AX, [BP-6]
	CMP	AX, 21
	JZ	.L156
	MOV	AX, 0
	JMP	.L157
.L156:
	MOV	AX, [BP-12]
	AND	AX, 48
.L157:
	AND	AX, AX
	JNZ	.L153
	JMP	.L154
.L153:
	MOV	AX, [BP-12]
	CMP	AX, 18
	JNZ	.L158
	JMP	.L159
.L158:
	JMP	.L162
.L161:	DB 'SAR', 9, 'AX, 1', 0
.L162:
	ADD	SP, -8
	MOV	AX, .L161
	MOV	[BP-24], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L160
.L159:
.L160:
	MOV	WORD [_CurrentType], 3
	JMP	.L155
.L154:
.L155:
.L78:
.L75:
	JMP	.L1
.L3:
	ADD	SP, 16
.L0:
	POP	BP
	RET
_ParseExpr0:
	PUSH	BP
	MOV	BP, SP
	CALL	_ParseCastExpression
	ADD	SP, -8
	MOV	AX, [BP+4]
	MOV	[BP-8], AX
	CALL	_ParseExpr1
	ADD	SP, 8
.L0:
	POP	BP
	RET
_ParseExpr:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	WORD [BP-8], 15
	CALL	_ParseExpr0
	ADD	SP, 8
.L0:
	POP	BP
	RET
_ParseAssignmentExpression:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	WORD [BP-8], 14
	CALL	_ParseExpr0
	ADD	SP, 8
.L0:
	POP	BP
	RET
_ParseDeclSpecs:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	CALL	_IsTypeStart
	MOV	[BP-8], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_TokenType]
	CMP	AX, 48
	JZ	.L1
	JMP	.L2
.L1:
	CALL	_GetToken
	ADD	SP, -8
	CALL	_IsTypeStart
	MOV	[BP-8], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L3
.L2:
.L3:
	PUSH	AX	; [BP-2] = t
	MOV	AX, [_TokenType]
	CMP	AX, 57
	JZ	.L4
	JMP	.L5
.L4:
	MOV	WORD [BP-2], 0
	JMP	.L6
.L5:
	MOV	AX, [_TokenType]
	CMP	AX, 47
	JZ	.L7
	JMP	.L8
.L7:
	MOV	WORD [BP-2], 2
	JMP	.L9
.L8:
	MOV	AX, [_TokenType]
	CMP	AX, 54
	JZ	.L10
	JMP	.L11
.L10:
	MOV	WORD [BP-2], 3
	JMP	.L12
.L11:
	MOV	AX, [_TokenType]
	CMP	AX, 59
	JZ	.L13
	JMP	.L14
.L13:
	CALL	_GetToken
	MOV	AX, 18
	JMP	.L0
.L14:
	CALL	_Unexpected
.L15:
.L12:
.L9:
.L6:
	CALL	_GetToken
.L16:
	MOV	AX, [_TokenType]
	CMP	AX, 23
	JZ	.L17
	JMP	.L18
.L17:
	MOV	AX, [BP-2]
	ADD	AX, 16
	MOV	[BP-2], AX
	CALL	_GetToken
	JMP	.L16
.L18:
	MOV	AX, [BP-2]
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_AddVarDecl:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = vd
	ADD	SP, -8
	MOV	AX, [_ScopesCount]
	MOV	[BP-10], AX
	CALL	_Check
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [_Scopes]
	PUSH	AX
	MOV	AX, [_ScopesCount]
	SUB	AX, 1
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	CMP	AX, 249
	MOV	AX, 1
	JL	.L1
	DEC	AL
.L1:
	MOV	[BP-10], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_Scopes]
	PUSH	AX
	MOV	AX, [_ScopesCount]
	SUB	AX, 1
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	INC	AX
	MOV	[BX], AX
	MOV	[BP-2], AX
	MOV	AX, [_VarDeclType]
	PUSH	AX
	MOV	AX, [BP-2]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	PUSH	AX
	MOV	AX, [BP+4]
	POP	BX
	MOV	[BX], AX
	MOV	AX, [_VarDeclOffset]
	PUSH	AX
	MOV	AX, [BP-2]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	WORD [BX], 0
	MOV	AX, [_VarDeclId]
	PUSH	AX
	MOV	AX, [BP-2]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	PUSH	AX
	MOV	AX, [BP+6]
	POP	BX
	MOV	[BX], AX
	MOV	AX, [BP-2]
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_ParseDecl:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = Type
	PUSH	AX	; [BP-4] = Id
	CALL	_ParseDeclSpecs
	MOV	[BP-2], AX
	CALL	_ExpectId
	MOV	[BP-4], AX
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-12], AX
	MOV	AX, [BP-4]
	MOV	[BP-10], AX
	CALL	_AddVarDecl
	ADD	SP, 8
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_PushScope:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = End
	ADD	SP, -8
	MOV	AX, [_ScopesCount]
	CMP	AX, 10
	MOV	AX, 1
	JL	.L1
	DEC	AL
.L1:
	MOV	[BP-10], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_ScopesCount]
	AND	AX, AX
	JNZ	.L2
	JMP	.L3
.L2:
	MOV	AX, [_Scopes]
	PUSH	AX
	MOV	AX, [_ScopesCount]
	SUB	AX, 1
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[BP-2], AX
	JMP	.L4
.L3:
	MOV	WORD [BP-2], -1
.L4:
	MOV	AX, [_Scopes]
	PUSH	AX
	MOV	AX, [_ScopesCount]
	PUSH	AX
	INC	AX
	MOV	[_ScopesCount], AX
	POP	AX
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	PUSH	AX
	MOV	AX, [BP-2]
	POP	BX
	MOV	[BX], AX
	ADD	SP, 2
.L0:
	POP	BP
	RET
_PopScope:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	AX, [_ScopesCount]
	MOV	[BP-8], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_ScopesCount]
	DEC	AX
	MOV	[_ScopesCount], AX
.L0:
	POP	BP
	RET
_DoCond:
	PUSH	BP
	MOV	BP, SP
	CALL	_ParseExpr
	CALL	_LvalToRval
	MOV	AX, [_CurrentType]
	CMP	AX, 1
	JZ	.L1
	JMP	.L2
.L1:
	JMP	.L5
.L4:	DB '%s', 9, '.L%d', 0
.L5:
	ADD	SP, -8
	MOV	AX, .L4
	MOV	[BP-8], AX
	ADD	SP, -8
	MOV	AX, [_CurrentVal]
	MOV	[BP-16], AX
	CALL	_GetJcc
	ADD	SP, 8
	MOV	[BP-6], AX
	MOV	AX, [BP+4]
	MOV	[BP-4], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L3
.L2:
	CALL	_GetVal
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 3
	MOV	AX, 1
	JZ	.L6
	DEC	AL
.L6:
	MOV	[BP-8], AX
	CALL	_Check
	ADD	SP, 8
	JMP	.L8
.L7:	DB 'AND', 9, 'AX, AX', 0
.L8:
	ADD	SP, -8
	MOV	AX, .L7
	MOV	[BP-8], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L10
.L9:	DB 'JNZ', 9, '.L%d', 0
.L10:
	ADD	SP, -8
	MOV	AX, .L9
	MOV	[BP-8], AX
	MOV	AX, [BP+4]
	MOV	[BP-6], AX
	CALL	_Emit
	ADD	SP, 8
.L3:
	ADD	SP, -8
	MOV	AX, [BP+6]
	MOV	[BP-8], AX
	CALL	_EmitJmp
	ADD	SP, 8
.L0:
	POP	BP
	RET
_ParseStatement:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [_BreakLabel]
	PUSH	AX	; [BP-2] = OldBreak
	MOV	AX, [_ContinueLabel]
	PUSH	AX	; [BP-4] = OldContinue
	ADD	SP, -8
	MOV	WORD [BP-12], 10
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	JMP	.L3
.L2:
	MOV	AX, [_TokenType]
	CMP	AX, 5
	JZ	.L4
	JMP	.L5
.L4:
	CALL	_ParseCompoundStatement
	JMP	.L6
.L5:
	CALL	_IsTypeStart
	AND	AX, AX
	JNZ	.L7
	JMP	.L8
.L7:
	PUSH	AX	; [BP-6] = vd
	CALL	_ParseDecl
	MOV	[BP-6], AX
	ADD	SP, -8
	MOV	WORD [BP-14], 11
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L10
	JMP	.L11
.L10:
	CALL	_ParseAssignmentExpression
	CALL	_LvalToRval
	CALL	_GetVal
	MOV	AX, [_CurrentType]
	CMP	AX, 2
	JZ	.L13
	JMP	.L14
.L13:
	JMP	.L17
.L16:	DB 'CBW', 0
.L17:
	ADD	SP, -8
	MOV	AX, .L16
	MOV	[BP-14], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L15
.L14:
.L15:
	JMP	.L12
.L11:
.L12:
	MOV	AX, [_LocalOffset]
	SUB	AX, 2
	MOV	[_LocalOffset], AX
	MOV	AX, [_VarDeclOffset]
	PUSH	AX
	MOV	AX, [BP-6]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	PUSH	AX
	MOV	AX, [_LocalOffset]
	POP	BX
	MOV	[BX], AX
	JMP	.L19
.L18:	DB 'PUSH', 9, 'AX', 9, '; [BP%+d] = %s', 0
.L19:
	ADD	SP, -8
	MOV	AX, .L18
	MOV	[BP-14], AX
	MOV	AX, [_LocalOffset]
	MOV	[BP-12], AX
	ADD	SP, -8
	MOV	AX, [_VarDeclId]
	PUSH	AX
	MOV	AX, [BP-6]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[BP-22], AX
	CALL	_IdText
	ADD	SP, 8
	MOV	[BP-10], AX
	CALL	_Emit
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-14], 10
	CALL	_Expect
	ADD	SP, 8
	ADD	SP, 2
	JMP	.L9
.L8:
	ADD	SP, -8
	MOV	WORD [BP-12], 52
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L20
	JMP	.L21
.L20:
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	PUSH	AX	; [BP-6] = CondLabel
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	PUSH	AX	; [BP-8] = BodyLabel
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	PUSH	AX	; [BP-10] = EndLabel
	MOV	AX, [BP-6]
	PUSH	AX	; [BP-12] = IterLabel
	ADD	SP, -8
	MOV	WORD [BP-20], 3
	CALL	_Expect
	ADD	SP, 8
	MOV	AX, [_TokenType]
	CMP	AX, 10
	JNZ	.L23
	JMP	.L24
.L23:
	CALL	_ParseExpr
	JMP	.L25
.L24:
.L25:
	ADD	SP, -8
	MOV	WORD [BP-20], 10
	CALL	_Expect
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-20], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	MOV	AX, [_TokenType]
	CMP	AX, 10
	JNZ	.L26
	JMP	.L27
.L26:
	ADD	SP, -8
	MOV	AX, [BP-8]
	MOV	[BP-20], AX
	MOV	AX, [BP-10]
	MOV	[BP-18], AX
	CALL	_DoCond
	ADD	SP, 8
	JMP	.L28
.L27:
	ADD	SP, -8
	MOV	AX, [BP-8]
	MOV	[BP-20], AX
	CALL	_EmitJmp
	ADD	SP, 8
.L28:
	ADD	SP, -8
	MOV	WORD [BP-20], 10
	CALL	_Expect
	ADD	SP, 8
	MOV	AX, [_TokenType]
	CMP	AX, 4
	JNZ	.L29
	JMP	.L30
.L29:
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	MOV	[BP-12], AX
	ADD	SP, -8
	MOV	AX, [BP-12]
	MOV	[BP-20], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	CALL	_ParseExpr
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-20], AX
	CALL	_EmitJmp
	ADD	SP, 8
	JMP	.L31
.L30:
.L31:
	ADD	SP, -8
	MOV	WORD [BP-20], 4
	CALL	_Expect
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-8]
	MOV	[BP-20], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	MOV	AX, [BP-10]
	MOV	[_BreakLabel], AX
	MOV	AX, [BP-12]
	MOV	[_ContinueLabel], AX
	MOV	AX, [_BCStackLevel]
	PUSH	AX	; [BP-14] = OldBCStack
	MOV	AX, [_LocalOffset]
	MOV	[_BCStackLevel], AX
	CALL	_ParseStatement
	MOV	AX, [BP-14]
	MOV	[_BCStackLevel], AX
	ADD	SP, -8
	MOV	AX, [BP-12]
	MOV	[BP-22], AX
	CALL	_EmitJmp
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-10]
	MOV	[BP-22], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	ADD	SP, 10
	JMP	.L22
.L21:
	ADD	SP, -8
	MOV	WORD [BP-12], 53
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L32
	JMP	.L33
.L32:
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	PUSH	AX	; [BP-6] = IfLabel
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	PUSH	AX	; [BP-8] = ElseLabel
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	PUSH	AX	; [BP-10] = EndLabel
	ADD	SP, -8
	MOV	WORD [BP-18], 3
	CALL	_Accept
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-18], AX
	MOV	AX, [BP-8]
	MOV	[BP-16], AX
	CALL	_DoCond
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-18], 4
	CALL	_Accept
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-18], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	CALL	_ParseStatement
	ADD	SP, -8
	MOV	AX, [BP-10]
	MOV	[BP-18], AX
	CALL	_EmitJmp
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-8]
	MOV	[BP-18], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-18], 50
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L35
	JMP	.L36
.L35:
	CALL	_ParseStatement
	JMP	.L37
.L36:
.L37:
	ADD	SP, -8
	MOV	AX, [BP-10]
	MOV	[BP-18], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	ADD	SP, 6
	JMP	.L34
.L33:
	ADD	SP, -8
	MOV	WORD [BP-12], 55
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L38
	JMP	.L39
.L38:
	MOV	AX, [_TokenType]
	CMP	AX, 10
	JNZ	.L41
	JMP	.L42
.L41:
	CALL	_ParseExpr
	CALL	_LvalToRval
	CALL	_GetVal
	JMP	.L43
.L42:
.L43:
	MOV	AX, [_LocalOffset]
	AND	AX, AX
	JNZ	.L44
	JMP	.L45
.L44:
	MOV	WORD [_ReturnUsed], 1
	JMP	.L46
.L45:
.L46:
	ADD	SP, -8
	MOV	AX, [_ReturnLabel]
	MOV	[BP-12], AX
	CALL	_EmitJmp
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-12], 10
	CALL	_Expect
	ADD	SP, 8
	JMP	.L40
.L39:
	ADD	SP, -8
	MOV	WORD [BP-12], 58
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L47
	JMP	.L48
.L47:
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	PUSH	AX	; [BP-6] = StartLabel
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	PUSH	AX	; [BP-8] = BodyLabel
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	PUSH	AX	; [BP-10] = EndLabel
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-18], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-18], 3
	CALL	_Expect
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-8]
	MOV	[BP-18], AX
	MOV	AX, [BP-10]
	MOV	[BP-16], AX
	CALL	_DoCond
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-18], 4
	CALL	_Expect
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-8]
	MOV	[BP-18], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	MOV	AX, [BP-10]
	MOV	[_BreakLabel], AX
	MOV	AX, [BP-6]
	MOV	[_ContinueLabel], AX
	MOV	AX, [_BCStackLevel]
	PUSH	AX	; [BP-12] = OldBCStack
	MOV	AX, [_LocalOffset]
	MOV	[_BCStackLevel], AX
	CALL	_ParseStatement
	MOV	AX, [BP-12]
	MOV	[_BCStackLevel], AX
	ADD	SP, -8
	MOV	AX, [BP-6]
	MOV	[BP-20], AX
	CALL	_EmitJmp
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-10]
	MOV	[BP-20], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	ADD	SP, 8
	JMP	.L49
.L48:
	ADD	SP, -8
	MOV	WORD [BP-12], 46
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L50
	JMP	.L51
.L50:
	ADD	SP, -8
	MOV	AX, [_BreakLabel]
	CMP	AX, 0
	MOV	AX, 1
	JNL	.L53
	DEC	AL
.L53:
	MOV	[BP-12], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_LocalOffset]
	PUSH	AX
	MOV	AX, [_BCStackLevel]
	POP	CX
	CMP	AX, CX
	JNZ	.L54
	JMP	.L55
.L54:
	ADD	SP, -8
	MOV	AX, [_BCStackLevel]
	PUSH	AX
	MOV	AX, [_LocalOffset]
	POP	CX
	XCHG	AX, CX
	SUB	AX, CX
	MOV	[BP-12], AX
	CALL	_EmitAdjSp
	ADD	SP, 8
	JMP	.L56
.L55:
.L56:
	ADD	SP, -8
	MOV	AX, [_BreakLabel]
	MOV	[BP-12], AX
	CALL	_EmitJmp
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-12], 10
	CALL	_Expect
	ADD	SP, 8
	JMP	.L52
.L51:
	ADD	SP, -8
	MOV	WORD [BP-12], 49
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L57
	JMP	.L58
.L57:
	ADD	SP, -8
	MOV	AX, [_ContinueLabel]
	CMP	AX, 0
	MOV	AX, 1
	JNL	.L60
	DEC	AL
.L60:
	MOV	[BP-12], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_LocalOffset]
	PUSH	AX
	MOV	AX, [_BCStackLevel]
	POP	CX
	CMP	AX, CX
	JNZ	.L61
	JMP	.L62
.L61:
	ADD	SP, -8
	MOV	AX, [_BCStackLevel]
	PUSH	AX
	MOV	AX, [_LocalOffset]
	POP	CX
	XCHG	AX, CX
	SUB	AX, CX
	MOV	[BP-12], AX
	CALL	_EmitAdjSp
	ADD	SP, 8
	JMP	.L63
.L62:
.L63:
	ADD	SP, -8
	MOV	AX, [_ContinueLabel]
	MOV	[BP-12], AX
	CALL	_EmitJmp
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-12], 10
	CALL	_Expect
	ADD	SP, 8
	JMP	.L59
.L58:
	CALL	_ParseExpr
	ADD	SP, -8
	MOV	WORD [BP-12], 10
	CALL	_Expect
	ADD	SP, 8
.L59:
.L52:
.L49:
.L40:
.L34:
.L22:
.L9:
.L6:
.L3:
	MOV	AX, [BP-2]
	MOV	[_BreakLabel], AX
	MOV	AX, [BP-4]
	MOV	[_ContinueLabel], AX
	ADD	SP, 4
.L0:
	POP	BP
	RET
_ParseCompoundStatement:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [_LocalOffset]
	PUSH	AX	; [BP-2] = InitialOffset
	CALL	_PushScope
	ADD	SP, -8
	MOV	WORD [BP-10], 5
	CALL	_Expect
	ADD	SP, 8
.L1:
	ADD	SP, -8
	MOV	WORD [BP-10], 6
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JZ	.L2
	JMP	.L3
.L2:
	CALL	_ParseStatement
	JMP	.L1
.L3:
	CALL	_PopScope
	MOV	AX, [BP-2]
	PUSH	AX
	MOV	AX, [_LocalOffset]
	POP	CX
	CMP	AX, CX
	JNZ	.L5
	JMP	.L6
.L5:
	ADD	SP, -8
	MOV	AX, [BP-2]
	PUSH	AX
	MOV	AX, [_LocalOffset]
	POP	CX
	XCHG	AX, CX
	SUB	AX, CX
	MOV	[BP-10], AX
	CALL	_EmitAdjSp
	ADD	SP, 8
	MOV	AX, [BP-2]
	MOV	[_LocalOffset], AX
	JMP	.L7
.L6:
.L7:
	ADD	SP, 2
.L0:
	POP	BP
	RET
_EmitGlobal:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	AX, [_VarDeclId]
	PUSH	AX
	MOV	AX, [BP+4]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[BP-8], AX
	CALL	_EmitGlobalLabel
	ADD	SP, 8
	JMP	.L2
.L1:	DB 'DW', 9, '%d', 0
.L2:
	ADD	SP, -8
	MOV	AX, .L1
	MOV	[BP-8], AX
	MOV	AX, [BP+6]
	MOV	[BP-6], AX
	CALL	_Emit
	ADD	SP, 8
.L0:
	POP	BP
	RET
_ParseExternalDefition:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	WORD [BP-8], 51
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	PUSH	AX	; [BP-2] = id
	PUSH	AX	; [BP-4] = vd
	ADD	SP, -8
	MOV	WORD [BP-12], 5
	CALL	_Expect
	ADD	SP, 8
	MOV	AX, 0
	PUSH	AX	; [BP-6] = EnumVal
.L4:
	MOV	AX, [_TokenType]
	CMP	AX, 6
	JNZ	.L5
	JMP	.L6
.L5:
	CALL	_ExpectId
	MOV	[BP-2], AX
	ADD	SP, -8
	MOV	WORD [BP-14], 11
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L7
	JMP	.L8
.L7:
	CALL	_ParseAssignmentExpression
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 67
	MOV	AX, 1
	JZ	.L10
	DEC	AL
.L10:
	MOV	[BP-14], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [_CurrentVal]
	MOV	[BP-6], AX
	JMP	.L9
.L8:
.L9:
	ADD	SP, -8
	MOV	WORD [BP-14], 67
	MOV	AX, [BP-2]
	MOV	[BP-12], AX
	CALL	_AddVarDecl
	ADD	SP, 8
	MOV	[BP-4], AX
	MOV	AX, [_VarDeclOffset]
	PUSH	AX
	MOV	AX, [BP-4]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	PUSH	AX
	MOV	AX, [BP-6]
	POP	BX
	MOV	[BX], AX
	ADD	SP, -8
	MOV	WORD [BP-14], 9
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JZ	.L11
	JMP	.L12
.L11:
	JMP	.L6
.L12:
.L13:
	MOV	AX, [BP-6]
	INC	AX
	MOV	[BP-6], AX
	JMP	.L4
.L6:
	ADD	SP, -8
	MOV	WORD [BP-14], 6
	CALL	_Expect
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-14], 10
	CALL	_Expect
	ADD	SP, 8
	JMP	.L0
.L2:
.L3:
	CALL	_ParseDecl
	PUSH	AX	; [BP-2] = fd
	ADD	SP, -8
	MOV	WORD [BP-10], 11
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L15
	JMP	.L16
.L15:
	CALL	_ParseAssignmentExpression
	ADD	SP, -8
	MOV	AX, [_CurrentType]
	CMP	AX, 67
	MOV	AX, 1
	JZ	.L18
	DEC	AL
.L18:
	MOV	[BP-10], AX
	CALL	_Check
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-10], AX
	MOV	AX, [_CurrentVal]
	MOV	[BP-8], AX
	CALL	_EmitGlobal
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-10], 10
	CALL	_Expect
	ADD	SP, 8
	JMP	.L0
.L16:
	ADD	SP, -8
	MOV	WORD [BP-10], 10
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L19
	JMP	.L20
.L19:
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-10], AX
	MOV	WORD [BP-8], 0
	CALL	_EmitGlobal
	ADD	SP, 8
	JMP	.L0
.L20:
.L21:
.L17:
	ADD	SP, -8
	MOV	WORD [BP-10], 3
	CALL	_Expect
	ADD	SP, 8
	MOV	AX, [_VarDeclType]
	PUSH	AX
	MOV	AX, [BP-2]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	OR	AX, 8
	MOV	[BX], AX
	CALL	_PushScope
	PUSH	AX	; [BP-4] = ArgOffset
	PUSH	AX	; [BP-6] = vd
	MOV	WORD [BP-4], 4
.L22:
	MOV	AX, [_TokenType]
	CMP	AX, 4
	JNZ	.L23
	JMP	.L24
.L23:
	ADD	SP, -8
	MOV	WORD [BP-14], 57
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JZ	.L28
	JMP	.L29
.L28:
	ADD	SP, -8
	MOV	WORD [BP-14], 45
	CALL	_Accept
	ADD	SP, 8
.L29:
	AND	AX, AX
	JNZ	.L25
	JMP	.L26
.L25:
	JMP	.L24
.L26:
.L27:
	CALL	_ParseDecl
	MOV	[BP-6], AX
	MOV	AX, [_VarDeclOffset]
	PUSH	AX
	MOV	AX, [BP-6]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	PUSH	AX
	MOV	AX, [BP-4]
	POP	BX
	MOV	[BX], AX
	MOV	AX, [BP-4]
	ADD	AX, 2
	MOV	[BP-4], AX
	ADD	SP, -8
	MOV	WORD [BP-14], 9
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JZ	.L30
	JMP	.L31
.L30:
	JMP	.L24
.L31:
.L32:
	JMP	.L22
.L24:
	ADD	SP, -8
	MOV	WORD [BP-14], 4
	CALL	_Expect
	ADD	SP, 8
	ADD	SP, -8
	MOV	WORD [BP-14], 10
	CALL	_Accept
	ADD	SP, 8
	AND	AX, AX
	JZ	.L34
	JMP	.L35
.L34:
	MOV	WORD [_LocalOffset], 0
	MOV	WORD [_ReturnUsed], 0
	MOV	WORD [_LocalLabelCounter], 0
	MOV	AX, [_LocalLabelCounter]
	PUSH	AX
	INC	AX
	MOV	[_LocalLabelCounter], AX
	POP	AX
	MOV	[_ReturnLabel], AX
	MOV	WORD [_ContinueLabel], -1
	MOV	WORD [_BreakLabel], -1
	ADD	SP, -8
	MOV	AX, [_VarDeclId]
	PUSH	AX
	MOV	AX, [BP-2]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[BP-14], AX
	CALL	_EmitGlobalLabel
	ADD	SP, 8
	JMP	.L39
.L38:	DB 'PUSH', 9, 'BP', 0
.L39:
	ADD	SP, -8
	MOV	AX, .L38
	MOV	[BP-14], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L41
.L40:	DB 'MOV', 9, 'BP, SP', 0
.L41:
	ADD	SP, -8
	MOV	AX, .L40
	MOV	[BP-14], AX
	CALL	_Emit
	ADD	SP, 8
	CALL	_ParseCompoundStatement
	ADD	SP, -8
	MOV	AX, [_ReturnLabel]
	MOV	[BP-14], AX
	CALL	_EmitLocalLabel
	ADD	SP, 8
	MOV	AX, [_ReturnUsed]
	AND	AX, AX
	JNZ	.L42
	JMP	.L43
.L42:
	JMP	.L46
.L45:	DB 'MOV', 9, 'SP, BP', 0
.L46:
	ADD	SP, -8
	MOV	AX, .L45
	MOV	[BP-14], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L44
.L43:
.L44:
	JMP	.L48
.L47:	DB 'POP', 9, 'BP', 0
.L48:
	ADD	SP, -8
	MOV	AX, .L47
	MOV	[BP-14], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L50
.L49:	DB 'RET', 0
.L50:
	ADD	SP, -8
	MOV	AX, .L49
	MOV	[BP-14], AX
	CALL	_Emit
	ADD	SP, 8
	JMP	.L36
.L35:
.L36:
	CALL	_PopScope
	ADD	SP, 6
.L0:
	MOV	SP, BP
	POP	BP
	RET
_HeapStart:
	DW	0
_malloc:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ret
	MOV	AX, [_HeapStart]
	MOV	[BP-2], AX
	MOV	AX, [BP+4]
	MOV	CX, AX
	MOV	AX, [_HeapStart]
	ADD	AX, CX
	MOV	[_HeapStart], AX
	ADD	SP, -8
	MOV	AX, [_HeapStart]
	CMP	AX, 32767
	MOV	AX, 1
	JL	.L1
	DEC	AL
.L1:
	MOV	[BP-10], AX
	CALL	_Check
	ADD	SP, 8
	MOV	AX, [BP-2]
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_exit:
	PUSH	BP
	MOV	BP, SP
	MOV	AX, [BP+4]
	AND	AX, 255
	OR	AX, 19456
	MOV	[BP+4], AX
	ADD	SP, -8
	LEA	AX, [BP+4]
	MOV	[BP-8], AX
	MOV	WORD [BP-6], 0
	MOV	WORD [BP-4], 0
	MOV	WORD [BP-2], 0
	CALL	_DosCall
	ADD	SP, 8
.L0:
	POP	BP
	RET
_putchar:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ax
	MOV	WORD [BP-2], 512
	ADD	SP, -8
	LEA	AX, [BP-2]
	MOV	[BP-10], AX
	MOV	WORD [BP-8], 0
	MOV	WORD [BP-6], 0
	MOV	AX, [BP+4]
	MOV	[BP-4], AX
	CALL	_DosCall
	ADD	SP, 8
	ADD	SP, 2
.L0:
	POP	BP
	RET
_open:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ax
	MOV	AX, [BP+6]
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	MOV	WORD [BP-2], 15360
	JMP	.L3
.L2:
	MOV	WORD [BP-2], 15616
.L3:
	ADD	SP, -8
	LEA	AX, [BP-2]
	MOV	[BP-10], AX
	MOV	WORD [BP-8], 0
	MOV	WORD [BP-6], 0
	MOV	AX, [BP+4]
	MOV	[BP-4], AX
	CALL	_DosCall
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L4
	JMP	.L5
.L4:
	MOV	AX, -1
	JMP	.L0
.L5:
.L6:
	MOV	AX, [BP-2]
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_close:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ax
	MOV	WORD [BP-2], 15872
	ADD	SP, -8
	LEA	AX, [BP-2]
	MOV	[BP-10], AX
	MOV	AX, [BP+4]
	MOV	[BP-8], AX
	MOV	WORD [BP-6], 0
	MOV	WORD [BP-4], 0
	CALL	_DosCall
	ADD	SP, 8
	ADD	SP, 2
.L0:
	POP	BP
	RET
_read:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ax
	MOV	WORD [BP-2], 16128
	ADD	SP, -8
	LEA	AX, [BP-2]
	MOV	[BP-10], AX
	MOV	AX, [BP+4]
	MOV	[BP-8], AX
	MOV	AX, [BP+8]
	MOV	[BP-6], AX
	MOV	AX, [BP+6]
	MOV	[BP-4], AX
	CALL	_DosCall
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	MOV	AX, 0
	JMP	.L0
.L2:
.L3:
	MOV	AX, [BP-2]
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_write:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ax
	MOV	WORD [BP-2], 16384
	ADD	SP, -8
	LEA	AX, [BP-2]
	MOV	[BP-10], AX
	MOV	AX, [BP+4]
	MOV	[BP-8], AX
	MOV	AX, [BP+8]
	MOV	[BP-6], AX
	MOV	AX, [BP+6]
	MOV	[BP-4], AX
	CALL	_DosCall
	ADD	SP, 8
	AND	AX, AX
	JNZ	.L1
	JMP	.L2
.L1:
	MOV	AX, 0
	JMP	.L0
.L2:
.L3:
	MOV	AX, [BP-2]
	JMP	.L0
.L0:
	MOV	SP, BP
	POP	BP
	RET
_CallMain:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = Args
	PUSH	AX	; [BP-4] = NumArgs
	MOV	AX, [BP+6]
	PUSH	AX
	MOV	AX, [BP+4]
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	BYTE [BX], 0
	ADD	SP, -8
	MOV	WORD [BP-12], 20
	CALL	_malloc
	ADD	SP, 8
	MOV	[BP-2], AX
	MOV	AX, [BP-2]
	PUSH	AX
	POP	AX
	ADD	AX, 0
	JMP	.L2
.L1:	DB 'scc', 0
.L2:
	PUSH	AX
	MOV	AX, .L1
	POP	BX
	MOV	[BX], AX
	MOV	WORD [BP-4], 1
.L3:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	JNZ	.L4
	JMP	.L5
.L4:
.L6:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	JNZ	.L9
	JMP	.L10
.L9:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	CMP	AX, 32
	MOV	AX, 1
	JNG	.L11
	DEC	AL
.L11:
.L10:
	AND	AX, AX
	JNZ	.L7
	JMP	.L8
.L7:
	MOV	AX, [BP+6]
	INC	AX
	MOV	[BP+6], AX
	JMP	.L6
.L8:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	JZ	.L12
	JMP	.L13
.L12:
	JMP	.L5
.L13:
.L14:
	MOV	AX, [BP-2]
	PUSH	AX
	MOV	AX, [BP-4]
	PUSH	AX
	INC	AX
	MOV	[BP-4], AX
	POP	AX
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	PUSH	AX
	MOV	AX, [BP+6]
	POP	BX
	MOV	[BX], AX
.L16:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	JNZ	.L19
	JMP	.L20
.L19:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	CMP	AX, 32
	MOV	AX, 1
	JG	.L21
	DEC	AL
.L21:
.L20:
	AND	AX, AX
	JNZ	.L17
	JMP	.L18
.L17:
	MOV	AX, [BP+6]
	INC	AX
	MOV	[BP+6], AX
	JMP	.L16
.L18:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	JZ	.L22
	JMP	.L23
.L22:
	JMP	.L5
.L23:
.L24:
	MOV	AX, [BP+6]
	PUSH	AX
	INC	AX
	MOV	[BP+6], AX
	POP	AX
	MOV	BX, AX
	MOV	BYTE [BX], 0
	JMP	.L3
.L5:
	MOV	AX, [BP-2]
	PUSH	AX
	MOV	AX, [BP-4]
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	WORD [BX], 0
	ADD	SP, -8
	ADD	SP, -8
	MOV	AX, [BP-4]
	MOV	[BP-20], AX
	MOV	AX, [BP-2]
	MOV	[BP-18], AX
	CALL	_main
	ADD	SP, 8
	MOV	[BP-12], AX
	CALL	_exit
	ADD	SP, 8
	ADD	SP, 4
.L0:
	POP	BP
	RET
_MakeOutputFilename:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = LastDot
	MOV	WORD [BP-2], 0
.L1:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	AND	AX, AX
	JNZ	.L2
	JMP	.L3
.L2:
	MOV	AX, [BP+6]
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	CMP	AX, 46
	JZ	.L4
	JMP	.L5
.L4:
	MOV	AX, [BP+4]
	MOV	[BP-2], AX
	JMP	.L6
.L5:
.L6:
	MOV	AX, [BP+4]
	PUSH	AX
	INC	AX
	MOV	[BP+4], AX
	POP	AX
	PUSH	AX
	MOV	AX, [BP+6]
	PUSH	AX
	INC	AX
	MOV	[BP+6], AX
	POP	AX
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	POP	BX
	MOV	[BX], AL
	JMP	.L1
.L3:
	MOV	AX, [BP-2]
	AND	AX, AX
	JZ	.L7
	JMP	.L8
.L7:
	MOV	AX, [BP+4]
	MOV	[BP-2], AX
	JMP	.L9
.L8:
.L9:
	ADD	SP, -8
	MOV	AX, [BP-2]
	MOV	[BP-10], AX
	JMP	.L12
.L11:	DB '.asm', 0
.L12:
	MOV	AX, .L11
	MOV	[BP-8], AX
	CALL	_CopyStr
	ADD	SP, 8
	MOV	BX, AX
	MOV	BYTE [BX], 0
	ADD	SP, 2
.L0:
	POP	BP
	RET
_AddBuiltins:
	PUSH	BP
	MOV	BP, SP
	PUSH	AX	; [BP-2] = ch
	MOV	AX, [_IdOffset]
	PUSH	AX
	POP	AX
	ADD	AX, 0
	MOV	BX, AX
	MOV	WORD [BX], 0
.L1:
	JMP	.L2
.L2:
	MOV	AX, [BP+4]
	PUSH	AX
	INC	AX
	MOV	[BP+4], AX
	POP	AX
	MOV	BX, AX
	MOV	AL, [BX]
	CBW
	MOV	[BP-2], AL
	MOV	AL, [BP-2]
	CBW
	CMP	AX, 32
	JNG	.L4
	JMP	.L5
.L4:
	MOV	AX, [_IdBuffer]
	PUSH	AX
	MOV	AX, [_IdBufferIndex]
	PUSH	AX
	INC	AX
	MOV	[_IdBufferIndex], AX
	POP	AX
	POP	CX
	ADD	AX, CX
	MOV	BX, AX
	MOV	BYTE [BX], 0
	MOV	AX, [_IdOffset]
	PUSH	AX
	MOV	AX, [_IdCount]
	INC	AX
	MOV	[_IdCount], AX
	ADD	AX, AX
	POP	CX
	ADD	AX, CX
	PUSH	AX
	MOV	AX, [_IdBufferIndex]
	POP	BX
	MOV	[BX], AX
	MOV	AL, [BP-2]
	CBW
	AND	AX, AX
	JZ	.L7
	JMP	.L8
.L7:
	JMP	.L3
.L8:
.L9:
	JMP	.L6
.L5:
	MOV	AX, [_IdBuffer]
	PUSH	AX
	MOV	AX, [_IdBufferIndex]
	PUSH	AX
	INC	AX
	MOV	[_IdBufferIndex], AX
	POP	AX
	POP	CX
	ADD	AX, CX
	PUSH	AX
	MOV	AL, [BP-2]
	CBW
	POP	BX
	MOV	[BX], AL
.L6:
	JMP	.L1
.L3:
	ADD	SP, -8
	MOV	AX, [_IdCount]
	ADD	AX, 46
	SUB	AX, 1
	CMP	AX, 62
	MOV	AX, 1
	JZ	.L11
	DEC	AL
.L11:
	MOV	[BP-10], AX
	CALL	_Check
	ADD	SP, 8
	ADD	SP, 2
.L0:
	POP	BP
	RET
_main:
	PUSH	BP
	MOV	BP, SP
	ADD	SP, -8
	MOV	WORD [BP-8], 100
	CALL	_malloc
	ADD	SP, 8
	MOV	[_LineBuf], AX
	ADD	SP, -8
	MOV	WORD [BP-8], 32
	CALL	_malloc
	ADD	SP, 8
	MOV	[_TempBuf], AX
	ADD	SP, -8
	MOV	WORD [BP-8], 1024
	CALL	_malloc
	ADD	SP, 8
	MOV	[_InBuf], AX
	ADD	SP, -8
	MOV	WORD [BP-8], 4096
	CALL	_malloc
	ADD	SP, 8
	MOV	[_IdBuffer], AX
	ADD	SP, -8
	MOV	WORD [BP-8], 700
	CALL	_malloc
	ADD	SP, 8
	MOV	[_IdOffset], AX
	ADD	SP, -8
	MOV	WORD [BP-8], 500
	CALL	_malloc
	ADD	SP, 8
	MOV	[_VarDeclId], AX
	ADD	SP, -8
	MOV	WORD [BP-8], 500
	CALL	_malloc
	ADD	SP, 8
	MOV	[_VarDeclType], AX
	ADD	SP, -8
	MOV	WORD [BP-8], 500
	CALL	_malloc
	ADD	SP, 8
	MOV	[_VarDeclOffset], AX
	ADD	SP, -8
	MOV	WORD [BP-8], 20
	CALL	_malloc
	ADD	SP, 8
	MOV	[_Scopes], AX
	MOV	AX, [BP+4]
	CMP	AX, 2
	JL	.L1
	JMP	.L2
.L1:
	JMP	.L5
.L4:	DB 'Usage: %s input-file', 10, 0
.L5:
	ADD	SP, -8
	MOV	AX, .L4
	MOV	[BP-8], AX
	MOV	AX, [BP+6]
	PUSH	AX
	POP	AX
	ADD	AX, 0
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[BP-6], AX
	CALL	_Printf
	ADD	SP, 8
	MOV	AX, 1
	JMP	.L0
.L2:
.L3:
	ADD	SP, -8
	MOV	AX, [BP+6]
	PUSH	AX
	POP	AX
	ADD	AX, 2
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[BP-8], AX
	MOV	WORD [BP-6], 0
	CALL	_open
	ADD	SP, 8
	MOV	[_InFile], AX
	MOV	AX, [_InFile]
	CMP	AX, 0
	JL	.L6
	JMP	.L7
.L6:
	JMP	.L10
.L9:	DB 'Error opening input file', 0
.L10:
	ADD	SP, -8
	MOV	AX, .L9
	MOV	[BP-8], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L8
.L7:
.L8:
	ADD	SP, -8
	MOV	AX, [_IdBuffer]
	MOV	[BP-8], AX
	MOV	AX, [BP+6]
	PUSH	AX
	POP	AX
	ADD	AX, 2
	MOV	BX, AX
	MOV	AX, [BX]
	MOV	[BP-6], AX
	CALL	_MakeOutputFilename
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [_IdBuffer]
	MOV	[BP-8], AX
	MOV	WORD [BP-6], 1
	MOV	WORD [BP-4], 384
	CALL	_open
	ADD	SP, 8
	MOV	[_OutFile], AX
	MOV	AX, [_OutFile]
	CMP	AX, 0
	JL	.L11
	JMP	.L12
.L11:
	JMP	.L15
.L14:	DB 'Error creating output file', 0
.L15:
	ADD	SP, -8
	MOV	AX, .L14
	MOV	[BP-8], AX
	CALL	_Fatal
	ADD	SP, 8
	JMP	.L13
.L12:
.L13:
	JMP	.L17
.L16:	DB 'break char const continue else enum for if int return sizeof void while va_list va_start va_end va_arg', 0
.L17:
	ADD	SP, -8
	MOV	AX, .L16
	MOV	[BP-8], AX
	CALL	_AddBuiltins
	ADD	SP, 8
	JMP	.L19
.L18:	DB 9, 'cpu 8086', 10, 9, 'org 0x100', 10, 'Start:', 10, 9, 'XOR', 9, 'BP, BP', 10, 9, 'MOV', 9, 'WORD [_HeapStart], ProgramEnd', 10, 9, 'MOV', 9, 'AX, 0x81', 10, 9, 'PUSH', 9, 'AX', 10, 9, 'MOV', 9, 'AL, [0x80]', 10, 9, 'PUSH', 9, 'AX', 10, 9, 'PUSH', 9, 'AX', 10, 9, 'JMP', 9, '_CallMain', 10, 10, '_DosCall:', 10, 9, 'PUSH', 9, 'BP', 10, 9, 'MOV', 9, 'BP, SP', 10, 9, 'MOV', 9, 'BX, [BP+4]', 10, 9, 'MOV', 9, 'AX, [BX]', 10, 9, 'MOV', 9, 'BX, [BP+6]', 10, 9, 'MOV', 9, 'CX, [BP+8]', 10, 9, 'MOV', 9, 'DX, [BP+10]', 10, 9, 'INT', 9, '0x21', 10, 9, 'MOV', 9, 'BX, [BP+4]', 10, 9, 'MOV', 9, '[BX], AX', 10, 9, 'MOV', 9, 'AX, 0', 10, 9, 'SBB', 9, 'AX, AX', 10, 9, 'POP', 9, 'BP', 10, 9, 'RET', 10, 10, 0
.L19:
	ADD	SP, -8
	MOV	AX, .L18
	MOV	[BP-8], AX
	CALL	_OutputStr
	ADD	SP, 8
	CALL	_PushScope
	CALL	_GetToken
.L20:
	MOV	AX, [_TokenType]
	CMP	AX, 0
	JNZ	.L21
	JMP	.L22
.L21:
	CALL	_ParseExternalDefition
	JMP	.L20
.L22:
	CALL	_PopScope
	JMP	.L24
.L23:	DB 10, 'ProgramEnd:', 10, 0
.L24:
	ADD	SP, -8
	MOV	AX, .L23
	MOV	[BP-8], AX
	CALL	_OutputStr
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [_InFile]
	MOV	[BP-8], AX
	CALL	_close
	ADD	SP, 8
	ADD	SP, -8
	MOV	AX, [_OutFile]
	MOV	[BP-8], AX
	CALL	_close
	ADD	SP, 8
	MOV	AX, 0
	JMP	.L0
.L0:
	POP	BP
	RET

ProgramEnd:
