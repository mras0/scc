#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
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
#include <unistd.h>
#endif

enum {
    LINEBUF_MAX = 100,
    TMPBUF_MAX = 32,
    INBUF_MAX = 1024,
    SCOPE_MAX = 10,
    VARDECL_MAX = 200,
    ID_MAX = 300,
    IDBUFFER_MAX = 4096,
};

enum {
    VT_VOID,
    VT_CHAR,
    VT_INT,

    VT_BASEMASK = 3,

    VT_LVAL = 4,
    VT_FUN  = 8,
    VT_ENUM = 16,

    VT_PTR1 = 512,
    VT_PTRMASK = 3584, // 512+1024+2048
};

enum {
    TOK_EOF,

    TOK_NUM,
    TOK_STRLIT,

    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_EQ,
    TOK_EQEQ,
    TOK_NOT,
    TOK_NOTEQ,
    TOK_LT,
    TOK_LTEQ,
    TOK_GT,
    TOK_GTEQ,
    TOK_PLUS,
    TOK_PLUSPLUS,
    TOK_MINUS,
    TOK_MINUSMINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_MOD,
    TOK_AND,
    TOK_XOR,
    TOK_OR,
    TOK_ANDAND,
    TOK_OROR,
    TOK_LSH,
    TOK_RSH,
    TOK_PLUSEQ,
    TOK_MINUSEQ,
    TOK_STAREQ,
    TOK_SLASHEQ,
    TOK_MODEQ,
    TOK_LSHEQ,
    TOK_RSHEQ,
    TOK_ANDEQ,
    TOK_XOREQ,
    TOK_OREQ,
    TOK_TILDE,
    TOK_DOT,
    TOK_ELLIPSIS,

    // NOTE: Must match order of registration in main
    //       TOK_BREAK must be first
    TOK_BREAK,
    TOK_CHAR,
    TOK_CONST,
    TOK_CONTINUE,
    TOK_ELSE,
    TOK_ENUM,
    TOK_FOR,
    TOK_IF,
    TOK_INT,
    TOK_RETURN,
    TOK_SIZEOF,
    TOK_VOID,
    TOK_WHILE,

    TOK_VA_LIST,
    TOK_VA_START,
    TOK_VA_END,
    TOK_VA_ARG,

    TOK_ID
};

int OutFile;
char* LineBuf;
char* TempBuf;

int InFile;
char* InBuf;
int InBufCnt;
int InBufSize;
char UngottenChar;

int Line;

int TokenType;
int TokenNumVal;

char* IdBuffer;
int IdBufferIndex;

int* IdOffset;
int IdCount;

int* VarDeclId;
int* VarDeclType;
int* VarDeclOffset;

int* Scopes; // VarDecl index
int ScopesCount;

int LocalOffset;
int CurrentType;
int LocalLabelCounter;
int ReturnLabel;
int BreakLabel;
int ContinueLabel;

int IsDigit(char ch) { return ch >= '0' && ch <= '9'; }
int IsAlpha(char ch) { return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'); }

void PutStr(const char* s)
{
    while (*s)
        putchar(*s++);
}

char* CopyStr(char* dst, const char* src)
{
    while (*src) *dst++ = *src++;
    return dst;
}

int StrLen(const char* s)
{
    int l;
    l = 0;
    while (*s++)
        ++l;
    return l;
}

void OutputStr(const char* buf)
{
    write(OutFile, buf, StrLen(buf));
}

char* VSPrintf(char* dest, const char* format, va_list vl)
{
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
            *dest++ = va_arg(vl, int);
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
            buf = TempBuf + TMPBUF_MAX;
            *--buf = 0;
            for (;;) {
                *--buf = '0' + n % 10;
                n/=10;
                if (!n) break;
            }
            if (s) *--buf = '-';
            else if (always) *--buf = '+';
            dest  = CopyStr(dest, buf);
        } else {
            PutStr("Invalid format string");
            exit(1);
        }
    }
    *dest = 0;
    return dest;
}

void Printf(const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    VSPrintf(LineBuf, format, vl);
    PutStr(LineBuf);
    va_end(vl);
}

int StrEqual(const char* a, const char* b)
{
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    return !*a && !*b;
}

void Fatal(const char* Msg)
{
    Printf("In line %d: %s\n", Line, Msg);
    exit(1);
}

void Check(int ok)
{
    if (!ok) {
        Fatal("Check failed");
    }
}

#define Check(expr) do { if (!(expr)) { Printf("In %s:%d: ", __FILE__, __LINE__); Fatal(#expr " failed"); } } while (0)

const char* IdText(int id)
{
    Check(id >= 0 && id < IdCount);
    return IdBuffer + IdOffset[id];
}

void RawEmit(const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    VSPrintf(LineBuf, format, vl);
    OutputStr(LineBuf);
    va_end(vl);
}

void Emit(const char* format, ...)
{
    va_list vl;
    char* dest;
    dest = LineBuf;
    *dest++ = '\t';
    va_start(vl, format);
    dest = VSPrintf(dest, format, vl);
    va_end(vl);
    *dest++ = '\n';
    *dest++ = 0;
    OutputStr(LineBuf);
}

void EmitLocalLabel(int l)
{
    RawEmit(".L%d:\n", l);
}

void EmitGlobalLabel(int id)
{
    RawEmit("_%s:\n", IdText(id));
}

char GetChar(void)
{
    if (UngottenChar) {
        char ch;
        ch = UngottenChar;
        UngottenChar = 0;
        return ch;
    }
    if (InBufCnt == InBufSize) {
        InBufCnt = 0;
        InBufSize = read(InFile, InBuf, INBUF_MAX);
        if (!InBufSize) {
            return 0;
        }
    }
    return InBuf[InBufCnt++];
}

void UnGetChar(char ch)
{
    UngottenChar = ch;
}

int TryGetChar(char ch)
{
    char ch2;
    if ((ch2 = GetChar()) != ch) {
        UnGetChar(ch2);
        return 0;
    }
    return 1;
}

void SkipLine(void)
{
    int ch;
    while ((ch = GetChar()) != '\n' && ch)
        ;
    ++Line;
}

void SkipWhitespace(void)
{
    char ch;
    while ((ch = GetChar()) <= ' ') {
        if (!ch) {
            return;
        }
        if (ch == '\n') {
            ++Line;
        }
    }
    UnGetChar(ch);
}

char Unescape(void)
{
    char ch;
    ch = GetChar();
    if (ch == 'n') {
        return '\n';
    } else if (ch == 'r') {
        return '\r';
    } else if (ch == 't') {
        return '\t';
    } else if (ch == '\'') {
        return '\'';
    } else if (ch == '"') {
        return '"';
    } else if (ch == '\\') {
        return '\\';
    } else {
        Printf("TODO: Escaped character literal \\%c\n", ch);
        Fatal("Unsupported character literal");
    }
}

void GetStringLiteral(void)
{
    int JL;
    int open;
    char ch;

    TokenNumVal = LocalLabelCounter++;
    JL = LocalLabelCounter++;
    Emit("JMP\t.L%d", JL);
    RawEmit(".L%d:\tDB ", TokenNumVal);

    open = 0;
    for (;;) {
        while ((ch = GetChar()) != '"') {
            if (ch == '\\') {
                ch = Unescape();
            }
            if (!ch) {
                Fatal("Unterminated string literal");
            }
            if (ch < ' ' || ch == '\'') {
                if (open) {
                    RawEmit("', ");
                    open = 0;
                }
                RawEmit("%d, ", ch);
            } else {
                if (!open) {
                    open = 1;
                    RawEmit("'");
                }
                RawEmit("%c", ch);
            }
        }
        SkipWhitespace();
        ch = GetChar();
        if (ch != '"') {
            UnGetChar(ch);
            break;
        }
    }
    if (open) RawEmit("', ");
    RawEmit("0\n");
    RawEmit(".L%d:\n", JL);
}

void GetToken(void)
{
    char ch;

    SkipWhitespace();
    ch = GetChar();

    if (!ch) {
        TokenType = TOK_EOF;
        return;
    }

    if (ch == '#') {
        SkipLine();
        GetToken();
        return;
    } else if (ch >= '0' && ch <= '9') {
        TokenNumVal = 0;
        for (;;) {
            TokenNumVal = TokenNumVal*10 + ch - '0';
            ch = GetChar();
            if (!IsDigit(ch)) {
                break;
            }
        }
        UnGetChar(ch);
        TokenType = TOK_NUM;
    } else if (IsAlpha(ch) || ch == '_') {
        int id;
        char* pc;
        char* start;
        start = pc = &IdBuffer[IdBufferIndex];
        *pc++ = ch;
        for (;;) {
            ch = GetChar();
            if (ch != '_' && !IsDigit(ch) && !IsAlpha(ch)) {
                break;
            }
            *pc++ = ch;
        }
        *pc++ = 0;
        UnGetChar(ch);
        TokenType = -1;
        for (id = 0; id < IdCount; ++id) {
            if (StrEqual(IdText(id), start)) {
                TokenType = id;
                break;
            }
        }
        if (TokenType < 0) {
            Check(IdCount < ID_MAX);
            TokenType = IdCount++;
            IdOffset[TokenType] = IdBufferIndex;
            IdBufferIndex += pc - start;
        }
        TokenType += TOK_BREAK;
    } else if (ch == '\'') {
        ch = GetChar();
        if (ch == '\\') {
            TokenNumVal = Unescape();
        } else {
            TokenNumVal = ch;
        }
        if (GetChar() != '\'') {
            Fatal("Invalid character literal");
        }
        TokenType = TOK_NUM;
    } else if (ch == '"') {
        GetStringLiteral();
        TokenType = TOK_STRLIT;
    } else {
        if (ch == '(') { TokenType = TOK_LPAREN;    }
        else if (ch == ')') { TokenType = TOK_RPAREN;    }
        else if (ch == '{') { TokenType = TOK_LBRACE;    }
        else if (ch == '}') { TokenType = TOK_RBRACE;    }
        else if (ch == '[') { TokenType = TOK_LBRACKET;  }
        else if (ch == ']') { TokenType = TOK_RBRACKET;  }
        else if (ch == ',') { TokenType = TOK_COMMA;     }
        else if (ch == ';') { TokenType = TOK_SEMICOLON; }
        else if (ch == '~') { TokenType = TOK_TILDE;     }
        else if (ch == '=') {
            TokenType = TOK_EQ;
            if (TryGetChar('=')) {
                TokenType = TOK_EQEQ;
            }
        } else if (ch == '!') {
            TokenType = TOK_NOT;
            if (TryGetChar('=')) {
                TokenType = TOK_NOTEQ;
            }
        } else if (ch == '<') {
            TokenType = TOK_LT;
            if (TryGetChar('<')) {
                TokenType = TOK_LSH;
                if (TryGetChar('=')) {
                    TokenType = TOK_LSHEQ;
                }
            } else if (TryGetChar('=')) {
                TokenType = TOK_LTEQ;
            }
        } else if (ch ==  '>') {
            TokenType = TOK_GT;
            if (TryGetChar('>')) {
                TokenType = TOK_RSH;
                if (TryGetChar('=')) {
                    TokenType = TOK_RSHEQ;
                }
            } else if (TryGetChar('=')) {
                TokenType = TOK_GTEQ;
            }
        } else if (ch == '&') {
            TokenType = TOK_AND;
            if (TryGetChar('&')) {
                TokenType = TOK_ANDAND;
            } else if (TryGetChar('=')) {
                TokenType = TOK_ANDEQ;
            }
        } else if (ch == '|') {
            TokenType = TOK_OR;
            if (TryGetChar('|')) {
                TokenType = TOK_OROR;
            } else if (TryGetChar('=')) {
                TokenType = TOK_OREQ;
            }
        } else if (ch == '^') {
            TokenType = TOK_XOR;
            if (TryGetChar('=')) {
                TokenType = TOK_XOREQ;
            }
        } else if (ch == '+') {
            TokenType = TOK_PLUS;
            if (TryGetChar('+')) {
                TokenType = TOK_PLUSPLUS;
            } else if (TryGetChar('=')) {
                TokenType = TOK_PLUSEQ;
            }
        } else if (ch == '-') {
            TokenType = TOK_MINUS;
            if (TryGetChar('-')) {
                TokenType = TOK_MINUSMINUS;
            } else if (TryGetChar('=')) {
                TokenType = TOK_MINUSEQ;
            }
        } else if (ch == '*') {
            TokenType = TOK_STAR;
            if (TryGetChar('=')) {
                TokenType = TOK_STAREQ;
            }
        } else if (ch == '/') {
            if (TryGetChar('/')) {
                SkipLine();
                GetToken();
                return;
            }
            TokenType = TOK_SLASH;
            if (TryGetChar('=')) {
                TokenType = TOK_SLASHEQ;
            }
        } else if (ch == '%') {
            TokenType = TOK_MOD;
            if (TryGetChar('=')) {
                TokenType = TOK_MODEQ;
            }
        } else if (ch == '.') {
            TokenType = TOK_DOT;
            if (TryGetChar('.')) {
                if (!TryGetChar('.')) {
                    Fatal("Invalid token ..");
                }
                TokenType = TOK_ELLIPSIS;
            }
        } else {
            Printf("Unknown token start '%c' (%d)\n", ch, ch);
            Fatal("Unknown token encountered");
        }
    }
}

int OperatorPrecedence(int tok)
{
    if (tok == TOK_STAR || tok == TOK_SLASH || tok == TOK_MOD) {
        return 3;
    } else if (tok == TOK_PLUS || tok == TOK_MINUS) {
        return 4;
    } else if (tok == TOK_LSH || tok == TOK_RSH) {
        return 5;
    } else if (tok == TOK_LT || tok == TOK_LTEQ || tok == TOK_GT || tok == TOK_GTEQ) {
        return 6;
    } else if (tok == TOK_EQEQ || tok == TOK_NOTEQ) {
        return 7;
    } else if (tok == TOK_AND) {
        return 8;
    } else if (tok == TOK_XOR) {
        return 9;
    } else if (tok == TOK_OR) {
        return 10;
    } else if (tok == TOK_ANDAND) {
        return 11;
    } else if (tok == TOK_OROR) {
        return 12;
    } else if (tok == TOK_EQ || tok == TOK_PLUSEQ || tok == TOK_MINUSEQ || tok == TOK_STAREQ || tok == TOK_SLASHEQ || tok == TOK_MODEQ || tok == TOK_LSHEQ || tok == TOK_RSHEQ || tok == TOK_ANDEQ || tok == TOK_XOREQ || tok == TOK_OREQ) {
        return 14;
    } else if (tok == TOK_COMMA) {
        return 15;
    } else {
        return 100;
    }
}

void Unexpected(void)
{
    Printf("token type %d\n", TokenType);
    Fatal("Unexpected token");
}

int Accept(int type)
{
    if (TokenType == type) {
        GetToken();
        return 1;
    }
    return 0;
}

void Expect(int type)
{
    if (!Accept(type)) {
        Printf("Token type %d expected got ", type);
        Unexpected();
    }
}

int ExpectId(void)
{
    int id;
    id = TokenType;
    if (id >= TOK_ID) {
        GetToken();
        return id - TOK_BREAK;
    }
    Printf("Expected identifier got ");
    Unexpected();
}

int IsTypeStart(void)
{
    return TokenType == TOK_CONST
        || TokenType == TOK_VOID
        || TokenType == TOK_CHAR
        || TokenType == TOK_INT
        || TokenType == TOK_VA_LIST;
}

void LvalToRval(void)
{
    if (CurrentType & VT_LVAL) {
        CurrentType &= ~VT_LVAL;
        Emit("MOV\tBX, AX");
        if (CurrentType == VT_CHAR) {
            Emit("MOV\tAL, [BX]");
            Emit("CBW");
            CurrentType = VT_INT;
        } else {
            Check(CurrentType == VT_INT || (CurrentType & VT_PTRMASK));
            Emit("MOV\tAX, [BX]");
        }
    }
}

void DoIncDec(int Op)
{
    int Amm;
    if (CurrentType == VT_CHAR || CurrentType == VT_INT || CurrentType == (VT_CHAR|VT_PTR1)) {
        Amm = 1;
    } else {
        Amm = 2;
    }
    if (Op == TOK_PLUSPLUS) {
        if (Amm == 1) {
            Emit("INC\tAX");
        } else {
            Emit("ADD\tAX, %d", Amm);
        }
    } else {
        Check(Op == TOK_MINUSMINUS);
        if (Amm == 1) {
            Emit("DEC\tAX");
        } else {
            Emit("SUB\tAX, %d", Amm);
        }
    }
}

void DoIncDecOp(int Op, int Post)
{
    Check(CurrentType & VT_LVAL);
    CurrentType &= ~VT_LVAL;
    Emit("MOV\tBX, AX");
    if (CurrentType == VT_CHAR) {
        Emit("MOV\tAL, [BX]");
        Emit("CBW");
        if (Post) Emit("PUSH\tAX");
        DoIncDec(Op);
        Emit("MOV\t[BX], AL");
        CurrentType = VT_INT;
    } else {
        Check(CurrentType == VT_INT || (CurrentType & VT_PTRMASK));
        Emit("MOV\tAX, [BX]");
        if (Post) Emit("PUSH\tAX");
        DoIncDec(Op);
        Emit("MOV\t[BX], AX");
    }
    if (Post) Emit("POP\tAX");
}

void ParseExpr(void);
void ParseCastExpression(void);
void ParseAssignmentExpression(void);
int ParseDeclSpecs(void);

int Lookup(int id)
{
    int vd;
    Check(ScopesCount);
    for (vd = Scopes[ScopesCount-1]; vd >= 0; vd--) {
        if (VarDeclId[vd] == id) {
            break;
        }
    }
    return vd;
}

void ParsePrimaryExpression(void)
{
    int id;
    int vd;
    if (TokenType == TOK_LPAREN) {
        GetToken();
        ParseExpr();
        Expect(TOK_RPAREN);
    } else if (TokenType == TOK_NUM) {
        Emit("MOV\tAX, %d", TokenNumVal);
        CurrentType = VT_INT;
        GetToken();
    } else if (TokenType == TOK_STRLIT) {
        Emit("MOV\tAX, .L%d", TokenNumVal);
        CurrentType = VT_PTR1 | VT_CHAR;
        GetToken();
    } else if (TokenType == TOK_VA_START || TokenType == TOK_VA_END || TokenType == TOK_VA_ARG) {
        // Handle var arg builtins
        int offset;
        int func;
        func = TokenType;
        GetToken();
        Expect(TOK_LPAREN);
        id = ExpectId();
        vd = Lookup(id);
        if (vd < 0 || VarDeclType[vd] != (VT_CHAR|VT_PTR1)) {
            Fatal("Invalid va_list");
        }
        offset = VarDeclOffset[vd];
        if (func == TOK_VA_START) {
            Expect(TOK_COMMA);
            id = ExpectId();
            vd = Lookup(id);
            if (vd < 0 || !VarDeclOffset[vd]) {
                Fatal("Invalid argument to va_start");
            }
            Emit("LEA\tAX, [BP%+d]", VarDeclOffset[vd]);
            Emit("MOV\t[BP%+d], AX", offset);
            CurrentType = VT_VOID;
        } else if (func == TOK_VA_END) {
            CurrentType = VT_VOID;
        } else if (func == TOK_VA_ARG) {
            Expect(TOK_COMMA);
            Emit("MOV\tAX, [BP%+d]", offset);
            Emit("ADD\tAX, 2");
            Emit("MOV\t[BP%+d], AX", offset);
            CurrentType = VT_LVAL | ParseDeclSpecs();
        }
        Expect(TOK_RPAREN);
    } else {
        id = ExpectId();
        vd = Lookup(id);
        if (vd < 0) {
            // Lookup failed. Assume function returning int.
            CurrentType = VT_FUN|VT_INT;
            Emit("MOV\tAX, _%s", IdText(id));
        } else {
            if (VarDeclType[vd] & VT_ENUM) {
                Emit("MOV\tAX, %d", VarDeclOffset[vd]);
                CurrentType = VT_INT;
                return;
            }
            CurrentType = VarDeclType[vd] | VT_LVAL;
            if (VarDeclOffset[vd]) {
                Emit("LEA\tAX, [BP%+d]\t; %s", VarDeclOffset[vd], IdText(VarDeclId[vd]));
            } else {
                Emit("MOV\tAX, _%s", IdText(id));
                if (CurrentType & VT_FUN) {
                    CurrentType &= ~VT_LVAL;
                }
            }
        }
    }
}

void ParsePostfixExpression(void)
{
    for (;;) {
        if (Accept(TOK_LPAREN)) {
            int RetType;
            int NumArgs;
            int MaxArgs;
            MaxArgs = 4; // TODO: Handle this better...

            // Function call
            if (!(CurrentType & VT_FUN)) {
                Fatal("Not a function");
            }
            Emit("PUSH\tSI");
            Emit("PUSH\tDI");
            Emit("MOV\tSI, AX");
            Emit("SUB\tSP, %d", MaxArgs*2);
            Emit("MOV\tDI, SP");
            RetType = CurrentType & ~VT_FUN;
            NumArgs = 0;
            while (TokenType != TOK_RPAREN) {
                Check(NumArgs < MaxArgs);
                ParseAssignmentExpression();
                LvalToRval();
                Check(CurrentType == VT_INT || (CurrentType & VT_PTRMASK));
                Emit("MOV\t[DI+%d], AX", NumArgs*2);
                ++NumArgs;

                if (!Accept(TOK_COMMA)) {
                    break;
                }
            }
            Expect(TOK_RPAREN);
            Emit("CALL\tSI");
            Emit("ADD\tSP, %d", MaxArgs*2);
            Emit("POP\tDI");
            Emit("POP\tSI");
            CurrentType = RetType;
            if (CurrentType == VT_CHAR) {
                CurrentType = VT_INT;
            }
        } else if (Accept(TOK_LBRACKET)) {
            int AType;
            LvalToRval();
            if (!(CurrentType & VT_PTRMASK)) {
                Fatal("Expected pointer");
            }
            AType = CurrentType - VT_PTR1;
            Emit("PUSH\tAX");
            ParseExpr();
            Expect(TOK_RBRACKET);
            LvalToRval();
            Check(CurrentType == VT_INT);
            if (AType != VT_CHAR) {
                Check(AType == VT_INT || (AType & VT_PTRMASK));
                Emit("ADD\tAX, AX");
            }
            Emit("POP\tCX");
            Emit("ADD\tAX, CX");
            CurrentType = AType | VT_LVAL;
        } else {
            int Op;
            Op = TokenType;
            if (Op != TOK_PLUSPLUS && Op != TOK_MINUSMINUS) {
                break;
            }
            GetToken();
            DoIncDecOp(Op, 1);
        }
    }
}

void ParseUnaryExpression(void)
{
    int Op;
    Op = TokenType;
    if (Op == TOK_PLUSPLUS || Op == TOK_MINUSMINUS) {
        GetToken();
        ParseUnaryExpression();
        DoIncDecOp(Op, 0);
    } else if (Op == TOK_AND || Op == TOK_STAR || Op == TOK_PLUS || Op == TOK_MINUS || Op == TOK_TILDE || Op == TOK_NOT) {
        GetToken();
        ParseCastExpression();
        if (Op != TOK_AND) {
            LvalToRval();
        }
        if (Op == TOK_AND) {
            if (!(CurrentType & VT_LVAL)) {
                Fatal("Lvalue required for address-of operator");
            }
            CurrentType = (CurrentType&~VT_LVAL) + VT_PTR1;
        } else if (Op == TOK_STAR) {
            if (!(CurrentType & VT_PTR1)) {
                Fatal("Pointer required for dereference");
            }
            CurrentType = (CurrentType-VT_PTR1) | VT_LVAL;
        } else if (Op == TOK_PLUS) {
            Check(CurrentType == VT_INT);
        } else if (Op == TOK_MINUS) {
            Check(CurrentType == VT_INT);
            Emit("NEG\tAX");
        } else if (Op == TOK_TILDE) {
            Check(CurrentType == VT_INT);
            Emit("NOT\tAX");
        } else if (Op == TOK_NOT) {
            int Lab;
            Lab = LocalLabelCounter++;
            Check(CurrentType == VT_INT || (CurrentType & VT_PTRMASK));
            Emit("AND\tAX, AX");
            Emit("MOV\tAX, 0");
            Emit("JNZ\t.L%d", Lab);
            Emit("INC\tAL");
            EmitLocalLabel(Lab);
            CurrentType = VT_INT;
        } else {
            Unexpected();
        }
    } else if (Op == TOK_SIZEOF) {
        int Size;
        GetToken();
        Expect(TOK_LPAREN);
        if (Accept(TOK_CHAR)) {
            Size = 1;
        } else if (Accept(TOK_INT)) {
            Size = 2;
        } else {
            Fatal("sizeof not implemented for this type");
        }
        while (Accept(TOK_STAR)) {
            Size = 2;
        }
        Expect(TOK_RPAREN);
        Emit("MOV\tAX, %d", Size);
        CurrentType = VT_INT;
    } else {
        ParsePrimaryExpression();
        ParsePostfixExpression();
    }
}

void ParseCastExpression(void)
{
    ParseUnaryExpression();
}

const char* RelOp(int Op)
{
    if (Op == TOK_LT)         return "L";
    else if (Op == TOK_LTEQ)  return "NG";
    else if (Op == TOK_GT)    return "G";
    else if (Op == TOK_GTEQ)  return "NL";
    else if (Op == TOK_EQEQ)  return "E";
    else if (Op == TOK_NOTEQ) return "NE";
    Fatal("Not implemented");
}

// Emit: AX <- AX 'OP' CX, Must preserve BX.
void DoBinOp(int Op)
{
    if (Op == TOK_LT || Op == TOK_LTEQ || Op == TOK_GT || Op == TOK_GTEQ || Op == TOK_EQEQ || Op == TOK_NOTEQ) {
        int Lab;
        Lab = LocalLabelCounter++;
        Emit("CMP\tAX, CX");
        Emit("MOV\tAX, 1");
        RelOp(Op);
        Emit("J%s\t.L%d", RelOp(Op), Lab);
        Emit("DEC\tAL");
        EmitLocalLabel(Lab);
        return;
    }

    if (Op == TOK_PLUS || Op == TOK_PLUSEQ) {
        Emit("ADD\tAX, CX");
    } else if (Op == TOK_MINUS || Op == TOK_MINUSEQ) {
        Emit("SUB\tAX, CX");
    } else if (Op == TOK_STAR || Op == TOK_STAREQ) {
        Emit("IMUL\tCX");
    } else if (Op == TOK_SLASH || Op == TOK_SLASHEQ || Op == TOK_MOD || Op == TOK_MODEQ) {
        Emit("CWD");
        Emit("IDIV\tCX");
        if (Op == TOK_MOD || Op == TOK_MODEQ) {
            Emit("MOV\tAX, DX");
        }
    } else if (Op == TOK_AND || Op == TOK_ANDEQ) {
        Emit("AND\tAX, CX");
    } else if (Op == TOK_XOR || Op == TOK_XOREQ) {
        Emit("XOR\tAX, CX");
    } else if (Op == TOK_OR || Op == TOK_OREQ) {
        Emit("OR\tAX, CX");
    } else if (Op == TOK_LSH || Op == TOK_LSHEQ) {
        Emit("SHL\tAX, CL");
    } else if (Op == TOK_RSH || Op == TOK_RSHEQ) {
        Emit("SAR\tAX, CL");
    }
}

void ParseExpr1(int OuterPrecedence)
{
    int LEnd;
    int LTemp;
    int Op;
    int Prec;
    int IsAssign;
    int LhsType;
    for (;;) {
        Op   = TokenType;
        Prec = OperatorPrecedence(Op);
        if (Prec > OuterPrecedence) {
            break;
        }
        GetToken();

        IsAssign = Prec == OperatorPrecedence(TOK_EQ);

        if (IsAssign) {
            if (!(CurrentType & VT_LVAL)) {
                Fatal("L-value required");
            }
        } else {
            LvalToRval();
        }
        LhsType = CurrentType;

        if (Op == TOK_ANDAND || Op == TOK_OROR) {
            const char* JText;
            LTemp = LocalLabelCounter++;
            LEnd = LocalLabelCounter++;
            Check(CurrentType == VT_INT);
            Emit("AND\tAX, AX");
            if (Op == TOK_ANDAND)
                JText = "JNZ";
            else
                JText = "JZ";
            Emit("%s\t.L%d", JText, LTemp);
            Emit("JMP\t.L%d", LEnd);
            EmitLocalLabel(LTemp);
        } else {
            LEnd = -1;
            Emit("PUSH\tAX");
        }

        // TODO: Question, ?:
        ParseCastExpression(); // RHS
        for (;;) {
            int LookAheadOp;
            int LookAheadPrecedence;
            LookAheadOp         = TokenType;
            LookAheadPrecedence = OperatorPrecedence(LookAheadOp);
            if (LookAheadPrecedence > Prec) // || (LookAheadPrecedence == Prec && !IsRightAssociative(LookAheadOp))
                break;
            ParseExpr1(LookAheadPrecedence);
        }
        LvalToRval();
        if (LEnd >= 0) {
            EmitLocalLabel(LEnd);
        } else if (IsAssign) {
            Check(LhsType & VT_LVAL);
            LhsType &= ~VT_LVAL;
            Emit("POP\tBX");
            if (Op != TOK_EQ) {
                Check(LhsType == VT_INT || (LhsType & VT_PTRMASK)); // For pointer types only += and -= should be allowed
                Check(CurrentType == VT_INT);
                Emit("MOV\tCX, AX");
                Emit("MOV\tAX, [BX]");
                DoBinOp(Op);
            }
            if (LhsType == VT_CHAR) {
                Check(CurrentType == VT_INT);
                Emit("MOV\t[BX], AL");
            } else {
                Check(LhsType == VT_INT || (LhsType & VT_PTRMASK));
                Check(CurrentType == VT_INT || (CurrentType & VT_PTRMASK));
                Emit("MOV\t[BX], AX");
            }
        } else {
            Check(LhsType == VT_INT || (LhsType & VT_PTRMASK));
            Check(CurrentType == VT_INT || (Op == TOK_MINUS && (CurrentType & VT_PTRMASK)));
            if (Op == TOK_PLUS && (LhsType & VT_PTRMASK) && LhsType != (VT_CHAR|VT_PTR1)) {
                Emit("ADD\tAX, AX");
            }
            Emit("POP\tCX");
            Emit("XCHG\tAX, CX");
            DoBinOp(Op);
            if (Op == TOK_MINUS && (LhsType & VT_PTRMASK)) {
                if (LhsType != (VT_CHAR|VT_PTR1))
                    Emit("SAR\tAX, 1");
                CurrentType = VT_INT;
            }
        }
    }
}

void ParseExpr0(int OuterPrecedence)
{
    ParseCastExpression();
    ParseExpr1(OuterPrecedence);
}

void ParseExpr(void)
{
    ParseExpr0(OperatorPrecedence(TOK_COMMA));
}

void ParseAssignmentExpression(void)
{
    ParseExpr0(OperatorPrecedence(TOK_EQ));
}

int ParseDeclSpecs(void)
{
    Check(IsTypeStart());
    if (TokenType == TOK_CONST) {
        // Ignore
        GetToken();
        Check(IsTypeStart());
    }
    int t;
    if (TokenType == TOK_VOID) {
        t = VT_VOID;
    } else if (TokenType == TOK_CHAR) {
        t = VT_CHAR;
    } else if (TokenType == TOK_INT) {
        t = VT_INT;
    } else if (TokenType == TOK_VA_LIST) {
        GetToken();
        return VT_CHAR | VT_PTR1;
    } else {
        Unexpected();
    }
    GetToken();
    while (TokenType == TOK_STAR) {
        t += VT_PTR1;
        GetToken();
    }
    return t;
}

int AddVarDecl(int Type, int Id)
{
    int vd;
    Check(ScopesCount);
    Check(Scopes[ScopesCount-1] < VARDECL_MAX-1);
    vd = ++Scopes[ScopesCount-1];
    VarDeclType[vd]   = Type;
    VarDeclOffset[vd] = 0;
    VarDeclId[vd]     = Id;
    return vd;
}

int ParseDecl(void)
{
    int Type;
    int Id;
    Type = ParseDeclSpecs();
    Id   = ExpectId();
    return AddVarDecl(Type, Id);
}

void PushScope(void)
{
    int End;
    Check(ScopesCount < SCOPE_MAX);
    if (ScopesCount)
        End = Scopes[ScopesCount-1];
    else
        End = -1;
    Scopes[ScopesCount++] = End;
}

void PopScope(void)
{
    Check(ScopesCount);
    --ScopesCount;
}

void ParseCompoundStatement(void);

void ParseStatement(void)
{
    int OldBreak;
    int OldContinue;
    OldBreak    = BreakLabel;
    OldContinue = ContinueLabel;

    if (Accept(TOK_SEMICOLON)) {
    } else if (TokenType == TOK_LBRACE) {
        ParseCompoundStatement();
    } else if (IsTypeStart()) {
        int vd;
        vd = ParseDecl();
        LocalOffset -= 2;
        VarDeclOffset[vd] = LocalOffset;
        Emit("SUB\tSP, 2\t; [BP%+d] = %s", VarDeclOffset[vd], IdText(VarDeclId[vd]));
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_FOR)) {
        int CondLabel;
        int BodyLabel;
        int EndLabel;
        int IterLabel;

        CondLabel = LocalLabelCounter++;
        BodyLabel = LocalLabelCounter++;
        EndLabel  = LocalLabelCounter++;
        IterLabel = CondLabel;

        Expect(TOK_LPAREN);

        // Init
        if (TokenType != TOK_SEMICOLON) {
            ParseExpr();
        }
        Expect(TOK_SEMICOLON);

        // Cond
        EmitLocalLabel(CondLabel);
        if (TokenType != TOK_SEMICOLON) {
            ParseExpr();
            LvalToRval();
            Check(CurrentType == VT_INT);
            Emit("AND\tAX, AX");
            Emit("JNZ\t.L%d", BodyLabel); // TODO: Need far jump?
            Emit("JMP\t.L%d", EndLabel);
        } else {
            Emit("JMP\t.L%d", BodyLabel);
        }
        Expect(TOK_SEMICOLON);

        // Iter
        if (TokenType != TOK_RPAREN) {
            IterLabel  = LocalLabelCounter++;
            EmitLocalLabel(IterLabel);
            ParseExpr();
            Emit("JMP\t.L%d", CondLabel);
        }
        Expect(TOK_RPAREN);

        EmitLocalLabel(BodyLabel);
        BreakLabel    = EndLabel;
        ContinueLabel = IterLabel;
        ParseStatement();
        Emit("JMP\t.L%d", IterLabel);
        EmitLocalLabel(EndLabel);
    } else if (Accept(TOK_IF)) {
        int IfLabel;
        int ElseLabel;
        int EndLabel;

        IfLabel    = LocalLabelCounter++;
        ElseLabel  = LocalLabelCounter++;
        EndLabel   = LocalLabelCounter++;
        Accept(TOK_LPAREN);
        ParseExpr();
        LvalToRval();
        Check(CurrentType == VT_INT);
        Emit("AND\tAX, AX");
        Emit("JNZ\t.L%d", IfLabel);
        Emit("JMP\t.L%d", ElseLabel);
        Accept(TOK_RPAREN);
        EmitLocalLabel(IfLabel);
        ParseStatement();
        Emit("JMP\t.L%d", EndLabel);
        EmitLocalLabel(ElseLabel);
        if (Accept(TOK_ELSE)) {
            ParseStatement();
        }
        EmitLocalLabel(EndLabel);
    } else if (Accept(TOK_RETURN)) {
        if (TokenType != TOK_SEMICOLON) {
            ParseExpr();
            LvalToRval();
        }
        Emit("JMP\t.L%d", ReturnLabel);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_WHILE)) {
        int StartLabel;
        int BodyLabel;
        int EndLabel;
        StartLabel = LocalLabelCounter++;
        BodyLabel  = LocalLabelCounter++;
        EndLabel   = LocalLabelCounter++;
        EmitLocalLabel(StartLabel);
        Expect(TOK_LPAREN);
        ParseExpr();
        LvalToRval();
        Check(CurrentType == VT_INT);
        Emit("AND\tAX, AX");
        Emit("JNZ\t.L%d", BodyLabel);
        Emit("JMP\t.L%d", EndLabel);
        Expect(TOK_RPAREN);
        EmitLocalLabel(BodyLabel);
        BreakLabel = EndLabel;
        ContinueLabel = StartLabel;
        ParseStatement();
        Emit("JMP\t.L%d", StartLabel);
        EmitLocalLabel(EndLabel);
    } else if (Accept(TOK_BREAK)) {
        Check(BreakLabel >= 0);
        Emit("JMP\t.L%d", BreakLabel);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_CONTINUE)) {
        Check(ContinueLabel >= 0);
        Emit("JMP\t.L%d", ContinueLabel);
        Expect(TOK_SEMICOLON);
    } else {
        // Expression statement
        ParseExpr();
        Expect(TOK_SEMICOLON);
    }

    BreakLabel    = OldBreak;
    ContinueLabel = OldContinue;
}

void ParseCompoundStatement(void)
{
    int InitialOffset;
    InitialOffset = LocalOffset;
    PushScope();
    Expect(TOK_LBRACE);
    while (!Accept(TOK_RBRACE)) {
        ParseStatement();
    }
    PopScope();
    if (InitialOffset != LocalOffset) {
        Emit("ADD\tSP, %d", InitialOffset - LocalOffset);
        LocalOffset = InitialOffset;
    }
}

// Arg 2              BP + 6
// Arg 1              BP + 4
// Return Address     BP + 2
// Old BP             BP      <-- BP
// Local 1            BP - 2
// Local 2            BP - 4  <-- SP

void ParseExternalDefition(void)
{
    if (Accept(TOK_ENUM)) {
        int EnumVal;
        int id;
        int vd;
        Expect(TOK_LBRACE);
        EnumVal = 0;
        while (TokenType != TOK_RBRACE) {
            id = ExpectId();
            if (Accept(TOK_EQ)) {
                EnumVal = TokenNumVal;
                Expect(TOK_NUM);
            }
            vd = AddVarDecl(VT_INT|VT_ENUM, id);
            VarDeclOffset[vd] = EnumVal;
            if (!Accept(TOK_COMMA)) {
                break;
            }
            ++EnumVal;
        }
        Expect(TOK_RBRACE);
        Expect(TOK_SEMICOLON);
        return;
    }

    int fd;
    fd = ParseDecl();

    if (Accept(TOK_SEMICOLON)) {
        EmitGlobalLabel(VarDeclId[fd]);
        Emit("DW\t0");
        return;
    }

    // Note: We don't actually care that we might end up
    // with multiple VarDecl's for functions with prototypes.
    Expect(TOK_LPAREN);
    VarDeclType[fd] |= VT_FUN;
    PushScope();
    int ArgOffset;
    int vd;
    ArgOffset = 4;
    while (TokenType != TOK_RPAREN) {
        if (Accept(TOK_VOID) || Accept(TOK_ELLIPSIS)) {
            break;
        }
        vd = ParseDecl();
        VarDeclOffset[vd] = ArgOffset;
        ArgOffset += 2;
        if (!Accept(TOK_COMMA)) {
            break;
        }
    }
    Expect(TOK_RPAREN);
    if (!Accept(TOK_SEMICOLON)) {
        LocalOffset = 0;
        LocalLabelCounter = 0;
        ReturnLabel = LocalLabelCounter++;
        BreakLabel = ContinueLabel = -1;
        EmitGlobalLabel(VarDeclId[fd]);
        Emit("PUSH\tBP");
        Emit("MOV\tBP, SP");
        ParseCompoundStatement();
        EmitLocalLabel(ReturnLabel);
        Emit("MOV\tSP, BP");
        Emit("POP\tBP");
        Emit("RET");
    }
    PopScope();
}

#define CREATE_FLAGS O_WRONLY | O_CREAT | O_TRUNC

#if 0
// Only used when self-compiling
// Small "standard library"

int CREATE_FLAGS; // Ugly hack

char* HeapStart; // Initialized in "Start"

char* malloc(int Size)
{
    char* ret;
    ret = HeapStart;
    HeapStart += Size;
    Check(HeapStart < 32767); // FIXME
    return ret;
}

void exit(int retval)
{
    retval = (retval & 255) | 19456; // 19456 = 0x4c00
    DosCall(&retval, 0, 0, 0);
}

void putchar(int ch)
{
    int ax;
    ax = 512; // 512 = 0x0200
    DosCall(&ax, 0, 0, ch);
}

int open(const char* filename, int flags, ...)
{
    int ax;
    if (flags)
        ax = 15360; // 0x3c00 create or truncate file
    else
        ax = 15616; // 0x3d00 open existing file

    if (DosCall(&ax, 0, 0, filename))
        return -1;
    return ax;
}

void close(int fd)
{
    int ax;
    ax = 15872; // 0x3b00
    DosCall(&ax, fd, 0, 0);
}

int read(int fd, char* buf, int count)
{
    int ax;
    ax = 16128; // 0x3f00
    if (DosCall(&ax, fd, count, buf))
        return 0;
    return ax;
}

int write(int fd, const char* buf, int count)
{
    int ax;
    ax = 16384; // 0x4000
    if (DosCall(&ax, fd, count, buf))
        return 0;
    return ax;
}

void CallMain(int Len, char* CmdLine)
{
    char **Args;
    int NumArgs;
    CmdLine[Len] = 0;

    Args = malloc(sizeof(char*)*10);
    Args[0] = "scc";
    NumArgs = 1;

    while (*CmdLine) {
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
    CREATE_FLAGS = 1;
    exit(main(NumArgs, Args));
}
#endif

void StrCpy(char* d, const char* s) {
    while (*d++ = *s++)
        ;
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
    StrCpy(LastDot, ".asm");
}

void AddBuiltins(const char* s)
{
    char ch;
    IdOffset[0] = 0;
    for (;;) {
        ch = *s++;
        if (ch <= ' ') {
            IdBuffer[IdBufferIndex++] = 0;
            IdOffset[++IdCount] = IdBufferIndex;
            if (!ch) break;
        } else {
            IdBuffer[IdBufferIndex++] = ch;
        }
    }
    Check(IdCount+TOK_BREAK-1 == TOK_VA_ARG);
}

int main(int argc, char** argv)
{
    LineBuf       = malloc(LINEBUF_MAX);
    TempBuf       = malloc(TMPBUF_MAX);
    InBuf         = malloc(INBUF_MAX);
    IdBuffer      = malloc(IDBUFFER_MAX);
    IdOffset      = malloc(sizeof(int)*ID_MAX);
    VarDeclId     = malloc(sizeof(int)*VARDECL_MAX);
    VarDeclType   = malloc(sizeof(int)*VARDECL_MAX);
    VarDeclOffset = malloc(sizeof(int)*VARDECL_MAX);
    Scopes        = malloc(sizeof(int)*SCOPE_MAX);

    if (argc < 2) {
        Printf("Usage: %s input-file\n", argv[0]);
        return 1;
    }

    InFile = open(argv[1], 0); // O_RDONLY
    if (InFile < 0) {
        Fatal("Error opening input file");
    }

    MakeOutputFilename(TempBuf, argv[1]);
    OutFile = open(TempBuf, CREATE_FLAGS, 384); // 384=0600
    if (OutFile < 0) {
        Fatal("Error creating output file");
    }

    AddBuiltins("break char const continue else enum for if int return sizeof void while va_list va_start va_end va_arg");

    // Prelude
    OutputStr(
    "\tcpu 8086\n"
    "\torg 0x100\n"
    "Start:\n"
    "\tXOR\tBP, BP\n"
    "\tMOV\tWORD [_HeapStart], ProgramEnd\n"
    "\tMOV\tAX, 0x81\n"
    "\tPUSH\tAX\n"
    "\tMOV\tAL, [0x80]\n"
    "\tPUSH\tAX\n"
    "\tPUSH\tAX\n"
    "\tJMP\t_CallMain\n"
    "\n"
    "_DosCall:\n"
    "\tPUSH\tBP\n"
    "\tMOV\tBP, SP\n"
    "\tMOV\tBX, [BP+4]\n"
    "\tMOV\tAX, [BX]\n"
    "\tMOV\tBX, [BP+6]\n"
    "\tMOV\tCX, [BP+8]\n"
    "\tMOV\tDX, [BP+10]\n"
    "\tINT\t0x21\n"
    "\tMOV\tBX, [BP+4]\n"
    "\tMOV\t[BX], AX\n"
    "\tMOV\tAX, 0\n"
    "\tSBB\tAX, AX\n"
    "\tMOV\tSP, BP\n"
    "\tPOP\tBP\n"
    "\tRET\n"
    "\n"
    );

    PushScope();
    Line = 1;
    GetToken();
    while (TokenType != TOK_EOF) {
        ParseExternalDefition();
    }
    PopScope();

    // Postlude
    OutputStr("\nProgramEnd:\n");

    close(InFile);
    close(OutFile);

    return 0;
}
