;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; SASM - Simple/Stupid/Self-hosting Assembler   ;;
;;        for 16-bit x86 DOS-like Systems.       ;;
;;                                               ;;
;; Copyright 2019 Michael Rasmussen              ;;
;; See LICENSE.md for details                    ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; The primary purpose of this assembler is to be able
;; to assemble itself running only under software assembled
;; with itself (excluding the BIOS). DOS is of course the
;; obvious choice as a bootstrapping environment and .COM
;; files are easy to work with, so that's the basis.
;;
;; While the accepted syntax should be a strict subset of
;; what NASM allows and subsequently outputs, there are
;; some important differences (list not exhaustive):
;;   * SASM is a one-pass assembler and only performs very
;;     basic optimizations.
;;   * Literal expression support is pretty basic,
;;     especially in memory operands where only addition
;;     and subtraction are supported.
;;   * The only supported instructions are those that
;;     were at some point needed. This can include
;;     some operand types not being supported.
;;   * NASM warns about resb/resw in code sections and
;;     ouputs zeros while SASM doesn't output anything
;;     for trailing resb/resw's.
;;   * Error checking is limited, and SASM might emit
;;     garbage opcode bytes when encountering an
;;     illegal instruction (like ADD ES, 4). Check the
;;     code with the C version or even better with NASM
;;     from time to time.
;;   * Non-short jumps aren't synthesized for <386 like
;;     NASM does (e.g. jc FarAway -> jnc $+5/jmp FarAway)
;;
;; Development was generally done by first implementing
;; support for the new instruction/directive/etc. in the
;; accompanying C-version assembler (while being careful
;; to write in a fashion that would be implementable in
;; assembly), and then getting this version up to date.
;;
;; Starting out, it was hard to know whether everything
;; would fit in 64K, so I opted to play it safe and
;; use far pointers in most places I anticipated it could
;; become necessary. This helped stress test the assembler
;; as well as my sanity. That's why some (most) things are
;; dimensioned for sizes that haven't be necessary yet.
;;
;; Assemble using: nasm -f bin -o sasm.com sasm.asm
;; or sasm sasm.asm
;;
;; Calling convention (unless otherwise mentioned):
;;
;;   Callee-saved: DS, BP, SI, DI
;;   Scratch: AX, BX, CX, DX, ES
;;   Return value: (DX:)AX or BX for pointer values,
;;                 boolean values via the carry flag
;;

        cpu 8086
        org 0x100

STACK_SIZE       equ 1024       ; TODO: Figure out correct size..
TOKEN_MAX        equ 16         ; Maximum length of token (adjust token buffer if increasing)
INST_MAX         equ 6          ; Maximum length of directive/instruction (LOOPNE is longest)
BUFFER_SIZE      equ 512        ; Size of input buffer
OUTPUT_MAX       equ 0x2000     ; Maximum output size
LABEL_MAX        equ 300        ; Maximum number of labels
FIXUP_MAX        equ 600        ; Maximum number of fixups
EQU_MAX          equ 100        ; Maximum number of equates
DISPATCH_SIZE    equ INST_MAX+3 ; Size of DispatchListEntry
LABEL_ADDR       equ 18         ; Offset of label address
LABEL_FIXUP      equ 20         ; Offset of label fixup
LABEL_SIZE       equ 22         ; Size of Label (TOKEN_MAX+2+2*sizeof(WORD))
FIXUP_ADDR       equ 0          ; Offset of fixup address (into the output buffer)
FIXUP_LINK       equ 2          ; Offset of fixup link pointer (INVALID_ADDR terminates list)
FIXUP_TYPE       equ 4          ; Offset of fixup type (FIXUP_DISP16 or FIXUP_REL8)
FIXUP_SIZE       equ 5          ; Size of Fixup
EQU_VAL          equ 18         ; Offset of value in equate
EQU_SIZE         equ 20         ; Size of equate (TOKEN_MAX+2+sizeof(WORD))
IF_MAX           equ 8          ; Max %if nesting depth

QUOTE_CHAR       equ 39         ; '\''
HEX_ADJUST       equ  'A'-'0'-10
INVALID_ADDR     equ 0xFFFF

; Value of Operand(L)Type:
; Less than 0xC0: Memory access with ModRM = OperandType
OP_REG          equ 0xC0
OP_LIT          equ 0xC1

; Register indices (OperandValue when OperandType = OP_REG)
; Lower 3 bits matches index, bits 4&5 give class (r8, r16, sreg)
R_AL             equ 0x00
R_CL             equ 0x01
R_DL             equ 0x02
R_BL             equ 0x03
R_AH             equ 0x04
R_CH             equ 0x05
R_DH             equ 0x06
R_BH             equ 0x07
R_AX             equ 0x08
R_CX             equ 0x09
R_DX             equ 0x0a
R_BX             equ 0x0b
R_SP             equ 0x0c
R_BP             equ 0x0d
R_SI             equ 0x0e
R_DI             equ 0x0f
R_ES             equ 0x10
R_CS             equ 0x11
R_SS             equ 0x12
R_DS             equ 0x13

; Condition Codes
CC_O             equ 0x0
CC_NO            equ 0x1
CC_C             equ 0x2
CC_NC            equ 0x3
CC_Z             equ 0x4
CC_NZ            equ 0x5
CC_NA            equ 0x6
CC_A             equ 0x7
CC_S             equ 0x8
CC_NS            equ 0x9
CC_PE            equ 0xa
CC_PO            equ 0xb
CC_L             equ 0xc
CC_NL            equ 0xd
CC_NG            equ 0xe
CC_G             equ 0xf

; Fixup types
FIXUP_DISP16     equ 0
FIXUP_REL8       equ 1

; Flags for %if handling
IF_ACTIVE        equ 1 ; Current %if/%elif/%else is active
IF_WAS_ACTIVE    equ 2 ; A block has previously been active (= skip %elif/%else)
IF_SEEN_ELSE     equ 4 ; %else has been passed, only %endif is legal

ProgramEntry:
        ; Clear BSS
        mov di, BSS
        mov cx, ProgramEnd
        sub cx, di
        xor al, al
        rep stosb

        ; First free paragraph is at the end of the program
        ; COM files get the largest available block
        mov ax, ProgramEnd
        add ax, 15
        and ax, 0xfff0
        add ax, STACK_SIZE
        cli
        mov sp, ax
        sti
        mov cl, 4
        shr ax, cl
        mov bx, cs
        add ax, bx
        mov [FirstFreeSeg], ax

        call Init
        call MainLoop
        call Fini

        xor al, al
        jmp Exit

ParseCommandLine:
        mov cl, [0x80]
        inc cl ; Let count include CR
        mov si, 0x81
        mov di, InFileName
        call CopyFileName

        mov di, OutFileName
        call CopyFileName

        cmp byte [InFileName], 0
        jne .HasIn
        mov word [InFileName], 'A.'
        mov word [InFileName+2], 'AS'
        mov word [InFileName+4], 'M'
.HasIn:
        cmp byte [OutFileName], 0
        jne .Ret
        mov di, OutFileName
        mov si, InFileName
.Copy:
        lodsb
        cmp al, '.'
        jbe .AppendExt
        stosb
        jmp .Copy
.AppendExt:
        mov ax, '.C'
        stosw
        mov ax, 'OM'
        stosw
        xor al, al
        stosb
.Ret:
        ret

CopyFileName:
        and cl, cl
        jz .Done
        cmp byte [si], 0x0D
        je .Done
        ; Skip spaces
.SkipS:
        cmp byte [si], ' '
        jne .NotSpace
        inc si
        dec cl
        jnz .SkipS
        jmp short .Done
.NotSpace:
        mov ch, 12
.Copy:
        cmp byte [si], ' '
        jbe .Done
        movsb
        dec cl
        jz .Done
        dec ch
        jnz .Copy
.Done:
        xor al, al
        stosb
        ret

Init:
        call ParseCommandLine

        mov bx, MsgHello
        call PutString
        mov bx, InFileName
        call PutString
        mov bx, MsgHello2
        call PutString
        mov bx, OutFileName
        call PutString
        call PutCrLf

        mov ax, OUTPUT_MAX
        mov bx, 1
        call Malloc
        mov [OutputSeg], ax

        mov ax, LABEL_MAX
        mov bx, LABEL_SIZE
        call Malloc
        mov [LabelSeg], ax

        mov ax, FIXUP_MAX
        mov bx, FIXUP_SIZE
        call Malloc
        mov [FixupSeg], ax
        ; Initial fixup list
        mov es, ax
        xor bx, bx
        mov cx, FIXUP_MAX
.FixupInit:
        mov ax, bx
        add ax, FIXUP_SIZE
        mov [es:bx+FIXUP_LINK], ax
        mov bx, ax
        dec cx
        jnz .FixupInit
        mov word [es:bx+FIXUP_LINK-FIXUP_SIZE], INVALID_ADDR ; Terminate free list

        mov ax, EQU_MAX
        mov bx, EQU_SIZE
        call Malloc
        mov [EquSeg], ax

        mov byte [CpuLevel], 3

        call ParserInit
        ret

Fini:
        call ParserFini

        ; Check for open %if blocks
        cmp byte [NumIfs], 0
        je .IfsOK
        mov bx, MsgErrIfActive
        jmp short Error

.IfsOK:
        ; Check if there are undefined labels
        mov es, [LabelSeg]
        xor bx, bx
        mov cx, [NumLabels]
        and cx, cx
        jz .Done
.CheckLabels:
        cmp word [es:bx+LABEL_ADDR], INVALID_ADDR
        jne .Next
        call PrintLabel
        push cs
        mov bx, MsgErrLabelUnd
        jmp short Error
.Next:
        add bx, LABEL_SIZE
        dec cx
        jnz .CheckLabels
.Done:
        jmp WriteOutput

NotImplemented:
        mov bx, MsgErrNotImpl
        ; Fall through

; Exit and print error message in BX
Error:
        ; Error in line ...
        push bx
        call PutCrLf
        mov bx, MsgErrInLine
        call PutString
        mov ax, [CurrentLine]
        call PutDec
        mov al, ':'
        call PutChar
        mov al, ' '
        call PutChar
        pop bx
        call PutString
        call PutCrLf
        ; Print curren token
        cmp byte [TokenLen], 0
        jz .NoTok
        mov bx, MsgCurToken
        call PutString
        mov bx, Token
        call PutString
        mov al, '"'
        call PutChar
        call PutCrLf
.NoTok:
        mov al, 0xff
        ; Fall through

; Exit with error code in AL
Exit:
        mov ah, 0x4c
        int 0x21

MainLoop:
        ;
        ; Check if we're in a conditionally disabled block
        ;
        xor bh, bh
        mov bl, [NumIfs]
        sub bl, 1
        jc .NoIfs
        test byte [Ifs+bx], IF_ACTIVE
        jnz .NoIfs
.Skip:
        mov al, [CurrentChar]
        and al, al
        jz .Done
        cmp al, '%'
        je .CheckDir
        call MoveNext
        jmp .Skip
.CheckDir:
        call .GetToken
        mov ax, [UToken+1]
        cmp ax, 'IF'
        je .Dispatch
        cmp ax, 'EL'
        je .Dispatch
        cmp ax, 'EN'
        je .Dispatch
        jmp .Skip
.NoIfs:
        ; Grab next token
        call .GetToken
.Dispatch:
        cmp byte [TokenLen], 0
        je .Done

        ; Dispatch
        call Dispatch

        jmp MainLoop

.Done:
        mov al, [CurrentChar]
        and al, al
        jz .Ret
        push ax
        call PutHexByte
        mov al, ' '
        call PutChar
        pop ax
        call PutChar
        call PutCrLf
        mov bx, MsgErrNotDone
        call Error
.Ret:
        ret
.GetToken:
        ; Update line counter
        xor ax, ax
        xchg ax, [NumNewLines]
        add [CurrentLine], ax
        jmp GetToken

Dispatch:
        push si
        mov al, ':'
        call TryConsume
        jc .NotLabel
        call DefineLabel
        jmp short .Done

.NotLabel:
        mov si, DispatchList

        ; Is the token too short/long to be a valid instruction/directive?
        mov al, [TokenLen]
        cmp al, 2
        jb .CheckEQU
        cmp al, INST_MAX
        ja .CheckEQU

        ; Initialize fixup pointers
        mov ax, INVALID_ADDR
        mov [CurrentFixup], ax
        mov [CurrentLFixup], ax
        ; And ExplicitSize
        mov byte [ExplicitSize], 0

.Compare:
        xor bx, bx
.CLoop:
        mov al, [UToken+bx]
        cmp al, [si+bx]
        jne .Next
        and al, al ; at NUL terminator?
        jz .Match
        inc bl
        cmp bl, INST_MAX
        jb .CLoop

.Match:
        ; Found match, dispatch
        xor ah, ah
        mov al, [si+INST_MAX]
        mov bx, [si+INST_MAX+1]
        call bx
        cmp word [CurrentFixup], INVALID_ADDR
        jne .FixupUnhandled
        cmp word [CurrentLFixup], INVALID_ADDR
        jne .FixupUnhandled
        jmp short .Done
.FixupUnhandled:
        mov bx, MsgErrFixupUnh
        jmp Error

.Next:
        add si, DISPATCH_SIZE
        cmp si, DispatchListEnd
        jb .Compare

.CheckEQU:
        ; Avoid clobbering Token
        mov al, 'E'
        call TryGetU
        jc .Invalid
        mov al, 'Q'
        call TryGetU
        jc .Invalid
        mov al, 'U'
        call TryGetU
        jc .Invalid
        call SkipWS
        call MakeEqu
.Done:
        pop si
        ret
.Invalid:
        mov bx, MsgErrUnknInst
        jmp Error

; Allocate AX*BX bytes, returns segment in AX
Malloc:
        ; Calculate how many paragraphs are needed
        mul bx
        and dx, dx
        jnz .Err ; Overflow
        add ax, 15
        mov cl, 4
        shr ax, cl

        mov cx, [FirstFreeSeg]
        add ax, cx
        cmp ax, [2] ; (PSP) Segment of the first byte beyond the memory allocated to the program
        jae .Err ; Out of memory
        mov [FirstFreeSeg], ax
        mov ax, cx

        ret
.Err:
        mov bx, MsgErrMem
        jmp Error

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Screen Output
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Write character in AL
PutChar:
        mov dl, al
        mov ah, 2
        int 0x21
        ret

; Write CR and LF
PutCrLf:
        mov al, 13
        call PutChar
        mov al, 10
        jmp PutChar

; Write ASCIIZ string in BX
PutString:
        mov al, [bx]
        and al, al
        jz .Done
        push bx
        call PutChar
        pop bx
        inc bx
        jmp PutString
.Done:
        ret

; Write decimal word in AX
PutDec:
        push di
        mov di, sp ; Assumes DS=SS
        sub sp, 6
        dec di
        mov byte [di], 0
        mov bx, 10
.DecLoop:
        xor dx, dx
        div bx
        add dl, '0'
        dec di
        mov [di], dl
        and ax, ax
        jnz .DecLoop
        mov bx, di
        call PutString
        add sp, 6
        pop di
        ret


; Write hexadecimal word in AX
PutHex:
        push ax
        mov al, ah
        call PutHexByte
        pop ax
        ; Fall through

; Write hexadecimal byte in AX
PutHexByte:
        push ax
        shr al, 1
        shr al, 1
        shr al, 1
        shr al, 1
        call PutHexDigit
        pop ax
        ; Fall through

; Write hexadecimal digit in AL
PutHexDigit:
        and al, 0x0f
        add al, '0'
        cmp al, '9'
        jbe PutChar
        add al, HEX_ADJUST
        jmp PutChar

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; File Output
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Write output buffer to file
WriteOutput:
        ; Create file
        mov dx, OutFileName
        mov cx, 0x0020 ; Attributes
        mov ah, 0x3c
        int 0x21
        jc .Error
        mov si, ax ; Save file handle in SI

        ; Write
        mov ah, 0x40
        mov bx, si
        mov cx, [NumOutBytes]
        ; ds:dx -> buffer
        mov dx, [OutputSeg]
        push ds
        mov ds, dx
        xor dx, dx
        int 0x21
        pop ds
        jc .Error
        cmp cx, [NumOutBytes]
        jne .Error

        ; Close file
        mov ax, 0x3e00
        mov bx, si
        int 0x21
        ret
.Error:
        mov bx, MsgErrOutput
        jmp Error


; Output byte in AL to output buffer
; Doesn't modify any registers
OutputByte:
        push cx
        push di
        push es
        mov es, [OutputSeg]
        mov di, [NumOutBytes]
        xor cx, cx
        xchg cx, [PendingZeros]
        and cx, cx
        jnz .Zeros
        cmp word [NumOutBytes], OUTPUT_MAX
        jb .Output
.Overflow:
        mov bx, MsgErrOutMax
        jmp Error
.Output:
        stosb
        inc word [NumOutBytes]
        inc word [CurrentAddress]
        pop es
        pop di
        pop cx
        ret
.Zeros:
        add [NumOutBytes], cx
        cmp word [NumOutBytes], OUTPUT_MAX
        jae .Overflow
        push ax
        xor al, al
        rep stosb
        pop ax
        jmp .Output

; Output word in AX
OutputWord:
        call OutputByte
        mov al, ah ; Only works because OutputByte is friendly
        jmp OutputByte

; Output 8-bit immediate if AL is 0, output 16-bit immediate otherwise
; the immediate is assumed to be in OperandValue
OutputImm:
        and al, al
        jz OutputImm8
        ; Fall through

; Output 16-bit immediate from OperandValue
OutputImm16:
        cmp word [CurrentFixup], INVALID_ADDR
        je .Output
        mov al, FIXUP_DISP16
        call RegisterFixup
.Output:
        mov ax, [OperandValue]
        jmp OutputWord

; Output 8-bit immediate from OperandValue
OutputImm8:
        mov al, [OperandValue]
        jmp OutputByte

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Parser
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ParserInit:
        mov word [CurrentLine], 1

        ; Open file for reading
        mov dx, InFileName
        mov ax, 0x3d00
        int 0x21
        jnc .OK
        mov bx, MsgErrOpenIn
        jmp Error
.OK:
        mov [InputFile], ax
        call FillInBuffer
        call MoveNext
        ret

ParserFini:
        ; Close input file
        mov ax, 0x3e00
        mov bx, [InputFile]
        int 0x21
        ret

FillInBuffer:
        mov ah, 0x3f
        mov bx, [InputFile]
        mov cx, BUFFER_SIZE
        mov dx, InputBuffer
        int 0x21
        jc .ReadError  ; error code in AX
        mov [InputBufferBytes], ax
        mov word [InputBufferPos], 0
        ret

.ReadError:
        mov bx, MsgErrRead
        jmp Error

ReadNext:
        mov bx, [InputBufferPos]
        cmp bx, [InputBufferBytes]
        jb .HasData
        call FillInBuffer
        xor bx, bx
        and ax, ax
        jz .EOF
.HasData:
        mov al, [InputBuffer+bx]
        inc bx
        mov [InputBufferPos], bx
.EOF:
        mov [CurrentChar], al
        cmp al, 10
        jne .Ret
        mov byte [GotNL], 1
        inc word [NumNewLines]
.Ret:
        ret

; Try to get character in AL and ReadNext. Returns carry clear on success.
; I.e. it doesn't skip spaces after consuming the character.
TryGet:
        cmp [CurrentChar], al
        jne .NoMatch
        call ReadNext
        clc
        ret
.NoMatch:
        stc
        ret

; Like TryGet but case insensitive
TryGetU:
        call TryGet
        jc .NoMatch
        ret
.NoMatch:
        or al, ' ' ; ToLower
        jmp TryGet

SkipWS:
        mov al, [CurrentChar]
        and al, al
        jz .Done
        cmp al, ' '
        ja .CheckComment
        call ReadNext
        jmp SkipWS
.CheckComment:
        cmp al, ';'
        jne .Done
.SkipComment:
        call ReadNext
        mov al, [CurrentChar]
        and al, al
        jz .Done
        cmp al, 10
        je SkipWS
        jmp .SkipComment
.Done:
        ret

MoveNext:
        call ReadNext
        jmp SkipWS

; Consume CurrentChar if it matches AL, move next and
; return carry clear. Carry is set ortherwise.
; AL is left unchanged (for Expect)
TryConsume:
        cmp [CurrentChar], al
        jne .NoMatch
        call MoveNext
        clc
        ret
.NoMatch:
        stc
        ret

; Abort if the next character isn't AL
Expect:
        call TryConsume
        jc .Error
        ret
.Error:
        mov bx, MsgErrExpected
        mov [bx], al
        jmp Error

; Get number from Token to AX
GetTokenNumber:
        cmp word [UToken], '0X'
        je .Hex
        ; Decimal number
        xor ax, ax
        mov bx, UToken
        xor ch, ch
.Dec:
        mov cl, [bx]
        and cl, cl
        jnz .NotDone
        ret
.NotDone:
        inc bx
        mov dx, 10
        mul dx
        and dx, dx
        jnz .Error
        sub cl, '0'
        cmp cl, 9
        ja .Error
        add ax, cx
        jmp .Dec
.Hex:
        mov cl, [TokenLen]
        sub cl, 2
        cmp cl, 4
        ja .Error
        xor ax, ax
        mov bx, UToken
        add bx, 2
.HexCvt:
        shl ax, 1
        shl ax, 1
        shl ax, 1
        shl ax, 1
        mov ch, [bx]
        inc bx
        sub ch, '0'
        cmp ch, 9
        jbe .Add
        sub ch, HEX_ADJUST
.Add:
        or al, ch
        dec cl
        jnz .HexCvt
        ret
.Error:
        mov bx, MsgErrInvalidNum
        jmp Error

GetToken:
        push di
        xor di, di
.Get:
        mov al, [CurrentChar]
        mov ah, al
        cmp al, '.'
        je .Store
        cmp al, '_'
        je .Store
        cmp al, '$'
        je .Store
        cmp al, '%'
        je .Store
        ; IsDigit
        mov bh, al
        sub bh, '0'
        cmp bh, 9
        jbe .Store
        mov bh, al
        and bh, 0xDF ; to upper case
        sub bh, 'A'
        cmp bh, 26
        ja .Done
        and ah, 0xDF
.Store:
        cmp di, TOKEN_MAX
        jae .Next ; don't store if beyond limit
        mov [Token+di], al
        mov [UToken+di], ah
        inc di
.Next:
        call ReadNext
        jmp .Get
.Done:
        xor al, al
        mov [Token+di], al ; zero-terminate
        mov [UToken+di], al ; zero-terminate
        mov [IsTokenNumber], al
        mov ax, di
        mov [TokenLen], al
        pop di
        call SkipWS
        mov al, [Token]
        sub al, '0'
        cmp al, 9
        ja .CheckSpecial
        call GetTokenNumber
        jmp short .HasNum
.CheckSpecial:
        cmp word [Token], '$'
        jne .CheckSpecial2
        mov ax, [CurrentAddress]
        jmp short .HasNum
.CheckSpecial2:
        cmp byte [TokenLen], 2
        jne .CheckEqu
        cmp word [Token], '$$'
        jne .CheckEqu
        mov ax, [SectionStart]
        jmp short .HasNum
.CheckEqu:
        call FindEqu
        cmp bx, INVALID_ADDR
        je .NotEqu
        ; Found EQU
        mov ax, [es:bx+EQU_VAL]
.HasNum:
        mov word [OperandValue], ax
        mov byte [IsTokenNumber], 1
.NotEqu:
        ret

; Read one or two character literal to OperandValue
; Assumes initial quote character has been consumed
GetCharLit:
        xor ah, ah
        mov al, [CurrentChar]
        mov [OperandValue], ax
        call ReadNext
        mov al, QUOTE_CHAR
        call TryConsume
        jnc .Done
        mov al, [CurrentChar]
        mov [OperandValue+1], al
        call ReadNext
        mov al, QUOTE_CHAR
        call Expect
.Done:
        ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Literal Expression Parser
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MAX_PREC  equ 0xFF

OPER_LSH  equ '<'|0x80
OPER_RSH  equ '>'|0x80
OPER_LEQ  equ '<'|0xC0
OPER_GEQ  equ '>'|0xC0
OPER_LAND equ '&'|0x80
OPER_LXOR equ '^'|0x80
OPER_LOR  equ '|'|0x80

NewExpr:
        mov word [CurrentOp], 0
        mov byte [GotNL], 0
        ret

; Get operand/precedence pair to CurrentOp
; Preserves all registers
GetOp:
        push ax
        push bx
        push cx
        push dx
        call SkipWS
        cmp byte [GotNL], 0
        jne .NoOp
        mov al, [CurrentChar]
        mov ah, 5
        cmp al, '*'
        je .Ret
        cmp al, '/'
        je .Ret
        cmp al, '%'
        je .Ret
        inc ah
        cmp al, '+'
        je .Ret
        cmp al, '-'
        je .Ret
        mov ah, 7
        cmp al, '<'
        je .LtGt
        cmp al, '>'
        je .LtGt
        mov ah, 10
        cmp al, '='
        je .EqOp
        cmp al, '!'
        je .EqOp
        inc ah
        cmp al, '&'
        je .LogOp
        inc ah
        cmp al, '^'
        je .LogOp
        inc ah
        cmp al, '|'
        je .LogOp
.NoOp:
        xor ax, ax
        jmp short .RetNoRead
.LtGt:
        push ax
        call ReadNext
        pop ax
        cmp [CurrentChar], al
        je .ShiftOp
        mov ah, 9
        cmp byte [CurrentChar], '='
        jne .RetNoRead
        or al, 0xc0
        jmp short .Ret
.ShiftOp:
        or al, 0x80
        jmp short .Ret
.EqOp:
        push ax
        call ReadNext
        pop ax
        cmp byte [CurrentChar], '='
        je .Ret
        mov bx, MsgErrInvalidOp
        jmp Error
.LogOp:
        push ax
        call ReadNext
        pop ax
        cmp byte [CurrentChar], al
        jne .RetNoRead
        or al, 0x80
        add ah, 3
.Ret:
        push ax
        call ReadNext
        pop ax
.RetNoRead:
        mov [CurrentOp], ax
        pop dx
        pop cx
        pop bx
        pop ax
        ret

GetPrimary:
        call SkipWS
        mov al, '('
        call TryConsume
        jc .NotPar
        xor bx, bx
        xchg bx, [CurrentOp]
        push bx
        call GetExpr0
        pop bx
        mov [CurrentOp], bx
        push ax
        mov al, ')'
        call Expect
        pop ax
        ret
.NotPar:
        mov al, QUOTE_CHAR
        call TryGet
        jc .NotCharLit
        call GetCharLit
        mov ax, [OperandValue]
        ret
.NotCharLit:
        call GetToken
        cmp byte [IsTokenNumber], 0
        jne .Number
        call GetNamedLiteral
.Number:
        mov ax, [OperandValue]
        ret

; Returns carry clear if CurrentChar is an unary operator
IsUnaryOp:
        mov al, [CurrentChar]
        cmp al, '+'
        je .Yes
        cmp al, '-'
        je .Yes
        cmp al, '~'
        je .Yes
        cmp al, '!'
        je .Yes
        stc
        ret
.Yes:
        clc
        ret

GetUnary:
        call SkipWS
        call IsUnaryOp
        jc GetPrimary
        mov al, [CurrentChar]
        push ax
        call MoveNext
        call GetUnary
        pop bx
        cmp bl, '+'
        je .Done
        not ax
        cmp bl, '!'
        jne .NotLNot
        and ax, 1
        jmp short .Done
.NotLNot:
        cmp bl, '-'
        jne .Done ; AX holds value for '~'
        inc ax
.Done:
        ret

GetExpr:
        call NewExpr
GetExpr0:
        call GetUnary
        mov bx, MAX_PREC
GetExpr1:
        ; AX = LHS, BL = outer precedence
        mov cx, [CurrentOp]
        and cx, cx
        jnz .HasOp
        call GetOp
        mov cx, [CurrentOp]
        and cx, cx
        jz .Done
        cmp ch, bl ; inner precedence > outer precedence?
        jbe .HasOp
.Done:
        ret
.HasOp:
        ; CH = inner precedence, CL = current op
        push ax ; Save LHS

        push bx
        push cx
        call GetUnary
        pop cx
        pop bx
        ; AX = RHS
.RHSLoop:
        call GetOp
        mov dx, [CurrentOp]
        and dx, dx
        jz .RHSLoopDone
        cmp dh, ch
        jae .RHSLoopDone ; look ahead precedence >= inner precedence
        push bx
        push cx
        xor bh, bh
        mov bl, dh
        call GetExpr1
        pop cx
        pop bx
        jmp .RHSLoop
.RHSLoopDone:
        mov dx, cx ; DX = Operator
        mov cx, ax ; CX = RHS
        pop ax     ; AX = LHS

        call .Compute

        jmp GetExpr1
.Compute:
        ; AX @= CX, where DL contains the operator (@)
        ; Preserves BX and updates AX
        cmp dl, '*'
        je .CMul
        cmp dl, '/'
        je .CDiv
        cmp dl, '%'
        je .CMod
        cmp dl, '+'
        je .CAdd
        cmp dl, '-'
        je .CSub
        cmp dl, OPER_LSH
        je .CLsh
        cmp dl, OPER_RSH
        je .CRsh
        cmp dl, '&'
        je .CBAnd
        cmp dl, '^'
        je .CBXor
        cmp dl, '|'
        je .CBOr
        cmp dl, '='
        je .CEq
        cmp dl, '!'
        je .CNeq
        cmp dl, '<'
        je .CLt
        cmp dl, '>'
        je .CGt
        cmp dl, OPER_LEQ
        je .CLeq
        cmp dl, OPER_GEQ
        je .CGeq
        call .CToBool
        cmp dl, OPER_LAND
        je .CBAnd
        cmp dl, OPER_LXOR
        je .CBXor
        cmp dl, OPER_LOR
        je .CBOr
        mov ah, 2
        int 0x21
        mov bx, MsgErrInvalidOp
        jmp Error
.CMul:
        xor dx, dx
        mul cx
        ret
.CDiv:
        and cx, cx
        jnz .DivOK
        mov bx, MsgErrDivZero
        jmp Error
.DivOK:
        xor dx, dx
        div cx
        ret
.CMod:
        call .CDiv
        mov ax, dx
        ret
.CAdd:
        add ax, cx
        ret
.CSub:
        sub ax, cx
        ret
.CLsh:
        shl ax, cl
        ret
.CRsh:
        shr ax, cl
        ret
.CBAnd:
        and ax, cx
        ret
.CBXor:
        xor ax, cx
        ret
.CBOr:
        or ax, cx
        ret
.CEq:
        cmp ax, cx
        je .COne
        jmp short .CZero
.CNeq:
        cmp ax, cx
        jne .COne
        jmp short .CZero
.CLeq:
        cmp ax, cx
        jbe .COne
        jmp short .CZero
.CGeq:
        cmp ax, cx
        jae .COne
        jmp short .CZero
.CLt:
        cmp ax, cx
        jb .COne
        jmp short .CZero
.CGt:
        cmp ax, cx
        ja .COne
        ; Fall through
.CZero:
        xor ax, ax
        ret
.COne:
        mov ax, 1
        ret
.CToBool:
        and ax, ax
        jz .CDoC
        mov ax, 1
.CDoC:
        and cx, cx
        jz .CTBDone
        mov cx, 1
.CTBDone:
        ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Operand Handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Get token and see if it's a register or number.
; Returns carry clear on success (token handled).
GetRegOrNumber:
        call GetToken
        cmp byte [IsTokenNumber], 0
        je .NotNumber
        mov byte [OperandType], OP_LIT
.RetOK:
        clc
        ret
.NotNumber:
        ; Check if register
        cmp byte [TokenLen], 2
        jne .NotRegOrNum
        mov ax, [UToken]
        mov bx, RegNames
.ChecReg:
        cmp ax, [bx]
        jne .NextReg
        sub bx, RegNames
        shr bx, 1
        mov [OperandValue], bx
        mov byte [OperandType], OP_REG
        jmp .RetOK
.NextReg:
        add bx, 2
        dec cl
        jnz .ChecReg
.NotRegOrNum:
        stc
        ret

; Get operand to OperandType/OperandValue (and possibly CurrentFixup)
GetOperand:
        call NewExpr
        mov al, [CurrentChar]
        cmp al, '['
        jne .NotMem
        jmp GetOperandMem
.NotMem:
        cmp al, QUOTE_CHAR
        jne .NotCharLit
.Expr:
        call GetExpr
        mov [OperandValue], ax
        mov byte [OperandType], OP_LIT
        ret
.NotCharLit:
        cmp al, '('
        je .Expr
        call IsUnaryOp
        je .Expr
        call GetRegOrNumber
        jc .NotRegOrNumber
        cmp byte [OperandType], OP_LIT
        je .ContinueExpr
        ret
.NotRegOrNumber:
        mov ax, [UToken]
        mov bx, [UToken+2]
        mov cl, [TokenLen]
        ; Check for word/byte [mem]
        cmp cl, 4
        jne .CheckFar
        cmp ax, 'BY'
        jne .CheckWord
        cmp bx, 'TE'
        jne .CheckNamedLit
        mov byte [ExplicitSize], 1
        jmp GetOperandMem
.CheckWord:
        cmp ax, 'WO'
        jne .CheckNamedLit
        cmp bx, 'RD'
        jne .CheckNamedLit
        mov byte [ExplicitSize], 2
        jmp GetOperandMem
.CheckFar:
        cmp cl, 3
        jne .CheckShort
        cmp ax, 'FA'
        jne .CheckNamedLit
        cmp bl, 'R'
        jne .CheckNamedLit
        mov byte [ExplicitSize], 2
        jmp short GetOperandMem
.CheckShort:
        cmp cl, 5
        jne .CheckNamedLit
        cmp ax, 'SH'
        jne .CheckNamedLit
        cmp bx, 'OR'
        jne .CheckNamedLit
        cmp byte [UToken+4], 'T'
        jne .CheckNamedLit
        mov byte [ExplicitSize], 1
        jmp .Expr
.CheckNamedLit:
        call GetNamedLiteral
.ContinueExpr:
        mov bx, MAX_PREC
        mov ax, [OperandValue]
        call GetExpr1
        mov [OperandValue], ax
        ret

GetNamedLiteral:
        mov byte [OperandType], OP_LIT
        call FindOrMakeLabel
        mov es, [LabelSeg]
        mov ax, [es:bx+LABEL_ADDR]
        cmp ax, INVALID_ADDR
        je .NeedFixup
        mov [OperandValue], ax
        ret
.NeedFixup:
        mov word [OperandValue], 0
        ;
        ; Add fixup for the label
        ;

        ; First grab a free fixup pointer
        mov ax, [NextFreeFixup]
        cmp ax, INVALID_ADDR
        jne .FixupValid
        mov bx, MsgErrFixupMax
        jmp Error
.FixupValid:
        ; Store the fixup along the other operand data
        mov [CurrentFixup], ax
        ; And update the linked list
        mov cx, [es:bx+LABEL_FIXUP] ; Remember old fixup pointer
        mov [es:bx+LABEL_FIXUP], ax ; Store fixup pointer

        ; Done with the label, switch to pointing to the fixups
        mov es, [FixupSeg]
        mov bx, ax ; es:bx points to the new fixup node

        ; Update free list
        mov ax, [es:bx+FIXUP_LINK]
        mov [NextFreeFixup], ax

        ; And point the link at the old head
        mov [es:bx+FIXUP_LINK], cx

        ret

GetOperandMem:
        push si
        push di
        mov al, '['
        call Expect

        ; si = MODRM (0xff = displacement only), bit 8: minus
        ; di = DISP
        mov si, 0xff
        mov di, 0

.Main:
        and si, 0xff
        mov al, '-'
        call TryConsume
        jc .NotMinus
        or si, 0x100
.NotMinus:
        call GetRegOrNumber
        jc .NamedLit
        cmp byte [OperandType], OP_REG
        jne .Lit
        test si, 0x100
        jnz .InvalidOp
        mov al, [OperandValue]
        cmp al, R_AX
        jae .RegOK
.InvalidOp:
        jmp InvalidOperand
.RegOK:
        cmp al, R_ES
        jae .SegOverride
        call .CombineModrm
        jmp short .Next
.SegOverride:
        ; Output segment override here even though it's a bit dirty
        sub al, R_ES
        mov cl, 3
        shl al, cl
        or al, 0x26
        call OutputByte
        mov al, ':'
        call Expect
        jmp .Main
.NamedLit:
        test si, 0x100
        jnz .InvalidOp
        call GetNamedLiteral
        ; Fall through
.Lit:
        test si, 0x100
        jnz .Sub
        add di, [OperandValue]
        jmp short .Next
.Sub:
        sub di, [OperandValue]
        jmp short .Next
.Next:
        cmp byte [CurrentChar], '-'
        je .Main
        mov al, '+'
        call TryConsume
        jnc .Main
        mov al, ']'
        call Expect
        mov [OperandValue], di
        ; Can't encode [bp] as MODRM=6
        mov ax, si
        cmp al, 6
        jne .NotBPRaw
        and di, di
        jnz .NotBPRaw
        or al, 0x40 ; Disp8
.NotBPRaw:
        cmp al, 0xff
        jne .NotDispOnly
        mov al, 6
        jmp short .Done
.NotDispOnly:
        cmp word [CurrentFixup], INVALID_ADDR
        je .NoFixup
.Disp16:
        or al, 0x80 ; Disp16
        jmp short .Done
.NoFixup:
        and di, di
        jz .Done
        mov bx, di
        push ax
        mov al, bl
        cbw
        mov bx, ax
        pop ax
        cmp bx, di
        jne .Disp16
        or al, 0x40 ; Disp8
        ; Fall through
.Done:
        mov [OperandType], al
        pop di
        pop si
        ret

; Modify MODRM in si with 16-bit register in al
.CombineModrm:
        and al, 7
        mov cx, si
        mov bx, .TabFF
        cmp cl, 0xff
        je .Translate
        mov bx, .Tab04
        cmp cl, 0x04
        je .Translate
        mov bx, .Tab05
        cmp cl, 0x05
        je .Translate
        mov bx, .Tab06
        cmp cl, 0x06
        je .Translate
        mov bx, .Tab07
        cmp cl, 0x07
        jne .CInvalidOp
        ; Fall through
.Translate:
        add bl, al
        adc bh, 0
        mov al, [bx]
        cmp al, 0xff
        je .CInvalidOp
        mov si, ax
        and si, 0xff
        ret
.CInvalidOp:
        jmp InvalidOperand
;
; Conversion tables from previous modrm value to new
;
;            AX    CX    DX    BX    SP    BP    SI    DI    Current ModR/M meaning
.Tab04: db  0xff, 0xff, 0xff, 0x00, 0xff, 0x02, 0xff, 0xff ; SI
.Tab05: db  0xff, 0xff, 0xff, 0x01, 0xff, 0x03, 0xff, 0xff ; DI
.Tab06: db  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x02, 0x03 ; BP
.Tab07: db  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x01 ; BX
.TabFF: db  0xff, 0xff, 0xff, 0x07, 0xff, 0x06, 0x04, 0x05 ; Displacement only


SwapOperands:
        mov al, [OperandType]
        xchg al, [OperandLType]
        mov [OperandType], al
        mov ax, [OperandValue]
        xchg ax, [OperandLValue]
        mov [OperandValue], ax
        ; Fall through
SwapFixup:
        mov ax, [CurrentFixup]
        xchg ax, [CurrentLFixup]
        mov [CurrentFixup], ax
        ret

Get2Operands:
        call GetOperand
        mov al, ','
        call Expect
        call SwapOperands
        jmp GetOperand

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Label Handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Define label (from Token) at current address
DefineLabel:
        cmp byte [Token], '.'
        je .Local
        ; Non-local label, retire all previously defined local labels
        call RetireLocLabs
.Local:
        call FindLabel
        cmp bx, INVALID_ADDR
        jne .KnownLabel
        call MakeLabel
        mov es, [LabelSeg]
        mov ax, [CurrentAddress]
        mov [es:bx+LABEL_ADDR], ax
        ret
.KnownLabel:
        mov es, [LabelSeg]
        cmp word [es:bx+LABEL_ADDR], INVALID_ADDR
        je .NotDup
        mov bx, MsgErrDupLabel
        jmp Error
.NotDup:
        mov ax, [CurrentAddress]
        mov [es:bx+LABEL_ADDR], ax ; Set label address
        ; Resolve fixups
        mov cx, [es:bx+LABEL_FIXUP] ; Store last valid fixup in CX
        mov word [es:bx+LABEL_FIXUP], INVALID_ADDR
        mov bx, cx
        push si
        push di
        push cx
        mov dx, [FixupSeg]
        mov di, [OutputSeg]
.ResolveFixup:
        cmp bx, INVALID_ADDR
        je .Done

        mov cx, bx ; Update last valid fixup node
        mov es, dx
        mov si, [es:bx+FIXUP_ADDR]
        ; Note instruction order
        cmp byte [es:bx+FIXUP_TYPE], FIXUP_DISP16
        mov bx, [es:bx+FIXUP_LINK]
        mov es, di
        jne .FixupRel8
        add [es:si], ax
        jmp .ResolveFixup
.FixupRel8:
        push ax
        mov ax, [NumOutBytes]
        sub ax, si
        dec ax
        cmp ax, 0x7F
        jbe .RangeOK
        mov bx, MsgErrNotRel8
        jmp Error
.RangeOK:
        mov [es:si], al
        pop ax
        jmp .ResolveFixup
.Done:
        mov es, dx
        pop dx ; First fixup
        cmp cx, INVALID_ADDR
        je .NoFixups
        mov bx, cx
        mov ax, [NextFreeFixup]
        mov [es:bx+FIXUP_LINK], ax
        mov [NextFreeFixup], dx
.NoFixups:
        pop di
        pop si
        ret

; Print Label in BX (assumes ES=LabelSeg). Registers preserved.
PrintLabel:
        push ax
        push bx
        push cx
        push dx
        push ds
        mov ax, es
        mov ds, ax
        push bx
        call PutString
        pop bx
        mov al, ' '
        call PutChar
        pop ds
        mov ax, [es:bx+LABEL_ADDR]
        push bx
        call PutHex
        mov al, ' '
        call PutChar
        pop bx
        mov ax, [es:bx+LABEL_FIXUP]
        call PutHex
        call PutCrLf
        pop dx
        pop cx
        pop bx
        pop ax
        ret

RetireLocLabs:
        mov es, [LabelSeg]
        xor bx, bx
        mov cx, [NumLabels]
        and cx, cx
        jnz .L
        ret
.L:
        cmp byte [es:bx], '.'
        jne .Next

        cmp word [es:bx+LABEL_ADDR], INVALID_ADDR
        jne .NotInv
.Inv:
        call PrintLabel
        mov bx, MsgErrUndefLab
        jmp Error
.NotInv:
        cmp word [es:bx+LABEL_FIXUP], INVALID_ADDR
        jne .Inv

        mov ax, [NumLabels]
        dec ax
        mov [NumLabels], ax
        jz .Done

        mov dx, LABEL_SIZE
        mul dx

        push ds
        push si
        push di
        push cx
        mov si, es
        mov ds, si
        mov di, bx
        mov si, ax
        mov cx, LABEL_SIZE
        rep movsb
        pop cx
        pop di
        pop si
        pop ds

        sub bx, LABEL_SIZE ; Re-do current label

.Next:
        add bx, LABEL_SIZE
        dec cx
        jnz .L
.Done:
        ret

; Register fixup of type AL for CurrentFixup at this exact output location
RegisterFixup:
        mov bx, [CurrentFixup]
        mov cx, INVALID_ADDR
        cmp bx, cx
        jne .OK
        mov bx, MsgErrNoFixup
        jmp Error
.OK:
        mov [CurrentFixup], cx
        mov es, [FixupSeg]
        mov cx, [NumOutBytes]
        mov [es:bx+FIXUP_ADDR], cx
        mov [es:bx+FIXUP_TYPE], al
        ret

; Find label matching Token, returns pointer in BX or
; INVALID_ADDR if not found
FindLabel:
        push si
        push di
        mov cx, [NumLabels]
        and cx, cx
        jz .NotFound
        mov es, [LabelSeg]
        xor bx, bx
.Search:
        mov si, Token
        mov di, bx
.Compare:
        mov al, [si]
        cmp [es:di], al
        jne .Next
        and al, al
        jz .Done
        inc si
        inc di
        jmp .Compare
.Next:
        add bx, LABEL_SIZE
        dec cx
        jnz .Search
.NotFound:
        mov bx, INVALID_ADDR
.Done:
        pop di
        pop si
        ret

; Returns pointer to label in BX. Label must NOT exist.
MakeLabel:
        mov ax, [NumLabels]
        cmp ax, LABEL_MAX
        jb .OK
        mov bx, MsgErrLabelMax
        jmp Error
.OK:
        inc word [NumLabels]
        mov bx, LABEL_SIZE
        mul bx
        mov bx, ax
        mov es, [LabelSeg]
        mov ax, INVALID_ADDR
        mov [es:bx+LABEL_ADDR], ax
        mov [es:bx+LABEL_FIXUP], ax
        push di
        push si
        xor ch, ch
        mov cl, [TokenLen]
        inc cx
        mov di, bx
        mov si, Token
        rep movsb
        pop si
        pop di
        ret

; Returns pointer to label (from Token) in BX
FindOrMakeLabel:
        call FindLabel
        cmp bx, INVALID_ADDR
        je MakeLabel
        ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; EQU Handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Find EQU (from Token) in BX, INVALID_ADDR if not found
; Leaves ES pointer to EquSeg
FindEqu:
        push si
        push di
        mov es, [EquSeg]
        mov cx, [NumEqus]
        and cx, cx
        jz .NotFound
        xor bx, bx
.Main:
        mov si, Token
        mov di, bx
.Compare:
        cmpsb
        jne .Next
        cmp byte [si-1], 0
        jz .Done
        jmp .Compare
.Next:
        add bx, EQU_SIZE
        dec cx
        jnz .Main
        ; Fall through
.NotFound:
        mov bx, INVALID_ADDR
.Done:
        pop di
        pop si
        ret

; Create new EQU from Token and the next token(s) (EQU assumed to be parsed)
MakeEqu:
        call FindEqu
        cmp bx, INVALID_ADDR
        je .NewEqu
        mov bx, MsgErrDupEqu
        jmp Error
.NewEqu:
        mov ax, [NumEqus]
        cmp ax, EQU_MAX
        jb .HasRoom
        mov bx, MsgErrEquMax
        jmp Error
.HasRoom:
        inc word [NumEqus]
        mov bx, EQU_SIZE
        mul bx
        push si
        push di
        xor ch, ch
        mov cl, [TokenLen]
        inc cx
        mov di, ax
        mov si, Token
        rep movsb
        pop di
        pop si
        push ax
        call GetExpr
        pop bx
        mov [es:bx+EQU_VAL], ax
        ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Directives
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DirORG:
        call GetExpr
        mov [SectionStart], ax
        mov [CurrentAddress], ax
        ret

DirCPU:
        call GetExpr
        xor dx, dx
        mov bx, 100
        div bx
        cmp dx, 86
        jne .Invalid
        cmp ax, 0
        je .Invalid
        cmp ax, 3
        jbe .OK
        cmp ax, 80
        jne .Invalid
        mov ax, 0
.OK:
        mov [CpuLevel], al
        ret
.Invalid:
        mov bx, MsgErrInvalidCPU
        jmp Error

; AL = 1 if DB 2 if DW
DirDX:
        push si
        mov si, ax
.Main:
        mov al, QUOTE_CHAR
        call TryGet
        jc .NotLit
        ; Handle character literal
        xor cx, cx
.CharLit:
        push cx
        mov al, QUOTE_CHAR
        call TryConsume
        pop cx
        jnc .LitDone
        inc cx
        mov al, [CurrentChar]
        cmp al, ' '
        jae .OK
        mov bx, MsgErrChLitErr
        jmp Error
.OK:
        push cx
        call OutputByte ; Always output bytes for character literals
        call ReadNext
        pop cx
        jmp .CharLit
.LitDone:
        ; Output zero byte if necessary to ensure dw alignment
        cmp si, 2
        jne .Next
        and cl, 1
        jz .Next
        xor al, al
        call OutputByte
        jmp short .Next
.NotLit:
        call GetExpr
        push ax
        cmp word [CurrentFixup], INVALID_ADDR
        je .Out1
        cmp si, 2
        je .Fixup
        mov bx, MsgErrDBFixup
        jmp Error
.Fixup:
        mov al, FIXUP_DISP16
        call RegisterFixup
.Out1:
        pop ax
        call OutputByte
        cmp si, 1
        je .Next
        mov al, ah
        call OutputByte
        ; Fall thorugh
.Next:
        mov al, ','
        call TryConsume
        jnc .Main
        pop si
        ret

; AL = 1 if RESB 2 if RESW
DirResX:
        push ax
        call GetExpr
        pop bx
        xor dx, dx
        mul bx
        and dx, dx
        jnz .Overflow
        add [CurrentAddress], ax
        jc .Overflow
        add [PendingZeros], ax
        ret
.Overflow:
        mov bx, MsgErrMemOvrflow
        jmp Error

DirIf:
        xor bh, bh
        mov bl, [NumIfs]
        cmp bl, IF_MAX
        jb .NotMax
        mov bx, MsgErrIfMax
        jmp Error
.NotMax:
        push bx
        call GetExpr
        pop bx
        and bl, bl
        jz .NotDisabled
        test byte [Ifs+bx-1], IF_ACTIVE
        jnz .NotDisabled
        ; In disabled block
        mov byte [Ifs+bx], IF_WAS_ACTIVE
        jmp short .Ret
.NotDisabled:
        xor cl, cl
        and ax, ax
        je .Store
        mov cl, IF_ACTIVE|IF_WAS_ACTIVE
.Store:
        mov [Ifs+bx], cl
.Ret:
        inc bl
        mov [NumIfs], bl
        ret

; Return current If-state in AL and pointer to it in BX
GetCurrentIf:
        xor bh, bh
        mov bl, [NumIfs]
        dec bl
        jc .Bad
        add bx, Ifs
        mov al, [bx]
        test al, IF_SEEN_ELSE
        jnz .Bad
        ret
.Bad:
        mov bx, MsgErrBadElsif
        jmp Error

DirElif:
        call GetExpr
        push ax
        call GetCurrentIf
        pop cx
        test al, IF_WAS_ACTIVE
        jnz .NotActive
        and cx, cx
        jz .NotActive
        or al, IF_ACTIVE|IF_WAS_ACTIVE
        jmp short .Ret
.NotActive:
        and al, ~IF_ACTIVE
.Ret:
        mov [bx], al
        ret

DirElse:
        call GetCurrentIf
        test al, IF_WAS_ACTIVE
        jnz .WasActive
        or al, IF_ACTIVE|IF_WAS_ACTIVE
        jmp short .Ret
.WasActive:
        and al, ~IF_ACTIVE
.Ret:
        or al, IF_SEEN_ELSE
        mov [bx], al
        ret

DirEndif:
        dec byte [NumIfs]
        jc .Err
        ret
.Err:
        mov bx, MsgErrBadEndif
        jmp Error

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Instructions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

InvalidOperand:
        mov bx, MsgErrInvalidOpe
        jmp Error

HandleRel16:
        cmp byte [OperandType], OP_LIT
        jne InvalidOperand
        cmp word [CurrentFixup], INVALID_ADDR
        je .NoFixup
        mov al, FIXUP_DISP16
        call RegisterFixup
.NoFixup:
        mov ax, [OperandValue]
        sub ax, [CurrentAddress]
        sub ax, 2
        jmp OutputWord

; Output instruction byte in AL, ORed with 1 if OperandValue
; is a 16-bit register (R_AX .. R_DI)
OutInst816:
        ; 16-bit?
        mov bl, [OperandValue]
        mov cl, 3
        shr bl, cl
        cmp bl, 1
        jne .Not16
        or al, 1
.Not16:
        jmp OutputByte

; Outputs MODR/M byte(s) with register in AL
; and memory operand assumed in OperandL*
OutputModRM:
        mov cl, 3
        shl al, cl
        or al, [OperandLType]
        call OutputByte
        mov al, [OperandLType]
        cmp al, 6
        je .Disp16
        and al, 0xc0
        cmp al, 0x80
        je .Disp16
        cmp al, 0x40
        je .Disp8
        ret
.Disp8:
        mov al, [OperandLValue]
        jmp OutputByte
.Disp16:
        cmp word [CurrentLFixup], INVALID_ADDR
        je .Out
        call SwapFixup
        mov al, FIXUP_DISP16
        call RegisterFixup
        call SwapFixup
.Out:
        mov ax, [OperandLValue]
        jmp OutputWord

; Output reg, reg instruction with instruction byte in AL
; The low bit of the instruction is set if OperandValue
; signifies a 16-bit (not sreg) register
OutputRR:
        call OutInst816
        mov al, 0xc0
        mov ah, [OperandLValue]
        and ah, 7
        or al, ah
        mov ah, [OperandValue]
        and ah, 7
        mov cl, 3
        shl ah, cl
        or al, ah
        jmp OutputByte

; Output mem, reg otherwise see notes for OutputRR
OutputMR:
        ; TODO: Check if ExplicitSize (it's either redundant or wrong)
        call OutInst816
        mov al, [OperandValue]
        and al, 7
        jmp OutputModRM

; Output reg, mem otherwise see notes for OutputMR
OutputRM:
        push ax
        call SwapOperands
        pop ax
        jmp OutputMR

; Output mem, imm with base instruction in AH, /r in AL
OutputMImm:
        mov bl, [ExplicitSize]
        dec bl
        jns .HasSize
        mov bx, MsgErrNoSize
        jmp Error
.HasSize:
        jz .Output
        cmp ah, 0x80
        jne .Output
        cmp word [CurrentFixup], INVALID_ADDR
        jne .Output
        push ax
        mov ax, [OperandValue]
        mov cx, ax
        cbw
        cmp cx, ax
        pop ax
        jne .Output
        or ah, 3
        xor bl, bl
.Output:
        push bx
        push ax
        or bl, ah
        mov al, bl
        call OutputByte ; Opcode
        pop ax
        call OutputModRM
        pop bx
        mov al, bl
        jmp OutputImm

InstMOV:
        call Get2Operands
        cmp byte [OperandLType], OP_REG
        je .MOVr
        jb .MOVm
        jmp .InvalidOp
.MOVr:
        cmp byte [OperandType], OP_REG
        je .MOVrr
        jb .MOVrm
        ; MOV reg, imm
        mov al, 0xb0
        add al, [OperandLValue]
        call OutputByte
        mov al, [OperandLValue]
        mov cl, 3
        shr al, cl
        jmp OutputImm
.MOVrr:
        ; MOV reg, reg
        cmp byte [OperandLValue], R_ES
        jae .MOVs
        ; LHS isn't s-reg
        cmp byte [OperandValue], R_ES
        jae .MOVrs
        mov al, 0x88
        jmp OutputRR
.MOVrs:
        mov al, 0x8c
        jmp OutputRR
.MOVs:
        mov al, [OperandValue]
        sub al, R_AX
        cmp al, 7
        ja .InvalidOp
        call SwapOperands
        mov al, 0x8e
        jmp OutputRR
.MOVrm:
        mov al, [OperandLValue]
        cmp al, R_ES
        jae .MOVsrm
        cmp byte [OperandType], 6
        jne .MOVrm3
        cmp al, R_AL
        jne .MOVrm2
        mov al, 0xA0
        jmp short .Imm16
.MOVrm2:
        cmp al, R_AX
        jne .MOVrm3
        mov al, 0xA1
.Imm16:
        call OutputByte
        jmp OutputImm16
.MOVrm3:
        mov al,0x8A
        jmp OutputRM
.MOVsrm:
        ; MOV sreg, r/m
        mov al, 0x8E
        jmp OutputRM
.MOVm:
        cmp byte [OperandType], OP_REG
        je .MOVmr
        jb .InvalidOp
        mov ax, 0xc600
        jmp OutputMImm
.MOVmr:
        mov al, [OperandValue]
        cmp al, R_ES
        jae .MOVmsr
        cmp byte [OperandLType], 6
        jne .MOVmr3
        cmp al, R_AL
        jne .MOVmr2
        call SwapOperands
        mov al, 0xA2
        jmp .Imm16
.MOVmr2:
        cmp al, R_AX
        jne .MOVmr3
        call SwapOperands
        mov al, 0xA3
        jmp .Imm16
.MOVmr3:
        mov al, 0x88
        jmp OutputMR
.MOVmsr:
        mov al, 0x8C
        jmp OutputMR
.InvalidOp:
        jmp InvalidOperand

InstXCHG:
        call Get2Operands
        cmp byte [OperandLType], OP_REG
        je .XCHGr
        ja .InvalidOp
        cmp byte [OperandType], OP_REG
        jne .InvalidOp
.XCHGmr:
        mov al, 0x86
        call OutInst816
        mov al, [OperandValue]
        and al, 7
        jmp OutputModRM
.XCHGr:
        cmp byte [OperandType], OP_REG
        je .XCHGrr
        ja .InvalidOp
        call SwapOperands
        jmp .XCHGmr
.XCHGrr:
        cmp byte [OperandLValue], R_AX
        jne .X1
        mov al, [OperandValue]
        and al, 7
        or al, 0x90
        jmp OutputByte
.X1:
        cmp byte [OperandValue], R_AX
        jne .X2
        mov al, [OperandLValue]
        and al, 7
        or al, 0x90
        jmp OutputByte
.X2:
        mov al, 0x86
        jmp OutputRR
.InvalidOp:
        jmp InvalidOperand

InstTEST:
        call Get2Operands
        cmp byte [OperandType], OP_REG
        je .TESTr
        jb .TESTm
        cmp byte [OperandLType], OP_REG
        ja .InvalidOp
        je .TESTrl
        ; TEST mem, lit
        mov al, [ExplicitSize]
        dec al
        jns .HasSize
        mov bx, MsgErrNoSize
        jmp Error
.HasSize:
        push ax
        or al, 0xF6
        call OutputByte
        xor al, al
        call OutputModRM
        pop ax
        jmp OutputImm
.TESTrl:
        ; TEST reg, lit 0x
        mov al, [OperandLValue]
        cmp al, R_ES
        jae .InvalidOp
        cmp al, R_AL
        je .TESTalImm
        cmp al, R_AX
        je .TESTaxImm
        mov ah, al
        mov cl, 3
        shr al, cl
        push ax
        or al, 0xF6
        and ah, 7
        or ah, 0xC0
        call OutputWord
        pop ax
        jmp OutputImm
.TESTalImm:
        mov al, 0xA8
        call OutputByte
        jmp OutputImm8
.TESTaxImm:
        mov al, 0xA9
        call OutputByte
        jmp OutputImm16
.TESTr:
        ; TEST xxx, reg
        cmp byte [OperandValue], R_ES
        jae .InvalidOp
        mov al, 0x84
        cmp byte [OperandLType], OP_REG
        ja .InvalidOp
        jb .TestMR
        jmp OutputRR
.TestMR:
        jmp OutputMR
.TESTm:
        cmp byte [OperandLType], OP_REG
        jne .InvalidOp
        call SwapOperands
        jmp .TESTr
.InvalidOp:
        jmp InvalidOperand

; AL=second opcode byte
InstMOVXX:
        push ax
        mov al, 3
        call CheckCPU
        call Get2Operands
        pop ax
        cmp byte [OperandLType], OP_REG
        je .MOVXXr
        jmp NotImplemented
.MOVXXr:
        mov ah, al
        mov al, 0x0f
        call OutputWord
        cmp byte [OperandType], OP_REG
        je .RR
        call SwapOperands
        mov al, [OperandValue]
        and al, 7
        jmp OutputModRM
.RR:
        mov al, [OperandValue]
        and al, 7
        mov ah, [OperandLValue]
        and ah, 7
        mov cl, 3
        shl ah, cl
        or al, ah
        or al, 0xc0
        jmp OutputByte

HandleLXXArgs:
        call Get2Operands
        cmp byte [OperandLType], OP_REG
        jne .InvalidOp
        cmp byte [OperandType], OP_REG
        jae .InvalidOp
        mov bl, [OperandLValue]
        mov cl, 3
        shr bl, cl
        cmp bl, 1
        jne .InvalidOp
        ret
.InvalidOp:
        jmp InvalidOperand

InstLEA:
        call HandleLXXArgs
        mov al, 0x8D
        jmp OutputRM

; AL=opcode byte
InstLXS:
        push ax
        call HandleLXXArgs
        pop ax
        ; Suppress OR 1 in OutInst816
        mov byte [ExplicitSize], 0
        and byte [OperandLValue], 7
        jmp OutputRM

; AL=0 if INC, AL=1 if DEC
InstIncDec:
        push ax
        call GetOperand
        pop ax
        cmp byte [OperandType], OP_REG
        je .Reg
        jb .Mem
        jmp InvalidOperand
.Mem:
        mov ah, [ExplicitSize]
        dec ah
        jns .HasSize
        mov bx, MsgErrNoSize
        jmp Error
.HasSize:
        or ah, 0xfe
        push ax
        mov al, ah
        call OutputByte
        call SwapOperands
        pop ax
        jmp OutputModRM
.Reg:
        mov cl, 3
        shl al, cl
        mov ah, [OperandValue]
        mov bl, ah
        and ah, 7
        or al, ah
        or al, 0x40 ; AL = 0x40|dec<<3|(OperandValue&7)
        shr bl, cl
        jz .Reg8
        jmp OutputByte
.Reg8:
        or al, 0xc0 ; Could just be |0x80 since 0x40 is already or'ed in
        mov ah, al
        mov al, 0xfe
        jmp OutputWord

; AL=/r (e.g. 3 for NEG)
InstNotNeg:
        push ax
        call GetOperand
        cmp byte [OperandType], OP_REG
        je .NNr
        jb .NNm
.InvalidOp:
        jmp InvalidOperand
.NNm:
        mov al, [ExplicitSize]
        dec al
        jns .HasSize
        mov bx, MsgErrNoSize
        jmp Error
.HasSize:
        or al, 0xF6
        call OutputByte
        call SwapOperands
        pop ax
        jmp OutputModRM
.NNr:
        mov al, [OperandValue]
        mov ah, al
        mov cl, 3
        shr al, cl
        cmp al, 1
        ja .InvalidOp
        or al, 0xF6
        call OutputByte
        and ah, 7
        pop bx
        shl bl, cl
        or ah, bl
        or ah, 0xc0
        mov al, ah
        jmp OutputByte

; Base instruction in AL (e.g. 0x38 for CMP)
InstALU:
        push ax
        call Get2Operands
        pop ax
        cmp byte [OperandLType], OP_REG
        je .ALUr
        ja .InvalidOp
        cmp byte [OperandType], OP_REG
        je .ALUmr
        jb .InvalidOp
        mov cl, 3
        shr al, cl
        mov ah, 0x80
        jmp OutputMImm
.InvalidOp:
        jmp InvalidOperand
.ALUmr:
        jmp OutputMR
.ALUr:
        cmp byte [OperandType], OP_REG
        je .ALUrr
        ja .ALUrl
        add al, 2
        jmp OutputRM
.ALUrr:
        jmp OutputRR
.ALUrl:
        cmp byte [OperandLValue], R_AL
        jne .ALUrl2
        add al, 4
        call OutputByte
        jmp OutputImm8
.ALUrl2:
        cmp byte [OperandLValue], R_AX
        jne .ALUrl3
        add al, 5
        call OutputByte
        jmp OutputImm16
.ALUrl3:
        mov ah, al
        or ah, 0xc0
        mov al, [OperandLValue]
        mov bl, al
        mov cl, 3
        shr al, cl
        mov bh, al
        cmp al, 1
        jne .ALUrl4
        cmp word [CurrentFixup], INVALID_ADDR
        jne .ALUrl4
        push ax
        mov cx, [OperandValue]
        mov ax, cx
        cbw
        cmp ax, cx
        pop ax
        jne .ALUrl4
        or al, 3
        xor bh, bh
.ALUrl4:
        or al, 0x80
        and bl, 7
        or ah, bl
        push bx
        call OutputWord
        pop bx
        mov al, bh
        jmp OutputImm

; /r in AL (e.g. 6 for DIV)
InstMulDiv:
        push ax
        call GetOperand
        pop ax
        cmp byte [OperandType], OP_REG
        jb .M
        jne .InvalidOp
        ; Output 0xF6 | is16bit, 0xC0 | r<<3 | (OperandValue&7)
        mov ah, al
        mov cl, 3
        shl ah, cl
        or ah, 0xC0
        mov al, [OperandValue]
        cmp al, R_ES
        jae .InvalidOp
        mov bl, al
        and bl, 7
        or ah, bl
        shr al, cl
        or al, 0xF6
        jmp OutputWord
.M:
        push ax
        mov al, [ExplicitSize]
        dec al
        jns .HasSize
        mov bx, MsgErrNoSize
        jmp Error
.HasSize:
        or al, 0xf6
        call OutputByte
        call SwapOperands
        pop ax
        jmp OutputModRM
.InvalidOp:
        jmp InvalidOperand

; /r in AL (e.g. 4 for SHL)
InstROT:
        push ax
        call Get2Operands
        pop bx
        cmp byte [OperandLType], OP_REG
        je .ROTr
        ja .InvalidOp

        mov al, [ExplicitSize]
        dec al
        jns .HasSize
        mov bx, MsgErrNoSize
        jmp Error
.HasSize:
        cmp byte [OperandType], OP_REG
        je .ROTmr
        jb .InvalidOp
        ; ROT mem, imm
        ; Special handling for ROT r/m, 1
        cmp byte [OperandValue], 1
        jne .ROTmimm
        or al, 0xd0
        jmp short .ROTmrFinal
.InvalidOp:
        jmp InvalidOperand
.ROTmimm:
        or al, 0xc0
        call .ROTmrFinal
        jmp OutputImm8
.ROTmr:
        cmp byte [OperandValue], R_CL
        jne .InvalidOp
        or al, 0xd2
.ROTmrFinal:
        push bx
        call OutputByte
        pop ax
        jmp OutputModRM
.ROTr:
        ; AL = is16bit
        ; AH = 0xC0 | r<<3 | (OperandLValue&7)
        mov ah, [OperandLValue]
        mov al, ah
        and ah, 7
        mov cl, 3
        shl bl, cl
        or ah, bl
        or ah, 0xc0
        shr al, cl
        and al, 1

        cmp byte [OperandType], OP_REG
        ja .ROTrl
        jb .InvalidOp
        cmp byte [OperandValue], R_CL
        jne .InvalidOp
        or al, 0xd2
        jmp OutputWord
.ROTrl:
        ; Special handling for ROT r/m, 1
        cmp byte [OperandValue], 1
        jne .ROTrimm
        or al, 0xd0
        jmp OutputWord
.ROTrimm:
        push ax
        mov al, 1
        call CheckCPU
        pop ax
        or al, 0xc0
        call OutputWord
        jmp OutputImm8

InstIN:
        call Get2Operands
        mov al, 0xE4
        ; Fall through

HandleInOut:
        cmp byte [OperandLType], OP_REG
        jne .InvalidOp
        mov bl, [OperandLValue]
        test bl, 7
        jnz .InvalidOp
        mov cl, 3
        shr bl, cl
        cmp bl, 1
        ja .InvalidOp
        or al, bl
        cmp byte [OperandType], OP_REG
        jb .InvalidOp
        ja .INl
        cmp byte [OperandValue], R_DX
        jne .InvalidOp
        or al, 0x08
        jmp OutputByte
.INl:
        call OutputByte
        jmp OutputImm8
.InvalidOp:
        jmp InvalidOperand

InstOUT:
        call Get2Operands
        call SwapOperands
        mov al, 0xE6
        jmp HandleInOut

InstAAX:
        call OutputByte
        mov al, 0x0A
        jmp OutputByte

InstINT:
        mov al, 0xcd
        call OutputByte
        call GetOperand
        cmp byte [OperandType], OP_LIT
        jne .Err
        cmp byte [OperandValue+1], 0
        jne .Err
        jmp OutputByte
.Err:
        jmp InvalidOperand

; Output instruction in AL if ptr16:16
HandleFar:
        cmp byte [OperandType], OP_LIT
        jne .DontHandle
        push ax
        mov al, ':'
        call TryConsume
        pop ax
        jnc .Handle
.DontHandle:
        stc
        ret
.Handle:
        call OutputByte
        push word [OperandValue]
        call GetToken
        call GetTokenNumber
        mov ax, [OperandValue]
        call OutputWord
        pop ax
        call OutputWord
        clc
        ret

InstCALL:
        call GetOperand
        mov al, 0x9A
        call HandleFar
        jc .NotFar
        ret
.NotFar:
        cmp byte [OperandType], OP_REG
        jb .CallM
        je .CallR
        mov al, 0xE8
        call OutputByte
        jmp HandleRel16
.CallR:
        mov al, 0xFF
        call OutputByte
        mov al, [OperandValue]
        and al, 7
        or al, 0xD0 ; 0xC0 | (2<<3)
        jmp OutputByte
.CallM:
        mov al, 0xFF
        call OutputByte
        call SwapOperands
        mov al, 2
        cmp byte [ExplicitSize], 2
        jne .DoRM
        inc al ; far CALL
.DoRM:
        jmp OutputModRM

InstPUSH:
        call GetOperand
        cmp byte [OperandType], OP_REG
        je .PushR
        ja .PushL
        mov al, 0xFF
        call OutputByte
        call SwapOperands
        mov al, 6
        jmp OutputModRM
.PushL:
        mov al, 1
        call CheckCPU
        cmp word [CurrentFixup], INVALID_ADDR
        jne .PushImm16
        mov bx, [OperandValue]
        mov al, bl
        cbw
        xchg ax, bx
        cmp ax, bx
        jne .PushImm16
        mov al, 0x6A
        call OutputByte
        jmp OutputImm8
.PushImm16:
        mov al, 0x68
        call OutputByte
        jmp OutputImm16
.PushR:
        mov al, [OperandValue]
        sub al, R_AX
        cmp al, 8
        jae .PushS
        or al, 0x50
        jmp OutputByte
.PushS:
        sub al, 8
        mov cl, 3
        shl al, cl
        or al, 0x06
        jmp OutputByte

InstPOP:
        call GetOperand
        cmp byte [OperandType], OP_REG
        jb .PopM
        jne .InvalidOp
        mov al, [OperandValue]
        sub al, R_AX
        js .InvalidOp
        cmp al, 8
        jae .PopS
        or al, 0x58
        jmp OutputByte
.PopS:
        and al, 7
        mov cl, 3
        shl al, cl
        or al, 0x07
        jmp OutputByte
.InvalidOp:
        jmp InvalidOperand
.PopM:
        mov al, 0x8F
        call OutputByte
        call SwapOperands
        xor al, al
        jmp OutputModRM

; Output instruction byte in AL and Rel8 if OperandValue is a short
; (known) jump or forced by AH=1 or SHORT in the program text.
; Returns carry clear on success
HandleShortRel:
        cmp byte [ExplicitSize], 1 ; SHORT?
        jne .HasOperand
        mov ah, 1
.HasOperand:
        cmp byte [OperandType], OP_LIT
        je .Lit
        jmp InvalidOperand
.Lit:
        cmp word [CurrentFixup], INVALID_ADDR
        jne .Fixup
        mov bx, [OperandValue]
        sub bx, [CurrentAddress]
        sub bx, 2
        push ax
        mov al, bl
        cbw
        mov cx, ax
        pop ax
        cmp cx, bx
        jne .NotShort
        push bx
        call OutputByte
        pop ax
        call OutputByte
.RetNC:
        clc
        ret
.Fixup:
        and ah, ah
        jz .NotShort
        call OutputByte
        mov al, FIXUP_REL8
        call RegisterFixup
        xor al, al
        call OutputByte
        jmp .RetNC
.NotShort:
        and ah, ah
        jz .RetC
        mov bx, MsgErrNotRel8
        jmp Error
.RetC:
        stc
        ret

InstJMP:
        call GetOperand
        mov al, 0xEA
        call HandleFar
        jc .NotFar
        ret
.NotFar:
        cmp byte [OperandType], OP_REG
        je .JmpR
        jb .JmpM
        xor ah, ah
        mov al, 0xEB
        call HandleShortRel
        jc .NotShort
        ret
.NotShort:
        mov al, 0xE9
        call OutputByte
        jmp HandleRel16
.JmpR:
        mov al, 0xFF
        call OutputByte
        mov al, [OperandValue]
        cmp al, R_AX
        jb .InvOp
        cmp al, R_DI
        ja .InvOp
        and al, 0x07
        or al, 0xC0 | 4<<3
        jmp OutputByte
.JmpM:
        mov al, 0xFF
        call OutputByte
        call SwapOperands
        mov al, 4
        cmp byte [ExplicitSize], 2
        jne .DoRM
        inc al ; far JMP
.DoRM:
        jmp OutputModRM
.InvOp:
        jmp InvalidOperand

; AL contains opcode
InstJ8:
        push ax
        call GetOperand
        pop ax
        mov ah, 1 ; Force short jump
        jmp HandleShortRel

; AL contains the condition code
InstJCC:
        push ax
        call GetOperand
        pop ax
        push ax
        xor ah, ah
        cmp byte [CpuLevel], 3
        jae .Has386
        inc ah ; Force Rel8
.Has386:
        or al, 0x70
        call HandleShortRel
        pop ax
        jc .Rel16
        ret
.Rel16:
        mov ah, al
        or ah, 0x80
        mov al, 0x0F
        call OutputWord
        jmp HandleRel16

CheckCPU:
        cmp [CpuLevel], al
        jae .OK
        mov bx, MsgErrCpuLevel
        jmp Error
.OK:
        ret

OutputByte186:
        push ax
        mov al, 1
        call CheckCPU
        pop ax
        jmp OutputByte

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Constants
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MsgHello:         db 'SASM 1.1 Processing ', 0
MsgHello2:        db ' to ', 0
MsgCurToken:      db 'Current token: "', 0
MsgErrInLine:     db 'Error in line ', 0
MsgErrNotImpl:    db 'Not implemented', 0
MsgErrNotDone:    db 'File not completely parsed', 0
MsgErrUnknInst:   db 'Unknown directive or instruction', 0
MsgErrOpenIn:     db 'Error opening input file', 0
MsgErrOutput:     db 'Error during output', 0
MsgErrRead:       db 'Error reading from input file', 0
MsgErrMem:        db 'Alloc failed', 0
MsgErrInvalidNum: db 'Error invalid number', 0
MsgErrInvalidOpe: db 'Invalid operand', 0
MsgErrLabelMax:   db 'Too many labels', 0
MsgErrFixupMax:   db 'Too many fixups', 0
MsgErrDupLabel:   db 'Duplicate label', 0
MsgErrFixupUnh:   db 'Unhandled fixup', 0
MsgErrChLitErr:   db 'Invalid character literal', 0
MsgErrUndefLab:   db 'Undefined label', 0
MsgErrNoSize:     db 'Size missing for memory operand', 0
MsgErrDupEqu:     db 'Duplicate EQU', 0
MsgErrEquMax:     db 'Too many EQUs', 0
MsgErrOutMax:     db 'Output buffer full', 0
MsgErrNoFixup:    db 'No fixup to register?', 0
MsgErrMemOvrflow: db 'Address exceeds segment', 0
MsgErrInvalidCPU: db 'Invalid/unsupported CPU', 0
MsgErrNotRel8:    db 'Address out of 8-bit range', 0
MsgErrCpuLevel:   db 'Instruction not supported at this CPU level', 0
MsgErrDBFixup:    db 'Fixup not possible with DB', 0
MsgErrDivZero:    db 'Division by zero in literal expression', 0
MsgErrLabelUnd:   db 'Label not defined', 0
MsgErrInvalidOp:  db 'Invalid binary operator', 0
MsgErrIfMax:      db 'Too many nested %if blocks', 0
MsgErrBadElsif:   db 'Invalid use of %elif/%else', 0
MsgErrBadEndif:   db '%endif outside %if block', 0
MsgErrIfActive:   db 'Open %if block', 0
MsgErrExpected:   db '? expected',0 ; NOTE! modified by Expect


RegNames:
    dw 'AL', 'CL', 'DL', 'BL', 'AH', 'CH', 'DH', 'BH'
    dw 'AX', 'CX', 'DX', 'BX', 'SP', 'BP', 'SI', 'DI'
    dw 'ES', 'CS', 'SS', 'DS'

; Each entry has "DISPATCH_SIZE" and starts with a NUL-padded string
; of size INST_MAX followed by a byte that's passed in AL (AX) to the
; function in the final word.
DispatchList:
    ; Directives
    db 'DB',0,0,0,0, 0x01
    dw DirDX
    db 'DW',0,0,0,0, 0x02
    dw DirDX
    db 'RESB',0,0,   0x01
    dw DirResX
    db 'RESW',0,0,   0x02
    dw DirResX
    db 'ORG',0,0,0,  0x00
    dw DirORG
    db 'CPU',0,0,0,  0x00
    dw DirCPU
    db '%IF',0,0,0,  0x00
    dw DirIf
    db '%ELIF',0,    0x00
    dw DirElif
    db '%ELSE',0,    0x00
    dw DirElse
    db '%ENDIF',     0x00
    dw DirEndif

    ; MOV/XCHG/TEST
    db 'MOV',0,0,0,  0x00
    dw InstMOV
    db 'XCHG',0,0,   0x00
    dw InstXCHG
    db 'TEST',0,0,   0x00
    dw InstTEST

    ; MOVSX/MOVZX/LEA/LES/LDS
    db 'MOVZX',0,    0xB6
    dw InstMOVXX
    db 'MOVSX',0,    0xBE
    dw InstMOVXX
    db 'LEA',0,0,0,  0x00
    dw InstLEA
    db 'LES',0,0,0,  0xC4
    dw InstLXS
    db 'LDS',0,0,0,  0xC5
    dw InstLXS

    ; INC/DEC
    db 'INC',0,0,0,  0x00
    dw InstIncDec
    db 'DEC',0,0,0,  0x01
    dw InstIncDec

    ; NOT/NEG (argument is /r)
    db 'NOT',0,0,0,  0x02
    dw InstNotNeg
    db 'NEG',0,0,0,  0x03
    dw InstNotNeg

    ; ALU instructions (argument is base instruction)
    db 'ADD',0,0,0,  0x00
    dw InstALU
    db 'OR',0,0,0,0, 0x08
    dw InstALU
    db 'ADC',0,0,0,  0x10
    dw InstALU
    db 'SBB',0,0,0,  0x18
    dw InstALU
    db 'AND',0,0,0,  0x20
    dw InstALU
    db 'SUB',0,0,0,  0x28
    dw InstALU
    db 'XOR',0,0,0,  0x30
    dw InstALU
    db 'CMP',0,0,0,  0x38
    dw InstALU

    ; BCD instructions
    db 'DAA',0,0,0,  0x27
    dw OutputByte
    db 'DAS',0,0,0,  0x2F
    dw OutputByte
    db 'AAA',0,0,0,  0x37
    dw OutputByte
    db 'AAS',0,0,0,  0x3F
    dw OutputByte
    db 'AAM',0,0,0,  0xD4
    dw InstAAX
    db 'AAD',0,0,0,  0xD5
    dw InstAAX

    ; Misc
    db 'NOP',0,0,0,  0x90
    dw OutputByte
    db 'CBW',0,0,0,  0x98
    dw OutputByte
    db 'CWD',0,0,0,  0x99
    dw OutputByte
    db 'PUSHF',0,    0x9C
    dw OutputByte
    db 'POPF',0,0,   0x9D
    dw OutputByte
    db 'SAHF',0,0,   0x9E
    dw OutputByte
    db 'LAHF',0,0,   0x9F
    dw OutputByte
    db 'XLATB',0,    0xD7
    dw OutputByte

    ; I/O
    db 'IN',0,0,0,0, 0x00
    dw InstIN
    db 'OUT',0,0,0,  0x00
    dw InstOUT

    ; Prefixes
    db 'REPNE',0,    0xF2
    dw OutputByte
    db 'REPE',0,0,   0xF3
    dw OutputByte
    db 'REP',0,0,0,  0xF3
    dw OutputByte

    ; Flags/etc.
    db 'CMC',0,0,0,  0xF5
    dw OutputByte
    db 'CLC',0,0,0,  0xF8
    dw OutputByte
    db 'STC',0,0,0,  0xF9
    dw OutputByte
    db 'CLI',0,0,0,  0xFA
    dw OutputByte
    db 'STI',0,0,0,  0xFB
    dw OutputByte
    db 'CLD',0,0,0,  0xFC
    dw OutputByte
    db 'STD',0,0,0,  0xFD
    dw OutputByte

    ; Mul/Div instructions (argument is /r)
    db 'MUL',0,0,0,  0x04
    dw InstMulDiv
    db 'IMUL',0,0,   0x05
    dw InstMulDiv
    db 'DIV',0,0,0,  0x06
    dw InstMulDiv
    db 'IDIV',0,0,   0x07
    dw InstMulDiv

    ; Rotate instructions (argument is /r)
    db 'ROL',0,0,0,  0x00
    dw InstROT
    db 'ROR',0,0,0,  0x01
    dw InstROT
    db 'RCL',0,0,0,  0x02
    dw InstROT
    db 'RCR',0,0,0,  0x03
    dw InstROT
    db 'SHL',0,0,0,  0x04
    dw InstROT
    db 'SHR',0,0,0,  0x05
    dw InstROT
    db 'SAR',0,0,0,  0x07
    dw InstROT

    ; Stack instructions
    db 'POP',0,0,0,  0x00
    dw InstPOP
    db 'POPA',0,0,   0x61
    dw OutputByte186
    db 'PUSH',0,0,   0x00
    dw InstPUSH
    db 'PUSHA',0,    0x60
    dw OutputByte186

    ; String instructions
    db 'INSB',0,0,   0x6C
    dw OutputByte186
    db 'INSW',0,0,   0x6D
    dw OutputByte186
    db 'OUTSB',0,    0x6E
    dw OutputByte186
    db 'OUTSW',0,    0x6F
    dw OutputByte186
    db 'MOVSB',0,    0xA4
    dw OutputByte
    db 'MOVSW',0,    0xA5
    dw OutputByte
    db 'CMPSB',0,    0xA6
    dw OutputByte
    db 'CMPSW',0,    0xA7
    dw OutputByte
    db 'STOSB',0,    0xAA
    dw OutputByte
    db 'STOSW',0,    0xAB
    dw OutputByte
    db 'LODSB',0,    0xAC
    dw OutputByte
    db 'LODSW',0,    0xAD
    dw OutputByte
    db 'SCASB',0,    0xAE
    dw OutputByte
    db 'SCASW',0,    0xAF
    dw OutputByte

    ; Flow control
    db 'RET',0,0,0,  0xC3
    dw OutputByte
    db 'RETF',0,0,   0xCB
    dw OutputByte
    db 'IRET',0,0,   0xCF
    dw OutputByte
    db 'HLT',0,0,0,  0xF4
    dw OutputByte
    db 'INT',0,0,0,  0x00
    dw InstINT
    db 'INT3',0,0,   0xCC
    dw OutputByte
    db 'INTO',0,0,   0xCE
    dw OutputByte
    db 'CALL',0,0,   0x00
    dw InstCALL

    ; JMP
    db 'JMP',0,0,0,  0x00
    dw InstJMP

    ; JCXZ and looping instructions (argument is opcode)
    db 'LOOPNE',     0xE0
    dw InstJ8
    db 'LOOPE',0,    0xE1
    dw InstJ8
    db 'LOOP',0,0,   0xE2
    dw InstJ8
    db 'JCXZ',0,0,   0xE3
    dw InstJ8

    ; Conditional jump instructions (argument is condition code)
    db 'JO',0,0,0,0, CC_O
    dw InstJCC
    db 'JNO',0,0,0,  CC_NO
    dw InstJCC
    db 'JC',0,0,0,0, CC_C
    dw InstJCC
    db 'JB',0,0,0,0, CC_C
    dw InstJCC
    db 'JNC',0,0,0,  CC_NC
    dw InstJCC
    db 'JNB',0,0,0,  CC_NC
    dw InstJCC
    db 'JAE',0,0,0,  CC_NC
    dw InstJCC
    db 'JZ',0,0,0,0, CC_Z
    dw InstJCC
    db 'JE',0,0,0,0, CC_Z
    dw InstJCC
    db 'JNZ',0,0,0,  CC_NZ
    dw InstJCC
    db 'JNE',0,0,0,  CC_NZ
    dw InstJCC
    db 'JNA',0,0,0,  CC_NA
    dw InstJCC
    db 'JBE',0,0,0,  CC_NA
    dw InstJCC
    db 'JA',0,0,0,0, CC_A
    dw InstJCC
    db 'JS',0,0,0,0, CC_S
    dw InstJCC
    db 'JNS',0,0,0,  CC_NS
    dw InstJCC
    db 'JPE',0,0,0,  CC_PE
    dw InstJCC
    db 'JPO',0,0,0,  CC_PO
    dw InstJCC
    db 'JL',0,0,0,0, CC_L
    dw InstJCC
    db 'JNL',0,0,0,  CC_NL
    dw InstJCC
    db 'JNG',0,0,0,  CC_NG
    dw InstJCC
    db 'JG',0,0,0,0, CC_G
    dw InstJCC

DispatchListEnd:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

BSS:

FirstFreeSeg:     resw 1

CpuLevel:         resb 1

;;; Parser
InputFile:        resw 1 ; Input file handle
CurrentLine:      resw 1 ; Current line being processed
NumNewLines:      resw 1 ; Number of newlines passed by ReadNext
CurrentChar:      resb 1 ; Current input character (0 on EOF)
GotNL:            resb 1 ; Passed NL since last reset of var?
CurrentOp:        resw 1 ; Current operand in GetExpr (low byte op, high byte precedence)
IsTokenNumber:    resb 1 ; Is The current token a number?
TokenLen:         resb 1
Token:            resb TOKEN_MAX
                  resb 1 ; NUL-terminator
UToken:           resb TOKEN_MAX
                  resb 1 ; NUL
InputBufferPos:   resw 1
InputBufferBytes: resw 1
InputBuffer:      resb BUFFER_SIZE

; The last parsed operand is in OperandType/OperandValue
; Instructions with 2 operands have the left most operand saved in
; OperandLType/OperandLValue. CurrentFixup/CurrentLFixup holds
; any fixups needed for the operands (or INVALID_ADDR)
; See the EQUs at the top of the file for the meaning
OperandType:      resb 1
OperandValue:     resw 1
CurrentFixup:     resw 1
OperandLType:     resb 1
OperandLValue:    resw 1
CurrentLFixup:    resw 1

ExplicitSize:     resb 1 ; Explicit size of memory operand (0=none, 1=byte, 2=word)

LabelSeg:         resw 1
NumLabels:        resw 1

FixupSeg:         resw 1
NextFreeFixup:    resw 1 ; Points to next free fixup node (or INVALID_ADDR)

EquSeg:           resw 1
NumEqus:          resw 1

Ifs:              resb IF_MAX
NumIfs:           resb 1

;;; Output
OutputSeg:        resw 1
SectionStart:     resw 1
CurrentAddress:   resw 1 ; Current memory address of code (e.g. 0x100 first in a COM file)
NumOutBytes:      resw 1 ; Number of bytes output
PendingZeros:     resw 1 ; Number of zeros to output before next actual byte

InFileName:       resb 13
OutFileName:      resb 13

;;; Keep last
ProgramEnd:
