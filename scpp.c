// SCPP - Basic C pre-processor (part of SCC)
// Copyright 2020 Michael Rasmussen. See LICENSE.md for details.

#ifndef __SCC__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _MSC_VER
#include <io.h>
#define open _open
#define close _close
#define read _read
#define write _write
#else
#ifndef O_BINARY
#define O_BINARY 0
#endif
#include <unistd.h>
#endif

#else
// Small "standard library"

enum {
    O_RDONLY = 0,
    O_WRONLY = 1,
    O_CREAT  = 1,
    O_TRUNC  = 1,
    O_BINARY = 0,
};

void memcpy(void* dst, void* src, int size)
{
    _emit 0x8B _emit 0x7E _emit 0x04 // MOV DI,[BP+0x4]
    _emit 0x8B _emit 0x76 _emit 0x06 // MOV SI,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // MOV CX,[BP+0x8]
    _emit 0xF3 _emit 0xA4            // REP MOVSB
}

void memset(void* ptr, int val, int size)
{
    _emit 0x8B _emit 0x7E _emit 0x04 // MOV DI,[BP+0x4]
    _emit 0x8B _emit 0x46 _emit 0x06 // MOV AX,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // MOV CX,[BP+0x8]
    _emit 0xF3 _emit 0xAA            // REP STOSB
}

int DosCall(int* ax, int bx, int cx, int dx)
{
    // Use segment override to allow DS to not point at data segment

    _emit 0x8B _emit 0x5E _emit 0x04 // 8B5E04            MOV BX,[BP+0x4]
    _emit 0x36 _emit 0x8B _emit 0x07 // 8B07              MOV AX,[SS:BX]
    _emit 0x8B _emit 0x5E _emit 0x06 // 8B5E06            MOV BX,[BP+0x6]
    _emit 0x8B _emit 0x4E _emit 0x08 // 8B4E08            MOV CX,[BP+0x8]
    _emit 0x8B _emit 0x56 _emit 0x0A // 8B560A            MOV DX,[BP+0xA]
    _emit 0xCD _emit 0x21            // CD21              INT 0x21
    _emit 0x8B _emit 0x5E _emit 0x04 // 8B5E04            MOV BX,[BP+0x4]
    _emit 0x36 _emit 0x89 _emit 0x07 // 8907              MOV [SS:BX],AX
    _emit 0xB8 _emit 0x00 _emit 0x00 // B80000            MOV AX,0x0
    _emit 0x19 _emit 0xC0            // 19C0              SBB AX,AX
}

void exit(int retval)
{
    retval = (retval & 0xff) | 0x4c00;
    DosCall(&retval, 0, 0, 0);
}

void raw_putchar(int ch)
{
    int ax = 0x200;
    DosCall(&ax, 0, 0, ch);
}

void putchar(int ch)
{
    if (ch == '\n') raw_putchar('\r');
    raw_putchar(ch);
}

int open(const char* filename, int flags, ...)
{
    int ax;
    if (flags)
        ax = 0x3c00; // create or truncate file
    else
        ax = 0x3d00; // open existing file

    if (DosCall(&ax, 0, 0, filename))
        return -1;
    return ax;
}

void close(int fd)
{
    int ax = 0x3e00;
    DosCall(&ax, fd, 0, 0);
}

int read(int fd, char* buf, int count)
{
    int ax = 0x3f00;
    if (DosCall(&ax, fd, count, buf))
        return 0;
    return ax;
}

int write(int fd, const char* buf, int count)
{
    int ax = 0x4000;
    if (DosCall(&ax, fd, count, buf))
        return 0;
    return ax;
}

enum { _ARGV = 0x5C }; // Overwrite unopened FCBs

int ParseArgs()
{
    char** Args = _ARGV;
    char* CmdLine = (char*)0x80;
    int Len = *CmdLine++ & 0x7f;
    CmdLine[Len] = 0;
    Args[0] = "???";
    int NumArgs = 1;
    for (;;) {
        while (*CmdLine && *CmdLine <= ' ')
            ++CmdLine;
        if (!*CmdLine)
            break;
        Args[NumArgs++] = CmdLine;
        while (*CmdLine && *CmdLine > ' ')
            ++CmdLine;
        if (!*CmdLine)
            break;
        *CmdLine++ = 0;
    }
    Args[NumArgs] = 0;
    return NumArgs;
}

int main(int argc, char** argv);

void _start(void)
{
    // Clear BSS
    memset(&_SBSS, 0, &_EBSS-&_SBSS);
    exit(main(ParseArgs(), _ARGV));
}

char* CopyStr(char* dst, const char* src)
{
    while (*src) *dst++ = *src++;
    *dst = 0;
    return dst;
}

char* vsprintf(char* dest, const char* format, va_list vl)
{
    char TempBuf[9];
    char ch;
    while ((ch = *format++)) {
        if (ch != '%') {
            *dest++ = ch;
            continue;
        }
        ch = *format++;
        if (ch == 's') {
            dest = CopyStr(dest, va_arg(vl, char*));
        } else if(ch == 'c') {
            *dest++ = (char)va_arg(vl, int);
        } else if ((ch == '+' && *format == 'd') || ch == 'd') {
            char* buf;
            int n;
            int s;
            int always;
            if (ch == '+') {
                ++format;
                always = 1;
            } else {
                always = 0;
            }
            n = va_arg(vl, int);
            s = 0;
            if (n < 0) {
                s = 1;
                n = -n;
            }
            buf = TempBuf + sizeof(TempBuf);
            *--buf = 0;
            do {
                *--buf = '0' + n % 10;
                n/=10;
            } while (n);
            if (s) *--buf = '-';
            else if (always) *--buf = '+';
            dest  = CopyStr(dest, buf);
        } else if (ch == 'X') {
            const char* HexD = "0123456789ABCDEF";
            int n = va_arg(vl, int);
            int i;
            for (i = 3; i >= 0; --i) {
                *dest++ = HexD[(n>>i*4)&0xf];
            }
        } else {
            *dest++ = ch;
        }
    }
    *dest = 0;
    return dest;
}

void sprintf(char* str, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf(str, format, args);
    va_end(args);
}

    char msg[256]; // XXX
void printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);
    char* s = msg;
    while (*s) putchar(*s++);
}

void strcpy(char* dst, const char* src)
{
    CopyStr(dst, src);
}

int strcmp(const char* a, const char* b)
{
    _emit 0x8B _emit 0x76 _emit 0x04    // MOV SI, [BP+4]
    _emit 0x8B _emit 0x7E _emit 0x06    // MOV DI, [BP+6]
    _emit 0xAC                          // L: LODSB
    _emit 0x8A _emit 0x25               // MOV AH, [DI]
    _emit 0x47                          // INC DI
    _emit 0x28 _emit 0xE0               // SUB AL, AH
    _emit 0x75 _emit 0x04               // JNZ DONE
    _emit 0x20 _emit 0xE4               // AND AH, AH
    _emit 0x75 _emit 0xF4               // JNZ L
    _emit 0x98                          // DONE: CBW
}

int strlen(const char* s)
{
    _emit 0x8B _emit 0x7E _emit 0x04    // MOV DI, [BP+4]
    _emit 0x30 _emit 0xC0               // XOR AL, AL
    _emit 0xB9 _emit 0xFF _emit 0xFF    // MOV CX, -1
    _emit 0xF2 _emit 0xAE               // REPNE SCASB
    _emit 0xB8 _emit 0xFE _emit 0xFF    // MOV AX, -2
    _emit 0x29 _emit 0xC8               // SUB AX, CX
}

int* GetBP(void)
{
    _emit 0x8B _emit 0x46 _emit 0x00 // MOV AX, [BP]
}

void assert(int res)
{
    if (!res) {
        printf("Assertion failed\n");
        int* BP = GetBP();
        printf("\nBP   Return address\n");
        while (*BP) {
            printf("%X %X\n", BP[0], BP[1]);
            BP = (int*)BP[0];
        }
        exit(1);
    }
}
#endif

enum {
    ID_MAX          = 32,
    ID_HASH_MAX     = 128,  // Must be power of 2 and larger than ID_MAX
    TOKEN_DATA_MAX  = 2048,
    MAX_NEST        = 10,   // Maximum nesting of replacments
    COND_MAX        = 16,
    TOKEN_LIST_MAX  = 128,
    TOKEN_QUEUE_MAX = 16,
    ARGUMENT_MAX    = 32,
    FILES_MAX       = 4,
    FILE_BUF_SIZE   = 512,
    FNAME_MAX       = 64,
};

enum {
    TOK_EOF,
    TOK_WHITESPACE,
    TOK_NEWLINE,
    TOK_NUM,
    TOK_CHARLIT,
    TOK_STRLIT,
    TOK_INVALID_LIT, // Unterminated string literal

    TOK_ARG, // Text points to arguments

    TOK_CREATED_OP = 128,
    TOK_HASHHASH,
    TOK_LTEQ,
    TOK_GTEQ,
    TOK_EQEQ,
    TOK_NOTEQ,
    TOK_ANDAND,
    TOK_OROR,

    TOK_RAW_ID   = 256, // Hasn't been looked up
    // Keywords
    TOK_DEFINE,
    TOK_UNDEF,
    TOK_IF,
    TOK_IFDEF,
    TOK_IFNDEF,
    TOK_ELIF,
    TOK_ELSE,
    TOK_ENDIF,
    TOK_INCLUDE,
    TOK_LINE,
    TOK_ERROR,
    TOK_PRAGMA,
    TOK_DEFINED,
    TOK___FILE__,
    TOK___LINE__,
    TOK___SCC__,

    TOK_ID = TOK_DEFINE,
    TOK_USER_ID = TOK___SCC__+1,
};

struct File {
    const char* Filename;
    int FD;
    char* BufPtr;
    int BufSize;
    int OldLine;
    char LastChar;
    char Buf[FILE_BUF_SIZE];
};

struct File Files[FILES_MAX];
int NumFiles;
struct File* CurFile;
struct File* OutFile;
const char* CurFileBuf;

char TokenData[TOKEN_DATA_MAX];
int TokenDataPos;
int NextTokenDataPos; // Bump this to avoid the current token being overwritten

const char* IdText[ID_MAX];
unsigned IdHash[ID_HASH_MAX];
int IdCount;

int Line = 1;

char CurrentChar;

struct Token {
    int Type;
    const char* Text;
};

struct Token CurTok;
unsigned CurHash;

struct Token TokenQueue[TOKEN_QUEUE_MAX];
int TokenQueueHead, TokenQueueTail;

struct TokenList {
    struct Token Tok;
    struct TokenList* Next;
};
struct TokenList TokenListElems[TOKEN_LIST_MAX];
struct TokenList* NextFreeTokenList;

struct Argument {
    int Id;
    struct Argument* Next;
    struct TokenList* Replacement;
};
struct Argument Argument[ARGUMENT_MAX];
int ArgumentCount;

struct Macro {
    struct Argument*  Arguments;
    struct TokenList* Replacement;
    char Hide; // 0=Not hidden, 1=Hidden while being expanded, 2=Undefined
};
enum { MACRO_MAX = ID_MAX-(TOK_USER_ID-TOK_ID) };
struct Macro Macros[MACRO_MAX];

struct ReplaceStackElem {
    struct Macro* M;
    struct TokenList* TL;
    int Stop;
} ReplaceStack[MAX_NEST];
int ReplaceNest;

enum {
    COND_TRUE,        // In active block
    COND_FALSE,       // In inactive block
    COND_DONE,        // Done, waiting for #endif
};
char CondState[COND_MAX];
int CondCount;

enum { Debug = 0 };

struct File* OpenFile(const char* Filename, int Write)
{
    if (Debug) printf("Open file '%s' write=%d\n", Filename, Write);
    assert(NumFiles < FILES_MAX);
    struct File* F = &Files[NumFiles++];
    memset(F, 0, sizeof(*F));
    F->BufPtr = F->Buf;
    F->Filename = Filename;
    F->FD = open(F->Filename, Write ? O_WRONLY | O_CREAT | O_TRUNC | O_BINARY : O_RDONLY | O_BINARY, 0600);
    if (F->FD < 0) {
        printf("Error opening %s\n", F->Filename);
        exit(1);
    }
    return F;
}

void CloseFile(void)
{
    assert(NumFiles);
    --NumFiles;
    if (Debug) printf("Closing %s\n", Files[NumFiles].Filename);
    close(Files[NumFiles].FD);
}

void ReadChar(void)
{
    if (CurrentChar == '\n') ++Line;
    if ((CurrentChar = *CurFileBuf++))
        return;

    CurFileBuf = CurFile->Buf;
    CurFile->BufSize = read(CurFile->FD, CurFile->Buf, sizeof(CurFile->Buf)-1);
    if (CurFile->BufSize <= 0) {
        CurFile->BufSize = 0;
        CurrentChar = 0;
    } else {
        CurrentChar = *CurFileBuf++;
    }
    CurFile->Buf[CurFile->BufSize] = 0;
}

void ReadToTokenData(void)
{
    assert(TokenDataPos < TOKEN_DATA_MAX);
    TokenData[TokenDataPos++] = CurrentChar;
    if ((CurrentChar = *CurFileBuf++))
        return;
    --CurFileBuf;
    ReadChar();
}

int TryExtendOp(int Ch, int NewType)
{
    if (CurrentChar == Ch) {
        ReadToTokenData();
        CurTok.Type = NewType;
        return 1;
    }
    return 0;
}

struct TokenList** AddToken(struct TokenList** L, struct Token* Tok)
{
    assert(NextFreeTokenList);
    struct TokenList* E = NextFreeTokenList;
    NextFreeTokenList = E->Next;
    E->Next = 0;
    *L = E;
    E->Tok = *Tok;
    return &E->Next;
}

void FreeTokenList(struct TokenList* TL)
{
    if (!TL) return;
    struct TokenList* Last = NextFreeTokenList;
    NextFreeTokenList = TL;
    for (;;) {
        if (!TL->Next) {
            TL->Next = Last;
            break;
        }
        TL = TL->Next;
    }
}

struct TokenList* SkipWsNL(struct TokenList* TL)
{
    while (TL) {
        const int T = TL->Tok.Type;
        if (T != TOK_WHITESPACE && T != TOK_NEWLINE)
            break;
        TL = TL->Next;
    }
    return TL;
}

void FreeArgs(struct Argument* Args)
{
    if (Args == (struct Argument*)1)
        return;
    while (Args) {
        FreeTokenList(Args->Replacement);
        Args->Replacement = 0;
        Args = Args->Next;
    }
}

void TokenQueuePush(struct Token* Tok)
{
    assert(TokenQueueTail - TokenQueueHead < TOKEN_QUEUE_MAX);
    TokenQueue[TokenQueueTail++ % TOKEN_QUEUE_MAX] = *Tok;
}

struct Token* SimpleExpand(struct Token* Tok)
{
    while (Tok->Type == TOK_ARG) {
        const struct Argument* A = (const struct Argument*)Tok->Text;
        assert(A->Replacement && !A->Replacement->Next);
        Tok = &A->Replacement->Tok;
    }
    return Tok;
}

void PrintToken(const struct Token* T);

void PasteAppend(struct Token* Tok)
{
    CurTok = *SimpleExpand(&CurTok);
    Tok = SimpleExpand(Tok);
    if (Debug) { printf("Paste "); PrintToken(Tok); printf(" to "); PrintToken(&CurTok); printf("\n"); }
    const int l1 = (int)strlen(CurTok.Text);
    const int l2 = (int)strlen(Tok->Text);
    assert(NextTokenDataPos+l1+l2+1 < TOKEN_DATA_MAX);
    memcpy(&TokenData[NextTokenDataPos], CurTok.Text, l1);
    memcpy(&TokenData[NextTokenDataPos+l1], Tok->Text, l2);
    TokenData[NextTokenDataPos+l1+l2] = 0;
    CurTok.Text = &TokenData[NextTokenDataPos];
    NextTokenDataPos+=l1+l2+1;
    if (CurTok.Type > ' ' && CurTok.Type < TOK_RAW_ID) {
        CurTok.Type = TOK_CREATED_OP;
    }
    if (Debug) { printf(" --> "); PrintToken(&CurTok); printf("\n"); }
}

const char IsIdChar[] = "\x00\x00\x00\x00\x00\x00\xFF\x03\xFE\xFF\xFF\x87\xFE\xFF\xFF\x07\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

void RawGetToken(void)
{
    while (ReplaceNest) {
        struct ReplaceStackElem* RS = &ReplaceStack[ReplaceNest-1];
        assert(!RS->M || RS->M->Hide == 1);
        if (RS->TL) {
            CurTok = RS->TL->Tok;
            RS->TL = RS->TL->Next;

            // Look for paste operation
            struct TokenList* TL = SkipWsNL(RS->TL);
            while (TL && TL->Tok.Type == TOK_HASHHASH) {
                TL = SkipWsNL(TL->Next);
                assert(TL);
                PasteAppend(&TL->Tok);
                TL = SkipWsNL(TL->Next);
                RS->TL = TL;
            }

            CurHash = ID_HASH_MAX;
            if (Debug) { printf("Returning from replacement list: ");PrintToken(&CurTok);printf("\n"); }
            return;
        }
        if (RS->M) {
            RS->M->Hide = 0;
            FreeArgs(RS->M->Arguments);
        }
        --ReplaceNest;
        if (Debug) printf("Popped replacement list (Stop=%d)\n", RS->Stop);
        if (RS->Stop) {
            CurTok.Type = TOK_EOF;
            return;
        }
    }

    if (TokenQueueHead != TokenQueueTail) {
        CurTok = TokenQueue[TokenQueueHead++ % TOKEN_QUEUE_MAX];
        CurHash = ID_HASH_MAX;
        return;
    }

Restart:
    TokenDataPos = NextTokenDataPos;
    CurTok.Text = TokenData + TokenDataPos;

    switch (CurrentChar) {
    case '\n':
        CurTok.Type = TOK_NEWLINE;
        ReadChar();
        return;
    case '/':
        ReadChar();
        if (CurrentChar == '/') {
            do {
                while ((CurrentChar = *CurFileBuf++)) {
                    if (CurrentChar == '\n')
                        goto Restart;
                }
                --CurFileBuf;
                ReadChar();
            } while (CurrentChar && CurrentChar != '\n');
            goto Restart;
        } else if (CurrentChar == '*') {
            ReadChar();
            int star = 0;
            while (CurrentChar && (!star || CurrentChar != '/')) {
                star = CurrentChar == '*';
                ReadChar();
            }
            ReadChar();
            goto Restart;
        }
        CurTok.Type = '/';
        assert(TokenDataPos < TOKEN_DATA_MAX);
        TokenData[TokenDataPos++] = '/';
        goto Out;
    case '\\':
        CurTok.Type = CurrentChar;
        ReadToTokenData();
        if (CurrentChar == '\r') {
            ReadChar();
            assert(CurrentChar == '\n');
        }
        if (CurrentChar != '\n')
            goto Out;
        ReadChar();
        goto Restart;
    case '"':
    case '\'':
        {
            const int End = CurrentChar;
            ReadChar();
            CurTok.Type = End == '"' ? TOK_STRLIT : TOK_CHARLIT;
            int esc = 0;
            while (esc || CurrentChar != End) {
                if (!CurrentChar || CurrentChar == '\n') {
                    CurTok.Type = TOK_INVALID_LIT; // To support this happening in in-active block
                    goto Out;
                }
                if (CurrentChar == '\\')
                    esc = !esc;
                else
                    esc = 0;
                ReadToTokenData();
            }
            ReadChar();
            goto Out;
        }
    case 0:
        if (CurTok.Type == TOK_EOF) {
            // Fake up a newline to break out of loops
            CurTok.Type = TOK_NEWLINE;
        } else {
            CurTok.Type = TOK_EOF;
        }
        return;
    case '_':
        goto Identifier;
    }

    if (CurrentChar <= ' ') {
        CurTok.Type = TOK_WHITESPACE;
        ReadChar();
        if (!CurrentChar)
            return;
        while (CurrentChar <= ' ' && CurrentChar != '\n') {
            if ((CurrentChar = *CurFileBuf++))
                continue;
            --CurFileBuf;
            ReadChar();
            if (!CurrentChar)
                return;
        }
        return;
    }

    if (CurrentChar < 'A') {
        if ((unsigned)(CurrentChar - '0') < 10) {
            CurTok.Type = TOK_NUM;
            ReadToTokenData();
            if (CurrentChar == 'x' || CurrentChar == 'X')
                ReadToTokenData();
            while ((IsIdChar[(unsigned char)CurrentChar>>3]>>(CurrentChar&7)) & 1) {
                TokenData[TokenDataPos++] = CurrentChar;
                if ((CurrentChar = *CurFileBuf++))
                    continue;
                --CurFileBuf;
                ReadChar();
            }
            goto Out;
        }
    } else if ((CurrentChar&0xDF) <= 'Z') {
Identifier:
        CurTok.Type = TOK_RAW_ID;
        CurHash = 0;
        do {
            CurHash += (CurHash<<4)+(TokenData[TokenDataPos++] = CurrentChar);
            if (!(CurrentChar = *CurFileBuf++)) {
                --CurFileBuf;
                ReadChar();
            }
        } while ((IsIdChar[(unsigned char)CurrentChar>>3]>>(CurrentChar&7)) & 1);
        CurHash &= ID_HASH_MAX-1;
        goto Out;
    }
    CurTok.Type = CurrentChar;
    ReadToTokenData();
    switch (CurTok.Type) {
    case '#':
        TryExtendOp('#', TOK_HASHHASH);
        break;
    case '=':
        TryExtendOp('=', TOK_EQEQ);
        break;
    case '!':
        TryExtendOp('=', TOK_NOTEQ);
        break;
    case '<':
        TryExtendOp('=', TOK_LTEQ);
        break;
    case '>':
        TryExtendOp('=', TOK_GTEQ);
        break;
    case '&':
        TryExtendOp('&', TOK_ANDAND);
        break;
    case '|':
        TryExtendOp('|', TOK_OROR);
        break;
    case '*':
    case '/':
    case '%':
    case '+':
    case '-':
    case '^':
    case '(':
    case ')':
    case '?':
    case ':':
    case ',':
    case ';':
    case '{':
    case '}':
    case '[':
    case ']':
    case '.':
    case '~':
        break;
    default:
        printf("Unknown token type %c (%02X)\n", CurTok.Type, CurTok.Type);
        assert(0);
    }
Out:
    assert(TokenDataPos < TOKEN_DATA_MAX);
    TokenData[TokenDataPos++] = 0;
    if (Debug) { printf("Returning new token: ");PrintToken(&CurTok);printf("\n"); }
}

const char* TokenTypeText(int Type)
{
    switch (Type) {
    case TOK_EOF:        return "EOF";
    case TOK_WHITESPACE: return "WS";
    case TOK_NEWLINE:    return "NL";
    case TOK_NUM:        return "NUM";
    case TOK_CHARLIT:    return "CHARLIT";
    case TOK_STRLIT:     return "STRLIT";
    case TOK_ARG:        return "ARG";
    case TOK_CREATED_OP: return "CREATED";
    case TOK_RAW_ID:     return "RAWID";
    }
    return Type >= TOK_ID ? "ID" : "OP";
}

void PrintToken(const struct Token* T)
{
    printf("%s ", TokenTypeText(T->Type));
    if (T->Type == TOK_ARG)
        printf("for %s ", IdText[((struct Argument*)T->Text)->Id - TOK_ID]);
    else if (T->Type != TOK_EOF && T->Type != TOK_WHITESPACE && T->Type != TOK_NEWLINE)
        printf("'%s' ", T->Text);
}

int AddId(const char* Text)
{
    assert(IdCount < ID_MAX);
    const int Id = IdCount++;
    IdText[Id] = Text;
    unsigned H = 0;
    while (*Text)
        H += (H<<4)+*Text++;
    while (IdHash[H &= ID_HASH_MAX-1] < ID_MAX)
        ++H;
    IdHash[H] = Id;
    return Id;
}

void ReplaceId(void)
{
    assert(CurTok.Type == TOK_RAW_ID);

    if (CurHash == ID_HASH_MAX) {
        CurHash = 0;
        const char* Text = CurTok.Text;
        while (*Text)
            CurHash += (CurHash<<4)+*Text++;
        CurHash &= ID_HASH_MAX-1;
    }

    for (;;) {
        unsigned Id = IdHash[CurHash];
        if (Id >= ID_MAX)
            return;
        if (!strcmp(CurTok.Text, IdText[Id])) {
            CurTok.Type = TOK_ID + Id;
            return;
        }
        CurHash = (CurHash+1)&(ID_HASH_MAX-1);
    }
}

void RawSkipWS(void)
{
    while (CurTok.Type == TOK_WHITESPACE)
        RawGetToken();
}

void Stringify(struct TokenList* TL);

void PushHandleArg(struct Argument* A)
{
    if (Debug) {
        printf("Handling arg %s\n", IdText[A->Id-TOK_ID]);
    }
    assert(ReplaceNest < MAX_NEST);
    ReplaceStack[ReplaceNest].M  = 0;
    ReplaceStack[ReplaceNest].TL = A->Replacement;
    ReplaceStack[ReplaceNest].Stop = 0;
    ++ReplaceNest;
}

struct Argument* GetArg(int Id)
{
    if (ReplaceNest && ReplaceStack[ReplaceNest-1].M) {
        struct Argument* A = ReplaceStack[ReplaceNest-1].M->Arguments;
        for (; A; A = A->Next) {
            if (A->Id == Id) {
                if (Debug) {
                    printf("Matched argument %s\n", IdText[Id-TOK_ID]);
                    struct TokenList* TL = A->Replacement;
                    while (TL) {
                        printf("\t"); PrintToken(&TL->Tok); printf("\n");
                        TL = TL->Next;
                    }
                }
                return A;
            }
        }
    }
    return 0;
}

void GetToken(void)
{
    for (;;) {
        RawGetToken();

        switch (CurTok.Type) {
        case TOK_RAW_ID:
            break;
        case '#':
            if (!ReplaceNest)
                return;
            if (Debug) printf("Stringify!\n");
            RawGetToken();
            RawSkipWS();
            assert(CurTok.Type == TOK_ARG);
            Stringify(((struct Argument*)CurTok.Text)->Replacement);
            return;
        case TOK_ARG:
            PushHandleArg((struct Argument*)CurTok.Text);
            continue;
        default:
            return;
        }

        ReplaceId();
        if (CurTok.Type < TOK_USER_ID) {
            if (CurTok.Type > TOK_DEFINED) {
                if (Debug) printf("Special token! %s\n", CurTok.Text);
                if (CurTok.Type == TOK___LINE__) {
                    char* T = &TokenData[NextTokenDataPos];
                    sprintf(T, "%d", Line);
                    NextTokenDataPos += (int)strlen(T) + 1;
                    CurTok.Text = T;
                    CurTok.Type = TOK_NUM;
                } else if (CurTok.Type == TOK___FILE__) {
                    CurTok.Text = CurFile->Filename;
                    CurTok.Type = TOK_STRLIT;
                } else {
                    CurTok.Text = "1";
                    CurTok.Type = TOK_NUM;
                }
            }
            return;
        }
        const int Id = CurTok.Type;
        struct Argument* A = GetArg(Id);
        if (A) {
            assert(ReplaceNest < MAX_NEST);
            ReplaceStack[ReplaceNest].M  = 0;
            ReplaceStack[ReplaceNest].TL = A->Replacement;
            ReplaceStack[ReplaceNest].Stop = 0;
            ++ReplaceNest;
            continue;
        }
        struct Macro* M = &Macros[Id - TOK_USER_ID];
        if (M->Hide)
            return;
        if (Debug) printf("Start expanding macro %s\n", IdText[Id-TOK_ID]);
        if (M->Arguments) {
            RawGetToken();
            int WS = CurTok.Type;
            RawSkipWS();
            if (CurTok.Type != '(') {
                // Undo consumption of ID and any whitespace
                if (WS < ' ') {
                    struct Token WSToken;
                    WSToken.Type = WS;
                    TokenQueuePush(&WSToken);
                }
                TokenQueuePush(&CurTok);
                CurTok.Type = Id;
                CurTok.Text = IdText[Id - TOK_ID];
                return;
            }
            RawGetToken();
            RawSkipWS();
            if (CurTok.Type != ')') {
                assert(M->Arguments != (struct Argument*)1);
                A = M->Arguments;
                while (A) {
                    struct TokenList** ArgL = &A->Replacement;
                    int Nest = 0;
                    int PendWS = -1;
                    for (;; RawGetToken()) {
                        if (CurTok.Type == TOK_WHITESPACE || CurTok.Type == TOK_NEWLINE) {
                            if (PendWS == 0) PendWS = 1;
                            continue;
                        }
                        if (CurTok.Type == '(')
                            ++Nest;
                        else if (CurTok.Type == ')' && !Nest--)
                            break;
                        else if (!CurTok.Type || (!Nest && CurTok.Type == ','))
                            break;
                        if (PendWS == 1) {
                            struct Token WSTok;
                            WSTok.Type = TOK_WHITESPACE;
                            ArgL = AddToken(ArgL, &WSTok);
                        }
                        PendWS = 0;
                        ArgL = AddToken(ArgL, &CurTok);
                        NextTokenDataPos = TokenDataPos; // Preserve the token data
                    }
                    *ArgL = 0;
                    A = A->Next;
                    if (CurTok.Type != ',')
                        break;
                    RawGetToken();
                }
                assert(!A && CurTok.Type == ')');
            }
        }
        if (Debug) printf("Start expanding macro body of %s\n", IdText[Id-TOK_ID]);
        if (M->Replacement) {
            assert(ReplaceNest < MAX_NEST);
            ReplaceStack[ReplaceNest].M  = M;
            ReplaceStack[ReplaceNest].TL = M->Replacement;
            ReplaceStack[ReplaceNest].Stop = 0;
            M->Hide = 1;
            ++ReplaceNest;
        } else {
            FreeArgs(M->Arguments);
        }
    }
}

char StrBuf[256]; // HACK HACK

void Stringify(struct TokenList* TL)
{
    int StrPos = 0;
    int PendWS = 0;
    int FromS = 0;
    while (TL || FromS) {
        if (!FromS) {
            CurTok = TL->Tok;
            TL = TL->Next;
        } else {
            GetToken();
            if (CurTok.Type == TOK_EOF) {
                --FromS;
                if (Debug) printf("Hit stop: FromS=%d\n", FromS);
                continue;
            }
        }
        if (CurTok.Type == TOK_ARG) {
            ++FromS;
            if (Debug) printf("Switching to arg list FromS=%d\n", FromS);
            PushHandleArg((struct Argument*)CurTok.Text);
            ReplaceStack[ReplaceNest-1].Stop = 1;
            continue;
        }

        if (Debug) { printf("Stringifying: "); PrintToken(&CurTok); printf("\n"); }
        if (CurTok.Type == TOK_WHITESPACE || CurTok.Type == TOK_NEWLINE) {
            PendWS = 1;
        } else {
            if (PendWS) {
                assert(StrPos < (int)sizeof(StrBuf));
                StrBuf[StrPos++] = ' ';
                PendWS = 0;
            }
            const int l = (int)strlen(CurTok.Text);
            assert(StrPos+l < (int)sizeof(StrBuf));
            memcpy(&StrBuf[StrPos], CurTok.Text, l);
            StrPos += l;
        }
    }
    assert(StrPos < (int)sizeof(StrBuf));
    StrBuf[StrPos] = 0;

    CurTok.Type = TOK_STRLIT;
    CurTok.Text = StrBuf;
}

void HandleDefine(void)
{
    RawGetToken();
    RawSkipWS();
    assert(CurTok.Type == TOK_RAW_ID);
    ReplaceId();
    int Id = CurTok.Type;
    struct Macro* M;
    if (Id >= TOK_USER_ID) {
        M = &Macros[Id-TOK_USER_ID];
        if (!M->Hide) {
            // Macro redifition (must match previous definition)
            return;
        }
        // Macro was previously #undef'ed
        M->Hide = 0;
    } else {
        Id = AddId(CurTok.Text) + TOK_ID;
        NextTokenDataPos = TokenDataPos;
        assert(Id - TOK_USER_ID < MACRO_MAX);
        M = &Macros[Id-TOK_USER_ID];
    }
    if (Debug) printf("Defining macro %s\n", IdText[Id-TOK_ID]);
    RawGetToken();
    struct Argument** LastArg;
    if (CurTok.Type == '(') { // Function-like macros
        LastArg = &M->Arguments;
        RawGetToken();
        RawSkipWS();
        while (CurTok.Type != ')') {
            RawSkipWS();
            assert(CurTok.Type == TOK_RAW_ID);
            ReplaceId();
            if (CurTok.Type == TOK_RAW_ID) {
                CurTok.Type = AddId(CurTok.Text) + TOK_ID;
                NextTokenDataPos = TokenDataPos;
                assert(CurTok.Type - TOK_USER_ID < MACRO_MAX);
                Macros[CurTok.Type-TOK_USER_ID].Hide = 2; // Prented the Id refers to an undefined macro
            }
            assert(ArgumentCount < ARGUMENT_MAX);
            Argument[ArgumentCount].Id = CurTok.Type;
            *LastArg = &Argument[ArgumentCount];
            LastArg = &Argument[ArgumentCount].Next;
            ++ArgumentCount;
            RawGetToken();
            RawSkipWS();
            if (CurTok.Type != ',') {
                break;
            }
            RawGetToken();
        }
        *LastArg = 0;
        assert(CurTok.Type == ')');
        RawGetToken();
        if (!M->Arguments) M->Arguments = (struct Argument*)1;
    }
    RawSkipWS();
    struct TokenList** Last = &M->Replacement;
    int PendWS = -1;
    for (; CurTok.Type != TOK_NEWLINE; RawGetToken()) {
        if (CurTok.Type == TOK_WHITESPACE) {
            if (PendWS == 0) PendWS = 1;
            continue;
        }
        if (M->Arguments && CurTok.Type == TOK_RAW_ID) {
            ReplaceId();
            if (CurTok.Type >= TOK_USER_ID) {
                struct Argument* A = M->Arguments;
                while (A) {
                    if (A->Id == CurTok.Type) {
                        CurTok.Type = TOK_ARG;
                        CurTok.Text = (char*)A;
                        goto Add;
                    }
                    A = A->Next;
                }
            }
            CurTok.Type = TOK_RAW_ID;
        }
Add:
        if (PendWS == 1) {
            struct Token WS;
            WS.Type = TOK_WHITESPACE;
            Last = AddToken(Last, &WS);
        }
        PendWS = 0;
        Last = AddToken(Last, &CurTok);
        NextTokenDataPos = TokenDataPos; // Preserve the token data
    }
    *Last = 0;
}

void SkipWS(void)
{
    while (CurTok.Type == TOK_WHITESPACE)
        GetToken();
}

int Accept(int T)
{
    if (CurTok.Type == T) {
        GetToken();
        return 1;
    }
    return 0;
}

void Expect(int T)
{
    if (!Accept(T)) {
        printf("Expected %s got ", TokenTypeText(T));
        PrintToken(&CurTok);
        printf("\n");
        assert(0);
    }
}

int IsDefined(int Tok)
{
    return (Tok > TOK_DEFINED && Tok < TOK_USER_ID) || (Tok >= TOK_USER_ID && !Macros[Tok - TOK_USER_ID].Hide);
}

int EvalExpr(void);

int EvalUnary(void)
{
    int n = 0;
    SkipWS();
    if (CurTok.Type == TOK_NUM) {
        int base = 10;
        const char* s = CurTok.Text;
        if (*s == '0') {
            ++s;
            base = 8;
            if (*s == 'x' || *s == 'X') {
                base = 16;
                ++s;
            }
        }
        int ch;
        while ((ch = *s++)) {
            n *= base;
            if (ch >= '0' && ch <= '9')
                n += ch - '0';
            else {
                n += (ch & 0xdf) - ('A' - 10);
            }
        }
        GetToken();
        return n;
    } else if (CurTok.Type == TOK_RAW_ID) {
        GetToken();
        return 0;
    } else if (Accept(TOK_DEFINED)) {
        RawSkipWS();
        int Par = 0;
        if (CurTok.Type == '(') {
            Par = 1;
            RawGetToken();
            RawSkipWS();
        }
        ReplaceId();
        n = IsDefined(CurTok.Type);
        GetToken();
        if (Par) Expect(')');
        return n;
    } else if (Accept('!')) {
        return !EvalUnary();
    } else if (Accept('(')) {
        n = EvalExpr();
        Expect(')');
        return n;
    }

    if (CurTok.Type >= TOK_USER_ID) {
        // Undefined macro - evaluate as zero
        assert(Macros[CurTok.Type - TOK_USER_ID].Hide == 2);
        GetToken();
        return 0;
    }

    printf("TODO: EvalUnary() CurTok=");
    PrintToken(&CurTok);
    printf("\n");
    assert(!"TODO: Unary");
    return 0;
}

int OperatorPrecedence(int Op)
{
    switch (Op) {
    case '*': case '/': case '%': return 3;
    case '+': case '-': return 4;
    case '<': case '>': case TOK_LTEQ: case TOK_GTEQ: return 6;
    case TOK_EQEQ: case TOK_NOTEQ: return 7;
    case '&': return 8;
    case '^': return 9;
    case '|': return 10;
    case TOK_ANDAND: return 11;
    case TOK_OROR: return 12;
    case '?': return 13;
    }
    return 100;
}

int EvalExpr1(int LHS, int OuterPrec)
{
    for (;;) {
        SkipWS();
        const int Op = CurTok.Type;
        if (Op <= ' ' || Op >= TOK_ID)
            break;
        const int Prec = OperatorPrecedence(Op);
        if (Prec > OuterPrec)
            break;
        GetToken();
        if (Op == '?') {
            const int L = EvalExpr1(EvalUnary(), 14);
            Expect(':');
            const int R = EvalExpr1(EvalUnary(), 14);
            LHS = LHS ? L : R;
            continue;
        }
        int RHS = EvalUnary();
        for (;;) {
            SkipWS();
            const int LookAheadOp = CurTok.Type;
            if (LookAheadOp <= ' ' || LookAheadOp >= TOK_ID)
                break;
            const int LookAheadPrec = OperatorPrecedence(LookAheadOp);
            if (LookAheadPrec > Prec)
                break;
            RHS = EvalExpr1(RHS, LookAheadPrec);
        }
        switch (Op) {
        case '*': LHS *= RHS; break;
        case '/': LHS /= RHS; break;
        case '%': LHS %= RHS; break;
        case '+': LHS += RHS; break;
        case '-': LHS -= RHS; break;
        case '<': LHS = LHS < RHS; break;
        case '>': LHS = LHS < RHS; break;
        case TOK_LTEQ: LHS = LHS <= RHS; break;
        case TOK_GTEQ: LHS = LHS >= RHS; break;
        case TOK_EQEQ: LHS = LHS == RHS; break;
        case TOK_NOTEQ: LHS = LHS != RHS; break;
        case '&': LHS &= RHS; break;
        case '^': LHS ^= RHS; break;
        case '|': LHS |= RHS; break;
        case TOK_ANDAND: LHS = LHS && RHS; break;
        case TOK_OROR: LHS = LHS || RHS; break;
        default:
            printf("TODO: Calculate %d %s %d\n", LHS, TokenTypeText(Op), RHS);
            assert(0);
        }
    }
    return LHS;
}

int EvalExpr(void)
{
    return EvalExpr1(EvalUnary(), 15);
}

int CondActive(void)
{
    return !CondCount || CondState[CondCount-1] == COND_TRUE;
}

void NewCond(int Active)
{
    assert(CondCount < COND_MAX);
    char S = COND_FALSE;
    if (!CondActive())
        S = COND_DONE;
    else if (Active)
        S = COND_TRUE;
    if (Debug) printf("New cond: %d\n", S);
    CondState[CondCount++] = S;
}

void HandleInclude(void)
{
    GetToken();
    SkipWS();

    const char* Filename;
    if (CurTok.Type == '<') {
        TokenDataPos = NextTokenDataPos;
        Filename = &TokenData[TokenDataPos];
        while (CurrentChar != '>') {
            ReadToTokenData();
        }
        TokenData[TokenDataPos++] = 0;
        NextTokenDataPos = TokenDataPos;
    } else {
        NextTokenDataPos = TokenDataPos;
        Filename = CurTok.Text;
    }
    RawGetToken();
    CurFile->LastChar = CurrentChar;
    CurTok.Type = TOK_NEWLINE;
    CurFile->BufPtr = (char*)CurFileBuf;
    assert((unsigned)(CurFile->BufPtr - CurFile->Buf) < sizeof(CurFile->Buf));
    if (Debug) printf("Entering '%s' (%d)\n", Filename, NumFiles);
    CurFile = OpenFile(Filename, 0);
    CurFile->OldLine = Line;
    CurFileBuf = CurFile->Buf;
    Line = 1;
    ReadChar();
}

void HandleDirective(void)
{
    RawSkipWS();
    if (CurTok.Type == TOK_NEWLINE) {
        return;
    }
    ReplaceId();
    const int Directive = CurTok.Type;
    assert(Directive < TOK_USER_ID);

    if (Debug) printf("Processing directive %s (CondActive=%d)\n", IdText[Directive-TOK_ID], CondActive());

    if (Directive == TOK_DEFINE) {
        if (CondActive()) HandleDefine();
    } else if (Directive == TOK_INCLUDE) {
        if (CondActive()) HandleInclude();
    } else if (Directive == TOK_UNDEF) {
        RawGetToken();
        RawSkipWS();
        ReplaceId();
        if (CurTok.Type >= TOK_USER_ID) {
            // HACK: Just hide #undef'ed macros
            Macros[CurTok.Type - TOK_USER_ID].Hide = 2;
            FreeTokenList(Macros[CurTok.Type - TOK_USER_ID].Replacement);
        }
        GetToken();
    } else if (Directive == TOK_IFDEF || Directive == TOK_IFNDEF) {
        RawGetToken();
        RawSkipWS();
        ReplaceId();
        NewCond((int)(Directive == TOK_IFNDEF) ^ IsDefined(CurTok.Type));
        RawGetToken();
    } else if (Directive == TOK_ERROR) {
        if (CondActive()) {
            printf("#error in %s:%d!\n", CurFile->Filename, Line);
            exit(1);
        }
    } else if (Directive == TOK_PRAGMA || Directive == TOK_LINE) {
        // Ignore
    } else {
        GetToken();

        char* S = CondCount ? &CondState[CondCount-1] : 0;
        if (Directive == TOK_IF) {
            NewCond(EvalExpr());
        } else if (Directive == TOK_ELIF) {
            assert(S);
            if (*S == COND_TRUE)
                *S = COND_DONE;
            else if (*S == COND_FALSE && EvalExpr())
                *S = COND_TRUE;
        } else if (Directive == TOK_ELSE) {
            assert(S);
            if (*S == COND_TRUE)
                *S = COND_DONE;
            else if (*S == COND_FALSE)
                *S = COND_TRUE;
        } else if (Directive == TOK_ENDIF) {
            assert(CondCount);
            --CondCount;
        } else {
            printf("TODO: Handle directive '%s'\n", IdText[Directive-TOK_ID]);
        }
    }
    while (CurTok.Type != TOK_NEWLINE) {
        RawGetToken();
    }
}

void OutputStr(const char* Str)
{
    char* Buf = OutFile->BufPtr;
    char* End = OutFile->Buf + sizeof(OutFile->Buf);
    while ((*Buf = *Str++)) {
        if (++Buf != End)
            continue;
        write(OutFile->FD, OutFile->Buf, sizeof(OutFile->Buf));
        Buf = OutFile->Buf;
    }
    OutFile->BufPtr = Buf;
}

void PPMain(void)
{
    ReadChar();
    GetToken();

    enum {WS_NONE,WS_NORMAL,WS_NEWLINE};
    int WS = WS_NONE;
    int StartOfLine = 1;
    int Active = 1;
    for (;;) {
        switch (CurTok.Type) {
        case '#':
            if (!StartOfLine)
                break;
            RawGetToken();
            HandleDirective();
            if (!CurTok.Type)
                return;
            assert(CurTok.Type == TOK_NEWLINE);
            GetToken();
            StartOfLine = 1;
            Active = CondActive();
            continue;
        case 0:
            if (Debug) printf("Exiting '%s' (%d)\n", CurFile->Filename, NumFiles);
            Line = CurFile->OldLine;
            CloseFile();
            CurFile = &Files[NumFiles-1];
            if (NumFiles == 1) {
                return;
            }
            CurrentChar = CurFile->LastChar;
            CurFileBuf  = CurFile->BufPtr;
            StartOfLine = 1;
            GetToken();
            continue;
        }

        if (!Active) {
            GetToken();
            continue;
        }

        switch (CurTok.Type) {
        case TOK_WHITESPACE:
            WS |= WS_NORMAL;
            GetToken();
            continue;
        case TOK_NEWLINE:
            WS |= WS_NEWLINE;
            StartOfLine = 1;
            GetToken();
            continue;
        }
        if (WS) {
            if (WS & WS_NEWLINE) {
                OutputStr("\r\n");
            } else {
                OutputStr(" ");
            }
            WS = 0;
        }
        switch (CurTok.Type) {
        case TOK_STRLIT:
        case TOK_CHARLIT:
            {
                const char* S = CurTok.Type == TOK_STRLIT ? "\"" : "'";
                OutputStr(S);
                OutputStr(CurTok.Text);
                OutputStr(S);
            }
            break;
        default:
            OutputStr(CurTok.Text);
        }
        GetToken();
    }
}

void MakeOutputFilename(char* dest, const char* n)
{
    char* LastDot;
    LastDot = 0;
    while (*n) {
        if (*n == '.')
            LastDot = dest;
        *dest++ = *n++;
    }
    if (!LastDot) LastDot = dest;
    strcpy(LastDot, ".i");
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: scpp input-file [output-file]\n");
        return 1;
    }

    const char* OutName = argv[2];
    char Temp[FNAME_MAX];
    if (!OutName) {
        MakeOutputFilename(Temp, argv[1]);
        OutName = Temp;
    }
    OutFile = OpenFile(OutName, 1);

    // Open input after output to have nice stack of files
    CurFile = OpenFile(argv[1], 0);
    CurFileBuf = CurFile->Buf;

    memset(IdHash, -1, sizeof(IdHash));
    AddId("define");
    AddId("undef");
    AddId("if");
    AddId("ifdef");
    AddId("ifndef");
    AddId("elif");
    AddId("else");
    AddId("endif");
    AddId("include");
    AddId("line");
    AddId("error");
    AddId("pragma");
    AddId("defined");
    AddId("__FILE__");
    AddId("__LINE__");
    AddId("__SCC__");
    assert(TOK_ID+IdCount == TOK_USER_ID);

    int i;
    for (i = 0; i < TOKEN_LIST_MAX-1; ++i) {
        TokenListElems[i].Next = &TokenListElems[i+1];
    }
    NextFreeTokenList = &TokenListElems[0];

    PPMain();

    write(OutFile->FD, OutFile->Buf, OutFile->BufPtr - OutFile->Buf);
    CloseFile();
    assert(NumFiles == 0);

    return 0;
}
