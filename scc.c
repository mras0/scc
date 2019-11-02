#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

enum {
    TOKEN_MAX = 64,
    SCOPE_MAX = 10,
    VARDECL_MAX = 100,
    ID_MAX = 100,
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

    // NOTE: Must match order of registration in main
    //       TOK_BREAK must be first
    TOK_BREAK,
    TOK_CHAR,
    TOK_CONTINUE,
    TOK_ELSE,
    TOK_ENUM,
    TOK_FOR,
    TOK_IF,
    TOK_INT,
    TOK_RETURN,
    TOK_VOID,
    TOK_WHILE,

    TOK_ID
};

char* InBuf;
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

void Fatal(const char* Msg)
{
    puts(Msg);
    printf("In line %d\n", Line);
    abort();
}

const char* IdText(int id)
{
    assert(id >= 0 && id < IdCount);
    return IdBuffer + IdOffset[id];
}

int AddId(const char* Name)
{
    assert(IdCount < ID_MAX);
    const int cnt = (int)strlen(Name) + 1;
    const int ofs = IdBufferIndex;
    IdBufferIndex += cnt;
    assert(IdBufferIndex <= IDBUFFER_MAX);
    memcpy(IdBuffer + ofs, Name, cnt);
    IdOffset[IdCount++] = ofs;
    return IdCount - 1;
}

char GetChar(void)
{
    return *InBuf ? *InBuf++ : 0;
}

void UnGetChar(void)
{
    --InBuf;
}

int TryGetChar(char ch)
{
    if (*InBuf != ch) {
        return 0;
    }
    TokenText[TokenLen++] = ch;
    ++InBuf;
    return 1;
}

void SkipLine(void)
{
    int ch;
    while ((ch = GetChar()) != '\n' && ch)
        ;
    ++Line;
}

void GetToken(void)
{
    char ch;

    TokenLen = 0;
    ch = GetChar();
    while (ch <= ' ') {
        if (!ch) {
            TokenType = TOK_EOF;
            return;
        } else if (ch == '\n') {
            ++Line;
        }
        ch = GetChar();
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
        UnGetChar();
        TokenType = TOK_NUM;
    } else if (IsAlpha(ch) || ch == '_') {
        for (;;) {
            ch = GetChar();
            if (ch != '_' && !IsDigit(ch) && !IsAlpha(ch)) {
                break;
            }
            TokenText[TokenLen++] = ch;
        }
        TokenText[TokenLen] = '\0';
        UnGetChar();
        int id;
        for (id = 0; id < IdCount; ++id) {
            if (!strcmp(IdText(id), TokenText)) {
                break;
            }
        }
        TokenType = TOK_BREAK + (id < IdCount ? id : AddId(TokenText));
    } else if (ch == '\'') {
        --TokenLen;
        TokenText[TokenLen++] = GetChar();
        if (GetChar() != '\'') {
            Fatal("Invalid character literal");
        }
        TokenType = TOK_NUM;
        TokenNumVal = TokenText[0];
    } else if (ch == '"') {
        --TokenLen;
        while ((ch = GetChar()) != '"') {
            if (!ch) Fatal("Unterminated string literal");
            assert(TokenLen < TOKEN_MAX-1);
            TokenText[TokenLen++] = ch;
        }
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
        } else {
            printf("Unknown token start '%c' (0x%02X)\n", ch, ch);
            Fatal("Unknown token encountered");
        }
    }
    TokenText[TokenLen] = '\0';
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

void __declspec(noreturn) Unexpected(void)
{
    printf("%d \"%s\"\n", TokenType, TokenText);
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
        printf("Token type %d expected got ", type);
        Unexpected();
    }
}

int ExpectId(void)
{
    const int id = TokenType;
    if (id >= TOK_ID) {
        GetToken();
        return id - TOK_BREAK;
    }
    printf("Expected identifier got ");
    Unexpected();
}

int IsTypeStart(void)
{
    return TokenType == TOK_VOID || TokenType == TOK_CHAR || TokenType == TOK_INT;
}

void LvalToRval(void)
{
    if (CurrentType & VT_LVAL) {
        CurrentType &= ~VT_LVAL;
        printf("\tMOV\tBX, AX\n");
        if (CurrentType == VT_CHAR) {
            printf("\tMOV\tAL, [BX]\n");
            printf("\tCBW\n");
            CurrentType = VT_INT;
        } else {
            assert(CurrentType == VT_INT || (CurrentType & VT_PTR));
            printf("\tMOV\tAX, [BX]\n");
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
            printf("\tINC\tAX\n");
        } else {
            printf("\tADD\tAX, %d\n", Amm);
        }
    } else {
        assert(Op == TOK_MINUSMINUS);
        if (Amm == 1) {
            printf("\tDEC\tAX\n");
        } else {
            printf("\tSUB\tAX, %d\n", Amm);
        }
    }
}

void DoIncDecOp(int Op, int Post)
{
    assert(CurrentType & VT_LVAL);
    CurrentType &= ~VT_LVAL;
    printf("\tMOV\tBX, AX\n");
    if (CurrentType == VT_CHAR) {
        printf("\tMOV\tAL, [BX]\n");
        printf("\tCBW\n");
        if (Post) printf("\tPUSH\tAX\n");
        DoIncDec(Op);
        printf("\tMOV\t[BX], AL\n");
        CurrentType = VT_INT;
    } else {
        assert(CurrentType == VT_INT || (CurrentType & VT_PTR));
        printf("\tMOV\tAX, [BX]\n");
        if (Post) printf("\tPUSH\tAX\n");
        DoIncDec(Op);
        printf("\tMOV\t[BX], AX\n");
    }
    if (Post) printf("\tPOP\tAX\n");
}

void ParseExpression(void);
void ParseCastExpression(void);
void ParseAssignmentExpression(void);

void ParsePrimaryExpression(void)
{
    if (TokenType == TOK_LPAREN) {
        GetToken();
        ParseExpression();
        Expect(TOK_RPAREN);
    } else if (TokenType == TOK_NUM) {
        printf("\tMOV\tAX, 0x%04X\n", (unsigned short)TokenNumVal);
        CurrentType = VT_INT;
        GetToken();
    } else if (TokenType == TOK_STRLIT) {
        const int SL = LocalLabelCounter++;
        const int JL = LocalLabelCounter++;
        printf("\tJMP\t.L%d\n", JL);
        printf(".L%d:\tDB '%s', 0\n", SL, TokenText);
        printf(".L%d:\n", JL);
        printf("\tMOV\tAX, .L%d\n", SL);
        CurrentType = VT_PTR | VT_CHAR;
        GetToken();
    } else {
        const int id = ExpectId();
        int vd;

        // Lookup
        assert(ScopesCount);
        for (vd = Scopes[ScopesCount-1]; vd >= 0; vd--) {
            if (VarDeclId[vd] == id) {
                break;
            }
        }
        if (vd < 0) {
            // Lookup failed. Assume function returning int.
            CurrentType = VT_FUN|VT_INT;
            printf("\tMOV\tAX, _%s\n", IdText(id));
        } else {
            if (VarDeclType[vd] & VT_ENUM) {
                printf("\tMOV\tAX, %d\n", VarDeclOffset[vd]);
                CurrentType = VT_INT;
                return;
            }
            CurrentType = VarDeclType[vd] | VT_LVAL;
            if (VarDeclOffset[vd]) {
                printf("\tLEA\tAX, [BP%+d]\t; %s\n", VarDeclOffset[vd], IdText(id));
            } else {
                printf("\tMOV\tAX, _%s\n", IdText(id));
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
            // Function call
            if (!(CurrentType & VT_FUN)) {
                Fatal("Not a function");
            }
            printf("\tPUSH\tSI\n");
            printf("\tPUSH\tDI\n");
            printf("\tMOV\tSI, AX\n");
            const int RetType = CurrentType & ~VT_FUN;
            const int MaxArgs = 4; // TODO: Handle this better...
            printf("\tSUB\tSP, %d\n", MaxArgs*2);
            printf("\tMOV\tDI, SP\n");
            int NumArgs = 0;
            while (TokenType != TOK_RPAREN) {
                assert(NumArgs < MaxArgs);
                ParseAssignmentExpression();
                LvalToRval();
                assert(CurrentType == VT_INT || (CurrentType & VT_PTR));
                printf("\tMOV\t[DI%+d], AX\n", NumArgs*2);
                ++NumArgs;

                if (!Accept(TOK_COMMA)) {
                    break;
                }
            }
            Expect(TOK_RPAREN);
            printf("\tCALL\tSI\n");
            printf("\tADD\tSP, %d\n", MaxArgs*2);
            printf("\tPOP\tDI\n");
            printf("\tPOP\tSI\n");
            CurrentType = RetType;
        } else if (Accept(TOK_LBRACKET)) {
            LvalToRval();
            if (!(CurrentType & VT_PTR)) {
                Fatal("Expected pointer");
            }
            const int AType = CurrentType & ~VT_PTR;
            printf("\tPUSH\tAX\n");
            ParseExpression();
            Expect(TOK_RBRACKET);
            LvalToRval();
            assert(CurrentType == VT_INT);
            if (AType != VT_CHAR) {
                assert(AType == VT_INT);
                printf("\tADD\tAX, AX\n");
            }
            printf("\tPOP\tCX\n");
            printf("\tADD\tAX, CX\n");
            CurrentType = AType | VT_LVAL;
        } else {
            const int Op = TokenType;
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
    const int Op = TokenType;
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
            assert(CurrentType == VT_INT);
        } else if (Op == TOK_MINUS) {
            assert(CurrentType == VT_INT);
            printf("\tNEG\tAX\n");
        } else if (Op == TOK_TILDE) {
            assert(CurrentType == VT_INT);
            printf("\tNOT\tAX\n");
        } else if (Op == TOK_NOT) {
            const int Lab = LocalLabelCounter++;
            assert(CurrentType == VT_INT);
            printf("\tAND\tAX, AX\n");
            printf("\tMOV\tAX, 0\n");
            printf("\tJNZ\t.L%d\n", Lab);
            printf("\tINC\tAL\n");
            printf(".L%d:\n", Lab);
        } else {
            Unexpected();
        }
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
        const int Lab = LocalLabelCounter++;
        printf("\tCMP\tAX, CX\n");
        printf("\tMOV\tAX, 1\n");
        RelOp(Op);
        printf("\tJ%s\t.L%d\n", RelOp(Op), Lab);
        printf("\tDEC\tAL\n");
        printf(".L%d:\n", Lab);
        return;
    }

    if (Op == TOK_PLUS || Op == TOK_PLUSEQ) {
        printf("\tADD\tAX, CX\n");
    } else if (Op == TOK_MINUS || Op == TOK_MINUSEQ) {
        printf("\tSUB\tAX, CX\n");
    } else if (Op == TOK_STAR || Op == TOK_STAREQ) {
        printf("\tMUL\tCX\n");
    } else if (Op == TOK_SLASH || Op == TOK_SLASHEQ || Op == TOK_MOD || Op == TOK_MODEQ) {
        printf("\tXOR\tDX, DX\n");
        printf("\tIDIV\tCX\n");
        if (Op == TOK_MOD || Op == TOK_MODEQ) {
            printf("\tMOV\tAX, DX\n");
        }
    } else if (Op == TOK_AND || Op == TOK_ANDEQ) {
        printf("\tAND\tAX, CX\n");
    } else if (Op == TOK_XOR || Op == TOK_XOREQ) {
        printf("\tXOR\tAX, CX\n");
    } else if (Op == TOK_OR || Op == TOK_OREQ) {
        printf("\tOR\tAX, CX\n");
    } else if (Op == TOK_LSH || Op == TOK_LSHEQ) {
        printf("\tSHL\tAX, CL\n");
    } else if (Op == TOK_RSH || Op == TOK_RSHEQ) {
        printf("\tSAR\tAX, CL\n");
    }
}

void ParseExpression1(int OuterPrecedence)
{
    int LEnd;
    for (;;) {
        const int Op   = TokenType;
        const int Prec = OperatorPrecedence(Op);
        if (Prec > OuterPrecedence) {
            break;
        }
        GetToken();

        const int IsAssign = Prec == OperatorPrecedence(TOK_EQ);

        if (IsAssign) {
            if (!(CurrentType & VT_LVAL)) {
                Fatal("L-value required");
            }
        } else {
            LvalToRval();
        }
        int LhsType = CurrentType;

        if (Op == TOK_ANDAND || Op == TOK_OROR) {
            LEnd = LocalLabelCounter++;
            assert(CurrentType == VT_INT);
            printf("\tAND\tAX, AX\n");
            printf("\tJ%s\t.L%d\n", Op == TOK_ANDAND ? "Z" : "NZ", LEnd);
        } else {
            LEnd = -1;
            printf("\tPUSH\tAX\n");
        }

        // TODO: Question, ?:
        /*rhs = */ ParseCastExpression();
        for (;;) {
            const int LookAheadOp         = TokenType;
            const int LookAheadPrecedence = OperatorPrecedence(LookAheadOp);
            if (LookAheadPrecedence > Prec /* || (LookAheadPrecedence == Prec && !IsRightAssociative(LookAheadOp)) */) {
                break;
            }
            ParseExpression1(/*rhs, */LookAheadPrecedence);
        }
        LvalToRval();
        if (LEnd >= 0) {
            printf(".L%d:\n", LEnd);
        } else if (IsAssign) {
            assert(LhsType & VT_LVAL);
            LhsType &= ~VT_LVAL;
            printf("\tPOP\tBX\n");
            if (Op != TOK_EQ) {
                assert(LhsType == VT_INT);
                assert(CurrentType == VT_INT);
                printf("\tMOV\tCX, AX\n");
                printf("\tMOV\tAX, [BX]\n");
                DoBinOp(Op);
            }
            if (LhsType == VT_CHAR) {
                assert(CurrentType == VT_INT);
                printf("\tMOV\t[BX], AL\n");
            } else {
                assert(LhsType == VT_INT || (LhsType & VT_PTR));
                assert(CurrentType == VT_INT || (CurrentType & VT_PTR));
                printf("\tMOV\t[BX], AX\n");
            }
        } else {
            assert(LhsType == VT_INT);
            assert(CurrentType == VT_INT);
            printf("\tPOP\tCX\n");
            printf("\tXCHG\tAX, CX\n");
            DoBinOp(Op);
        }
    }
}

void ParseExpression0(int OuterPrecedence)
{
    ParseCastExpression();
    ParseExpression1(OuterPrecedence);
}

void ParseExpression(void)
{
    ParseExpression0(OperatorPrecedence(TOK_COMMA));
}

void ParseAssignmentExpression(void)
{
    ParseExpression0(OperatorPrecedence(TOK_EQ));
}

int ParseDeclSpecs(void)
{
    assert(IsTypeStart());
    int t;
    if (TokenType == TOK_VOID) {
        t = VT_VOID;
    } else if (TokenType == TOK_CHAR) {
        t = VT_CHAR;
    } else if (TokenType == TOK_INT) {
        t = VT_INT;
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
    assert(ScopesCount);
    assert(Scopes[ScopesCount-1] < VARDECL_MAX);
    const int vd = Scopes[ScopesCount-1]++;
    VarDeclType[vd]   = Type;
    VarDeclOffset[vd] = 0;
    VarDeclId[vd]     = Id;
    return vd;
}

int ParseDecl(void)
{
    const int Type = ParseDeclSpecs();
    const int Id   = ExpectId();
    return AddVarDecl(Type, Id);
}

void PushScope(void)
{
    assert(ScopesCount < SCOPE_MAX);
    const int End = ScopesCount ? Scopes[ScopesCount-1] : 0;
    Scopes[ScopesCount++] = End;
}

void PopScope(void)
{
    assert(ScopesCount);
    --ScopesCount;
}

void ParseCompoundStatement(void);

void ParseStatement(void)
{
    const int OldBreak    = BreakLabel;
    const int OldContinue = ContinueLabel;

    if (TokenType == TOK_LBRACE) {
        ParseCompoundStatement();
    } else if (IsTypeStart()) {
        const int vd = ParseDecl();
        LocalOffset -= 2;
        VarDeclOffset[vd] = LocalOffset;
        printf("\tSUB\tSP, 2\n");
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_FOR)) {
        const int CondLabel  = LocalLabelCounter++;
        const int BodyLabel  = LocalLabelCounter++;
        const int EndLabel   = LocalLabelCounter++;
        int IterLabel = CondLabel;
        Expect(TOK_LPAREN);

        // Init
        if (TokenType != TOK_SEMICOLON) {
            ParseExpression();
        }
        Expect(TOK_SEMICOLON);

        // Cond
        printf(".L%d:\t; Cond for\n", CondLabel);
        if (TokenType != TOK_SEMICOLON) {
            ParseExpression();
            LvalToRval();
            assert(CurrentType == VT_INT);
            printf("\tAND\tAX, AX\n");
            printf("\tJNZ\t.L%d\n", BodyLabel); // TODO: Need far jump?
            printf("\tJMP\t.L%d\n", EndLabel);
        } else {
            printf("\tJMP\t.L%d\n", BodyLabel);
        }
        Expect(TOK_SEMICOLON);

        // Iter
        if (TokenType != TOK_RPAREN) {
            IterLabel  = LocalLabelCounter++;
            printf(".L%d:\t; Iter for\n", IterLabel);
            ParseExpression();
            printf("\tJMP\t.L%d\n", CondLabel);
        }
        Expect(TOK_RPAREN);

        printf(".L%d:\t; Body for\n", BodyLabel);
        BreakLabel    = EndLabel;
        ContinueLabel = IterLabel;
        ParseStatement();
        printf("\tJMP\t.L%d\n", IterLabel);
        printf(".L%d:\t; End for\n", EndLabel);
    } else if (Accept(TOK_IF)) {
        const int IfLabel    = LocalLabelCounter++;
        const int ElseLabel  = LocalLabelCounter++;
        const int EndLabel   = LocalLabelCounter++;
        printf("\t; Start of if\n");
        Accept(TOK_LPAREN);
        ParseExpression();
        LvalToRval();
        assert(CurrentType == VT_INT);
        printf("\tAND\tAX, AX\n");
        printf("\tJNZ\t.L%d\n", IfLabel);
        printf("\tJMP\t.L%d\n", ElseLabel);
        Accept(TOK_RPAREN);
        printf(".L%d:\t; If\n", IfLabel);
        ParseStatement();
        printf("\tJMP\t.L%d\n", EndLabel);
        printf(".L%d:\t; Else\n", ElseLabel);
        if (Accept(TOK_ELSE)) {
            ParseStatement();
        }
        printf(".L%d:\t; End if\n", EndLabel);
    } else if (Accept(TOK_RETURN)) {
        if (TokenType != TOK_SEMICOLON) {
            ParseExpression();
        }
        printf("\tJMP\t.L%d\n", ReturnLabel);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_WHILE)) {
        const int StartLabel = LocalLabelCounter++;
        const int BodyLabel  = LocalLabelCounter++;
        const int EndLabel   = LocalLabelCounter++;
        printf(".L%d:\t; Start of while\n", StartLabel);
        Expect(TOK_LPAREN);
        ParseExpression();
        LvalToRval();
        assert(CurrentType == VT_INT);
        printf("\tAND\tAX, AX\n");
        printf("\tJNZ\t.L%d\n", BodyLabel);
        printf("\tJMP\t.L%d\n", EndLabel);
        Expect(TOK_RPAREN);
        printf(".L%d:\t; Body of while\n", BodyLabel);
        BreakLabel = EndLabel;
        ContinueLabel = StartLabel;
        ParseStatement();
        printf("\tJMP\t.L%d\n", StartLabel);
        printf(".L%d:\t; End of while\n", EndLabel);
    } else if (Accept(TOK_BREAK)) {
        assert(BreakLabel >= 0);
        printf("\tJMP\t.L%d\n", BreakLabel);
        Expect(TOK_SEMICOLON);
    } else if (Accept(TOK_CONTINUE)) {
        assert(ContinueLabel >= 0);
        printf("\tJMP\t.L%d\n", ContinueLabel);
        Expect(TOK_SEMICOLON);
    } else {
        // Expression statement
        ParseExpression();
        Expect(TOK_SEMICOLON);
    }

    BreakLabel    = OldBreak;
    ContinueLabel = OldContinue;
}

void ParseCompoundStatement(void)
{
    PushScope();
    Expect(TOK_LBRACE);
    while (!Accept(TOK_RBRACE)) {
        ParseStatement();
    }
    PopScope();
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
        Expect(TOK_LBRACE);
        int EnumVal = 0;
        while (TokenType != TOK_RBRACE) {
            const int id = ExpectId();
            if (Accept(TOK_EQ)) {
                EnumVal = TokenNumVal;
                Expect(TOK_NUM);
            }
            const int vd = AddVarDecl(VT_INT|VT_ENUM, id);
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

    const int fd = ParseDecl();

    if (Accept(TOK_SEMICOLON)) {
        printf("_%s:\n", IdText(VarDeclId[fd]));
        printf("\tDW\t0\n");
        return;
    }

    // Note: We don't actually care that we might end up
    // with multiple VarDecl's for functions with prototypes.
    Expect(TOK_LPAREN);
    VarDeclType[fd] |= VT_FUN;
    PushScope();
    int ArgOffset = 4;
    while (TokenType != TOK_RPAREN) {
        if (Accept(TOK_VOID)) {
            break;
        }
        const int vd = ParseDecl();
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
        printf("_%s:\n", IdText(VarDeclId[fd]));
        printf("\tPUSH\tBP\n");
        printf("\tMOV\tBP, SP\n");
        ParseCompoundStatement();
        printf(".L%d:\n", ReturnLabel);
        printf("\tMOV\tSP, BP\n");
        printf("\tPOP\tBP\n");
        printf("\tRET\n");
    }
    PopScope();
}

void* Malloc(int size)
{
    void* ptr = malloc(size);
    if (!ptr) Fatal("Out of memory");
    return ptr;
}

int main(void)
{
    //InBuf = "void putchar(char c); void puts(char* s) { int i; i = 0; while (s[i]) { putchar(s[i]); i = i + 1; } putchar(13); putchar(10); } void main() { puts(\"Hello world!\"); }";
    //InBuf = "void putchar(char c); int IsAlpha(char ch) { return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'); } void main() { int c; c = 0; while (c<256) { if (IsAlpha(c)) putchar(c); ++c; } }";
    //InBuf = "void main() { int i; for (i = 0; ;++i) { if (!(i&1)) continue; if(i>=10)break; putchar('0'+i); } }";
    //InBuf = "void main() { int i; i = 0; while (1) { ++i; if (i==10) break; if (i&1) continue; putchar('0'+i); } }";
    //InBuf = "enum { A, B, X=8, Y }; void main() { putchar('0'+A); putchar('0'+B); putchar('0'+X); putchar('0'+Y); }";
    //InBuf = "void main() { putchar('0' + 1+2*3); }";

    InBuf = "int a; void main() { a=42; putchar(a); }";
#if 0
    FILE* fp = fopen("../scc.c", "rb");
    if (!fp) Fatal("Error opening input");
    fseek(fp, 0L, SEEK_END);
    int size = (int)ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    InBuf = Malloc(size+1);
    fread(InBuf, size, 1, fp);
    InBuf[size] = 0;
#endif

    TokenText     = Malloc(TOKEN_MAX);
    IdBuffer      = Malloc(IDBUFFER_MAX);
    IdOffset      = Malloc(sizeof(int)*ID_MAX);
    VarDeclId     = Malloc(sizeof(int)*VARDECL_MAX);
    VarDeclType   = Malloc(sizeof(int)*VARDECL_MAX);
    VarDeclOffset = Malloc(sizeof(int)*VARDECL_MAX);
    Scopes        = Malloc(sizeof(int)*SCOPE_MAX);

    AddId("break");
    AddId("char");
    AddId("continue");
    AddId("else");
    AddId("enum");
    AddId("for");
    AddId("if");
    AddId("int");
    AddId("return");
    AddId("void");
    AddId("while");
    assert(IdCount+TOK_BREAK-1 == TOK_WHILE);

    // Prelude
    puts(
    "\tcpu 8086\n"
    "\torg 0x100\n"
    "Start:\n"
    "\tXOR\tBP, BP\n"
    "\tCALL\t_main\n"
    "\tMOV\tAH, 0x4C\n"
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
);

    PushScope();
    Line = 1;
    GetToken();
    while (TokenType != TOK_EOF) {
        ParseExternalDefition();
    }
    PopScope();

    // Postlude
    puts(
    "\n"
    "HeapPtr:\tdw HeapStart\n"
    "HeapStart:\n"
    );

    return 0;
}
