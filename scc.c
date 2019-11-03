#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#define open _open
#define close _close
#define read _read
#define write _write
#endif

enum {
    LINEBUF_MAX = 100,
    TMPBUF_MAX = 10,
    INBUF_MAX = 1024,
    TOKEN_MAX = 64,
    SCOPE_MAX = 10,
    VARDECL_MAX = 200,
    ID_MAX = 300,
    IDBUFFER_MAX = 4096,
};

enum {
    VT_VOID,
    VT_CHAR,
    VT_INT,

    VT_BASEMASK = 15,

    VT_LVAL = 256,
    VT_PTR  = 512,
    VT_FUN  = 1024,
    VT_ENUM = 2048
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

char* TokenText;
int TokenLen;
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
    while (ch = *format++) {
        if (ch != '%') {
            *dest++ = ch;
            continue;
        }
        ch = *format++;
        if (ch == 's') {
            dest = CopyStr(dest, va_arg(vl, char*));
        } else if(ch == 'c') {
            *dest++ = va_arg(vl, char);
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

#define Check(expr) do { if (!(expr)) Fatal(#expr " failed"); } while (0)

const char* IdText(int id)
{
    Check(id >= 0 && id < IdCount);
    return IdBuffer + IdOffset[id];
}

int AddId(const char* Name)
{
    int cnt;
    int ofs;
    Check(IdCount < ID_MAX);
    cnt = StrLen(Name) + 1;
    ofs = IdBufferIndex;
    IdBufferIndex += cnt;
    Check(IdBufferIndex <= IDBUFFER_MAX);
    memcpy(IdBuffer + ofs, Name, cnt);
    IdOffset[IdCount++] = ofs;
    return IdCount - 1;
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
    TokenText[TokenLen++] = ch2;
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
    TokenLen = 0;

    if (!ch) {
        TokenType = TOK_EOF;
        return;
    }

    TokenText[TokenLen++] = ch;

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
            TokenText[TokenLen++] = ch;
        }
        UnGetChar(ch);
        TokenType = TOK_NUM;
    } else if (IsAlpha(ch) || ch == '_') {
        int id;
        for (;;) {
            ch = GetChar();
            if (ch != '_' && !IsDigit(ch) && !IsAlpha(ch)) {
                break;
            }
            TokenText[TokenLen++] = ch;
        }
        TokenText[TokenLen] = 0;
        UnGetChar(ch);
        TokenType = -1;
        for (id = 0; id < IdCount; ++id) {
            if (StrEqual(IdText(id), TokenText)) {
                TokenType = id;
                break;
            }
        }
        if (TokenType < 0) {
            TokenType = AddId(TokenText);
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
    TokenText[TokenLen] = 0;
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
    Printf("%d \"%s\"\n", TokenType, TokenText);
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
            Check(CurrentType == VT_INT || (CurrentType & VT_PTR));
            Emit("MOV\tAX, [BX]");
        }
    }
}

void DoIncDec(int Op)
{
    int Amm;
    if (CurrentType == VT_CHAR || CurrentType == VT_INT || CurrentType == (VT_CHAR|VT_PTR)) {
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
        Check(CurrentType == VT_INT || (CurrentType & VT_PTR));
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
        CurrentType = VT_PTR | VT_CHAR;
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
        if (vd < 0 || VarDeclType[vd] != (VT_CHAR|VT_PTR)) {
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
                Check(CurrentType == VT_INT || (CurrentType & VT_PTR));
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
            if (!(CurrentType & VT_PTR)) {
                Fatal("Expected pointer");
            }
            AType = CurrentType & ~VT_PTR;
            Emit("PUSH\tAX");
            ParseExpr();
            Expect(TOK_RBRACKET);
            LvalToRval();
            Check(CurrentType == VT_INT);
            if (AType != VT_CHAR) {
                Check(AType == VT_INT);
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
            CurrentType = (CurrentType&~VT_LVAL) | VT_PTR;
        } else if (Op == TOK_STAR) {
            if (!(CurrentType & VT_PTR)) {
                Fatal("Pointer required for dereference");
            }
            CurrentType = (CurrentType&~VT_PTR) | VT_LVAL;
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
            Check(CurrentType == VT_INT);
            Emit("AND\tAX, AX");
            Emit("MOV\tAX, 0");
            Emit("JNZ\t.L%d", Lab);
            Emit("INC\tAL");
            EmitLocalLabel(Lab);
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
        Emit("XOR\tDX, DX");
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
                Check(LhsType == VT_INT);
                Check(CurrentType == VT_INT);
                Emit("MOV\tCX, AX");
                Emit("MOV\tAX, [BX]");
                DoBinOp(Op);
            }
            if (LhsType == VT_CHAR) {
                Check(CurrentType == VT_INT);
                Emit("MOV\t[BX], AL");
            } else {
                Check(LhsType == VT_INT || (LhsType & VT_PTR));
                Check(CurrentType == VT_INT || (CurrentType & VT_PTR));
                Emit("MOV\t[BX], AX");
            }
        } else {
            Check(LhsType == VT_INT || (LhsType & VT_PTR));
            Check(CurrentType == VT_INT);
            if (LhsType == (VT_INT|VT_PTR)) {
                Emit("ADD\tAX, AX");
            }
            Emit("POP\tCX");
            Emit("XCHG\tAX, CX");
            DoBinOp(Op);
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
        return VT_CHAR | VT_PTR;
    } else {
        Unexpected();
    }
    GetToken();
    if (TokenType == TOK_STAR) {
        t |= VT_PTR;
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
        Emit("; Start of if");
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
        Emit("ADD\tSP, %d\t; LocalOffset=%d", InitialOffset - LocalOffset, InitialOffset);
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
#define PERM_FLAGS S_IREAD | S_IWRITE

int main(void)
{
    LineBuf       = malloc(LINEBUF_MAX);
    TempBuf       = malloc(TMPBUF_MAX);
    InBuf         = malloc(INBUF_MAX);
    TokenText     = malloc(TOKEN_MAX);
    IdBuffer      = malloc(IDBUFFER_MAX);
    IdOffset      = malloc(sizeof(int)*ID_MAX);
    VarDeclId     = malloc(sizeof(int)*VARDECL_MAX);
    VarDeclType   = malloc(sizeof(int)*VARDECL_MAX);
    VarDeclOffset = malloc(sizeof(int)*VARDECL_MAX);
    Scopes        = malloc(sizeof(int)*SCOPE_MAX);


#if 0
    // Only active when self-compiling
    int CREATE_FLAGS;
    int PERM_FLAGS;
    CREATE_FLAGS=1;
    PERM_FLAGS=0;
#endif

    InFile = open("scc.c", 0); // O_RDONLY
    if (InFile < 0) {
        Fatal("Error opening input file");
    }

    OutFile = open("scc.asm", CREATE_FLAGS, PERM_FLAGS);
    if (OutFile < 0) {
        Fatal("Error opening output file");
    }

    AddId("break");
    AddId("char");
    AddId("const");
    AddId("continue");
    AddId("else");
    AddId("enum");
    AddId("for");
    AddId("if");
    AddId("int");
    AddId("return");
    AddId("sizeof");
    AddId("void");
    AddId("while");
    AddId("va_list");
    AddId("va_start");
    AddId("va_end");
    AddId("va_arg");
    Check(IdCount+TOK_BREAK-1 == TOK_VA_ARG);

    // Prelude
    OutputStr(
    "\tcpu 8086\n"
    "\torg 0x100\n"
    "Start:\n"
    "\tXOR\tBP, BP\n"
    "\tCALL\t_main\n"
    "\tPUSH\tAX\n"
    "\tCALL\t_exit\n"
    "\n"
    "_exit:\n"
    "\tPUSH\tBP\n"
    "\tMOV\tBP, SP\n"
    "\tMOV\tAH, 0x4C\n"
    "\tMOV\tAL, [BP+4]\n"
    "\tINT\t0x21\n"
    "\n"
    "_putchar:\n"
    "\tPUSH\tBP\n"
    "\tMOV\tBP, SP\n"
    "\tMOV\tDL, [BP+4]\n"
    "\tMOV\tAH, 0x02\n"
    "\tINT\t0x21\n"
    "\tMOV\tSP, BP\n"
    "\tPOP\tBP\n"
    "\tRET\n"
    "\n"
    "_malloc:\n"
    "\tPUSH\tBP\n"
    "\tMOV\tBP, SP\n"
    "\tMOV\tAX, [HeapPtr]\n"
    "\tMOV\tCX, [BP+4]\n"
    "\tINC\tCX\n"
    "\tAND\tCL, 0xFE\n"
    "\tADD\tCX, AX\n"
    "\tMOV\t[HeapPtr], CX\n"
    "\tMOV\tSP, BP\n"
    "\tPOP\tBP\n"
    "\tRET\n"
    "\n"
    "_open:\n"
    "\tPUSH\tBP\n"
    "\tMOV\tBP, SP\n"
    "\tMOV\tDX, [BP+4]\n"
    "\tMOV\tAX, 0x3D00\n"
    "\tCMP\tWORD [BP+6], 0\n"
    "\tJZ\t.DoOpen\n"
    "\tDEC\tAH\n"
    ".DoOpen:\n"
    "\tINT\t0x21\n"
    "\tJNC\t.Ok\n"
    "\tMOV\tAX, -1\n"
    "\t.Ok:\n"
    "\tMOV\tSP, BP\n"
    "\tPOP\tBP\n"
    "\tRET\n"
    "\n"
    "_close:\n"
    "\tPUSH\tBP\n"
    "\tMOV\tBP, SP\n"
    "\tMOV\tBX, [BP+4]\n"
    "\tMOV\tAH, 0x3E\n"
    "\tINT\t0x21\n"
    "\tMOV\tSP, BP\n"
    "\tPOP\tBP\n"
    "\tRET\n"
    "\n"
    "_read:\n"
    "\tPUSH\tBP\n"
    "\tMOV\tBP, SP\n"
    "\tMOV\tBX, [BP+4]\n"
    "\tMOV\tDX, [BP+6]\n"
    "\tMOV\tCX, [BP+8]\n"
    "\tMOV\tAH, 0x3F\n"
    "\tINT\t0x21\n"
    "\tJNC\t.Ok\n"
    "\tXOR\tAX, AX\n"
    "\t.Ok:\n"
    "\tMOV\tSP, BP\n"
    "\tPOP\tBP\n"
    "\tRET\n"
    "\n"
    "_write:\n"
    "\tPUSH\tBP\n"
    "\tMOV\tBP, SP\n"
    "\tMOV\tBX, [BP+4]\n"
    "\tMOV\tDX, [BP+6]\n"
    "\tMOV\tCX, [BP+8]\n"
    "\tMOV\tAH, 0x40\n"
    "\tINT\t0x21\n"
    "\tJNC\t.Ok\n"
    "\tXOR\tAX, AX\n"
    "\t.Ok:\n"
    "\tMOV\tSP, BP\n"
    "\tPOP\tBP\n"
    "\tRET\n"
    "\n"
    "_memcpy:\n"
    "\tPUSH\tBP\n"
    "\tMOV\tBP, SP\n"
    "\tPUSH\tSI\n"
    "\tPUSH\tDI\n"
    "\tMOV\tDI, [BP+4]\n"
    "\tMOV\tSI, [BP+6]\n"
    "\tMOV\tCX, [BP+8]\n"
    "\tREP\tMOVSB\n"
    "\tPOP\tDI\n"
    "\tPOP\tSI\n"
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
    OutputStr(
    "\n"
    "HeapPtr:\tdw HeapStart\n"
    "HeapStart:\n"
    );

    close(InFile);
    close(OutFile);

    return 0;
}
