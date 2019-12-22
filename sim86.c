#include "lib.h"

enum {
    R_AX,
    R_CX,
    R_DX,
    R_BX,
    R_SP,
    R_BP,
    R_SI,
    R_DI
};

enum {
    SR_ES,
    SR_CS,
    SR_SS,
    SR_DS,
};

enum {
    OP_ADD,
    OP_OR,
    OP_ADC,
    OP_SBB,
    OP_AND,
    OP_SUB,
    OP_XOR,
    OP_CMP,
};

enum {
    FC = 1<<0,
    FZ = 1<<6,
    FS = 1<<7,
    FO = 1<<11,
};

enum {
    DEFAULT_SEGMENT = 0x1000,
    LOAD_OFFSET     = 0x100,
};

int reg[8];
int sreg[4];
int ip;
int flags;
#ifndef __SCC__
char mem[65536];
#else
int memseg;
#endif

int modrm;
int addr;
int dsize;
char rmtext[16];

int verbose;

#ifndef __SCC__
#define PROFILING
#endif

#ifdef PROFILING
unsigned long long counts[65536];
unsigned long long total_cycles;
unsigned long long cycles;
const char* filename;
int stack_low = 0xffff;
int do_profile;
#endif

#ifdef __SCC__
#define TRIM(x) x
#else
#define TRIM(x) ((x)&0xffff)
#endif

void PrintState(void);

NORETURN void Fatal(const char* fmt, ...)
{
    PrintState();
    char buf[80];
    va_list vl;
    va_start(vl, fmt);
    vsprintf(buf, fmt, vl);
    va_end(vl);
    puts(buf);
    exit(1);
}

const char* RegName(int r)
{
    switch (r) {
    default:
        assert(0);
    case R_AX: return dsize ? "AX" : "AL";
    case R_CX: return dsize ? "CX" : "CL";
    case R_DX: return dsize ? "DX" : "DL";
    case R_BX: return dsize ? "BX" : "BL";
    case R_SP: return dsize ? "SP" : "AH";
    case R_BP: return dsize ? "BP" : "CH";
    case R_SI: return dsize ? "SI" : "DH";
    case R_DI: return dsize ? "DI" : "BH";
    }
}

const char* OpName(int op)
{
    switch (op) {
    default:
        assert(0);
    case OP_ADD: return "ADD";
    case OP_OR:  return "OR";
    case OP_ADC: return "ADC";
    case OP_SBB: return "SBB";
    case OP_AND: return "AND";
    case OP_SUB: return "SUB";
    case OP_XOR: return "XOR";
    case OP_CMP: return "CMP";
    }
}

void PrintReg(int r)
{
    printf("%s=%04X ", RegName(r), TRIM(reg[r]));
}

void PrintState(void)
{
    const int old = dsize;
    dsize = 1;
    puts("");
    PrintReg(R_AX);
    PrintReg(R_BX);
    PrintReg(R_CX);
    PrintReg(R_DX);
    PrintReg(R_SP);
    PrintReg(R_BP);
    PrintReg(R_SI);
    PrintReg(R_DI);
    printf("\nDS=%04X ES=%04X SS=%04X CS=%04X IP=%04X ", TRIM(sreg[SR_DS]), TRIM(sreg[SR_ES]), TRIM(sreg[SR_SS]), TRIM(sreg[SR_CS]), TRIM(ip));
    printf("%s %s %s %s\n", flags&FO?"OV":"NV", flags&FS?"NG":"PL", flags&FZ?"ZR":"NZ", flags&FC?"CY":"NC");
    dsize = old;
}

#ifdef __SCC__
int GetDS(void)
{
    _emit 0x8C _emit 0xD8               // MOV AX, DS
}

int FarPeek(int seg, int off)
{
    _emit 0x1E                         // PUSH DS
    _emit 0x8E _emit 0x5E _emit 0x04   // MOV DS, [BP+4]
    _emit 0x8B _emit 0x5E _emit 0x06   // MOV BX, [BP+6]
    _emit 0x8A _emit 0x07              // MOV AL, [BX]
    _emit 0x30 _emit 0xE4              // XOR AH, AH
    _emit 0x1F                         // POP DS
}

void FarPoke(int seg, int off, int val)
{
    _emit 0x1E                         // PUSH DS
    _emit 0x8E _emit 0x5E _emit 0x04   // MOV DS, [BP+4]
    _emit 0x8B _emit 0x5E _emit 0x06   // MOV BX, [BP+6]
    _emit 0x8A _emit 0x46 _emit 0x08   // MOV AL, [BP+8]
    _emit 0x88 _emit 0x07              // MOV [BX], AL
    _emit 0x1F                         // POP DS
}
#else
void CheckMem(int sr, int off)
{
    if (sreg[sr] != DEFAULT_SEGMENT || off < 0 || off >= (int)sizeof(mem)) {
        Fatal("Out of bounds memory access: %04X:%04X\n", sreg[sr], off);
    }
}
#endif


int Read8(int sr, int off)
{
#ifdef PROFILING
    ++cycles;
#endif

#ifdef __SCC__
    return FarPeek(memseg, off);
#else
    off = TRIM(off);
    CheckMem(sr, off);
    return mem[off] & 0xff;
#endif
}

int Read16(int sr, int off)
{
    return Read8(sr, off) | Read8(sr, off+1)<<8;
}

void Write8(int sr, int off, int val)
{
#ifdef PROFILING
    ++cycles;
#endif

#ifdef __SCC__
    FarPoke(memseg, off, val);
#else
    off = TRIM(off);
    CheckMem(sr, off);
    mem[off] = (char)val;
#endif
}

void Write16(int sr, int off, int val)
{
    Write8(sr, off, val);
    Write8(sr, off+1, val>>8);
}

int ReadIByte(void)
{
    return Read8(SR_CS, ip++);
}

int Imm16(void)
{
    const int l = ReadIByte();
    return l | ReadIByte() << 8;
}

void ModRM(void)
{
    assert(modrm == -1);
    modrm = ReadIByte();
    switch (modrm>>6) {
    case 0:
        if ((modrm & 7) == 6) { // Pure disp16
            addr += Imm16();
            if (verbose) sprintf(rmtext, "[0x%04X]", addr);
            return;
        }
        break;
    case 1:
        addr += (char)ReadIByte();
        break;
    case 2:
        addr += Imm16();
        break;
    case 3:
        strcpy(rmtext, RegName(modrm&7));
        return;
    }
    if (verbose) {
        switch (modrm&7) {
        case 0: sprintf(rmtext, "[BX+SI%+d]", addr);break;
        case 1: sprintf(rmtext, "[BX+DI%+d]", addr);break;
        case 2: sprintf(rmtext, "[BP+SI%+d]", addr);break;
        case 3: sprintf(rmtext, "[BP+DI%+d]", addr);break;
        case 4: sprintf(rmtext, "[SI%+d]", addr);   break;
        case 5: sprintf(rmtext, "[DI%+d]", addr);   break;
        case 6: sprintf(rmtext, "[BP%+d]", addr);   break;
        case 7: sprintf(rmtext, "[BX%+d]", addr);   break;
        }
    }
    switch (modrm&7) {
    case 0: addr += reg[R_BX] + reg[R_SI]; break;
    case 1: addr += reg[R_BX] + reg[R_DI]; break;
    case 2: addr += reg[R_BP] + reg[R_SI]; break;
    case 3: addr += reg[R_BP] + reg[R_DI]; break;
    case 4: addr += reg[R_SI]; break;
    case 5: addr += reg[R_DI]; break;
    case 6: addr += reg[R_BP]; break;
    case 7: addr += reg[R_BX]; break;
    }
}

int ReadRM(void)
{
    assert(modrm != -1);
    if (modrm>>6==3) {
        assert(dsize || !(modrm&4));//TODO: AH-DH
        return reg[modrm&7];
    }
    return dsize ? Read16(SR_DS, addr) : Read8(SR_DS, addr);
}

void WriteRM(int val)
{
    if (modrm>>6==3) {
        int r = modrm&7;
        if (!dsize) {
            if (r&4)
                val = (val<<8)|(reg[r]&0xff);
            else
                val = (val&0xff)|(reg[r]&0xff00);
        }
        reg[r] = val;
        return;
    }
    if (dsize)
        Write16(SR_DS, addr, val);
    else
        Write8(SR_DS, addr, val);
}

void DoPush(int val)
{
    reg[R_SP] -= 2;
    Write16(SR_SS, reg[R_SP], val);
}

int DoPop(void)
{
    const int v = Read16(SR_SS, reg[R_SP]);
    reg[R_SP] += 2;
    return v;
}

void DoOp(int op, int l, int r)
{
    if (!dsize) {
        l &= 0xff;
        r &= 0xff;
    }
    int res;
    int carry = 0;
    switch (op) {
    case OP_ADD:
        res = l + r;
    AddDone:
        carry = (l & r) | ((l | r) & ~res);
        break;
    case OP_OR:
        res = l | r;
        break;
    case OP_ADC:
        res = l + r + (flags & FC);
        goto AddDone;
    case OP_SBB:
        res = l - r - (flags & FC);
    SubDone:
        carry = (~l & r) | (~(l ^ r) & res);
        break;
    case OP_AND:
        res = l & r;
        break;
    case OP_CMP:
    case OP_SUB:
        res = l - r;
        goto SubDone;
    case OP_XOR:
        res = l ^ r;
        break;
    default:
        Fatal("TODO: %s %d, %d", OpName(op), l, r);
    }
    const int mask = dsize ? 0x8000 : 0x80;
    flags = 0;
    if (res & mask)
        flags |= FS;
    if (!TRIM(res))
        flags |= FZ;
    if (carry & mask)
        flags |= FC;
    if (((carry << 1) ^ carry) & mask)
        flags |= FO;
    if (op != OP_CMP)
        WriteRM(res);
}

void ALUOp(int op, int swap)
{
    ModRM();
    int r = modrm>>3&7;
    int rval = reg[r];
    int rmval = ReadRM();

    if (swap) {
        modrm = 0xc0|r; // Need to write to register
        DoOp(op, rval, rmval);
        if (verbose) printf("%s %s, %s", OpName(op), RegName(r), rmtext);
    } else {
        DoOp(op, rmval, rval);
        if (verbose) printf("%s %s, %s", OpName(op), rmtext, RegName(r));
    }
}

void MOV(int swap)
{
    ModRM();
    int r = modrm>>3&7;
    if (swap) {
        if (verbose) printf("MOV %s, %s", RegName(r), rmtext);
        reg[r] = ReadRM();
    } else {
        if (verbose) printf("MOV %s, %s", rmtext, RegName(r));
        WriteRM(reg[r]);
    }
}

int ReadToMem(int fd, int off, int size)
{
#ifdef __SCC__
    int s = memseg;
    _emit 0x1F // POP DS
    _emit 0x0E // PUSH CS (Match stack layout)
    read(fd, off, size);
    _emit 0x0E // PUSH CS
    _emit 0x1F // POP DS
#else
    return read(fd, mem+off, size);
#endif
}

int WriteFromMem(int fd, int off, int size)
{
#ifdef __SCC__
    int s = memseg;
    _emit 0x1F // POP DS
    _emit 0x0E // PUSH CS (Match stack layout)
    write(fd, off, size);
    _emit 0x0E // PUSH CS
    _emit 0x1F // POP DS
#else
    return write(fd, mem+off, size);
#endif
}

void ReadFile(const char* Filename)
{
    const int fd = open(Filename, O_BINARY | O_RDONLY);
    if (fd < 0) {
        Fatal("Error opening %s", Filename);
    }
    ReadToMem(fd, LOAD_OFFSET, 65536-LOAD_OFFSET);
    close(fd);
}

void JMP(int rel)
{
    if (verbose) printf("JMP 0x%04X", TRIM(ip+rel));
    ip += rel;
}

void PUSH(int r)
{
    if (verbose) printf("PUSH %s", RegName(r));
    DoPush(reg[r]);
}

void POP(int r)
{
    if (verbose) printf("POP %s", RegName(r));
    reg[r] = DoPop();
}

void JCC(int cc)
{
    const int rel = (char)ReadIByte();
    const char* n;
    const int e = !(cc & 1); // Expect true?
    int j;
    switch (cc>>1) {
    case 2:
        n = e ? "JZ" : "JNZ";
        j = !!(flags & FZ);
        break;
    case 3:
        n = e ? "JNA" : "JA";
        j = (flags & FZ) || (flags & FC);
        break;
    case 6:
        n = e ? "JL" : "JNL";
        j = !(flags & FS) != !(flags & FO);
        break;
    case 7:
        n = e ? "JNG" : "JG";
        j = (flags & FZ) || (!(flags & FS) != !(flags & FO));
        break;
    default:
        Fatal("TODO: JCC cc=%d rel=%d [%X]", cc, rel, ip+rel);
    }
    if (verbose) printf("%s 0x%04X", n, TRIM(ip+rel));
    if (j == e)
        ip += rel;
}

// TODO: INC/DEC shouldn't modify carry(?)
void DoIncDec(int v)
{
    const int s = ReadRM();
    WriteRM(s+v);
}

void INC(void)
{
    if (verbose) printf("INC %s", rmtext);
    DoIncDec(1);
}

void DEC(void)
{
    if (verbose) printf("DEC %s", rmtext);
    DoIncDec(-1);
}

void ReadAsciiz(char* dest, int sr, int off)
{
    int ch;
    do {
        ch = Read8(sr, off++);
        *dest++ = (char)ch;
    } while (ch); 
}

void HandleOpen(int oflags)
{
    char fname[64];
    ReadAsciiz(fname, SR_DS, reg[R_DX]);
    int fd = open(fname, O_BINARY | oflags, 0600);
    if (verbose) printf("\nopen(%s, %d) -> %d\n", fname, oflags, fd);
    if (fd < 0) {
        flags |= FC;
        fd = 2; // File not found
    } else {
        flags &= ~FC;
    }
    reg[R_AX] = fd;
}

void DoExit(int ec)
{
    if (verbose) {
        if (ec)
            printf("\nExiting with error code %d\n", ec);
        else
            printf("\nNormal exit\n");
    }
#ifdef PROFILING
    if (do_profile) {
        struct E {
            int Addr;
            char Id[64];
            unsigned long long Total;
        } entries[4096] = {
            { 0, "@@start", 0 },
        };
        int num_entries = 1;
        char fname[256];
        strcpy(fname, filename);
        char* d = strrchr(fname, '.');
        if (d) {
            strcpy(d, ".map");
            FILE* fp = fopen(fname, "rt");
            if (fp) {
                while (fscanf(fp, "%4X %63s\n", &entries[num_entries].Addr, &entries[num_entries].Id) == 2) {
                    ++num_entries;
                }
                fclose(fp);
            }
        }

        printf("\nTotal cylces;%llu\nStack low;0x%04X\n\n", total_cycles, stack_low&0xffff);
        int entry = 0;
        printf("Count;Address;Name\n");
        for (int i = 0; i < 65536; ++i) {
            if (entry+1 < num_entries && i >= entries[entry+1].Addr) {
                ++entry;
            }
            if (counts[i])
                printf("%llu;0x%04X;%s+0x%X\n", counts[i], i, entries[entry].Id, i - entries[entry].Addr);
            if (entry >= 0)
                entries[entry].Total += counts[i];
        }
        printf("\nCount;Function\n");
        for (int i = 0; i < num_entries; ++i) {
            if (entries[i].Total)
                printf("%llu;%s\n", entries[i].Total, entries[i].Id);
        }
    }
#endif
    exit(ec);
}

void INT(void)
{
    const int intr = ReadIByte();
    if (verbose) printf("INT 0x%02X", intr);
    if (intr == 0x20) {
        DoExit(0);
    }
    if (intr != 0x21) {
        Fatal("TODO: INT %02X", intr);
    }
    const int func = reg[R_AX]>>8&0xff;
    if (func == 0x02) {
        int c = reg[R_DX]&0xff;
        if (verbose)
            printf("\nOutput: 0x%02X (%c)", c, c >= ' ' ? c : '.');
        else
            putchar(c);
    } else if (func == 0x3C) {
        HandleOpen(O_WRONLY | O_CREAT | O_TRUNC);
    } else if (func == 0x3D) {
        HandleOpen(O_RDONLY);
    } else if (func == 0x3E) {
        if (verbose) printf("\nclose(%d)\n", reg[R_BX]);
        close(reg[R_BX]);
        flags &= ~FC;
    } else if (func == 0x3F) {
        int r = ReadToMem(reg[R_BX], reg[R_DX], reg[R_CX]);
        if (r <= 0) {
            flags |= FC;
            r = 0;
        } else {
            flags &= ~FC;
        }
        reg[R_AX] = r;
    } else if (func == 0x40) {
        int r = WriteFromMem(reg[R_BX], reg[R_DX], reg[R_CX]);
        if (r <= 0) {
            flags |= FC;
            r = 0;
        } else {
            flags &= ~FC;
        }
        reg[R_AX] = r;
    } else if (func == 0x4C) {
        DoExit(reg[R_AX]&0xff);
    } else {
        Fatal("TODO: INT 0x21/AH=%02X", func);
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) {
    Usage:
        printf("Usage: sim86 [-v] "
#ifdef PROFILING
                "[-p] "
#endif
                "com-file [...]\n");
        return 1;
    }
#ifdef __SCC__
    memseg = GetDS() + 0x1000;
#endif

    int argi = 1;
    for (; argv[argi]; ++argi) {
        if (!strcmp(argv[argi], "-v")) {
            verbose = 1;
#ifdef PROFILING
        } else if (!strcmp(argv[argi], "-p")) {
            do_profile = 1;
#endif
        } else {
            break;
        }
    }
    if (!argv[argi])
        goto Usage;

    sreg[0] = sreg[1] = sreg[2] = sreg[3] = DEFAULT_SEGMENT;
#ifdef PROFILING
    filename = argv[argi];
#endif
    ReadFile(argv[argi]);

    Write16(SR_CS, 0, 0x20CD);
    reg[R_SP] = 0;
    int p = 0;
    int i;
    for (i = argi + 1; argv[i]; ++i) {
        if (i != 2) Write8(SR_DS, 0x81+(p++), ' ');
        const char* s = argv[i];
        while (*s) Write8(SR_DS, 0x81+(p++), *s++);
    }
    Write8(SR_DS, 0x82+p, '\r');
    Write8(SR_DS, 0x80, p);

    DoPush(0);
    ip = LOAD_OFFSET;
    for (;;) {
#ifdef PROFILING
        const int orig_ip = TRIM(ip);
        cycles = 0;
        reg[R_SP]=TRIM(reg[R_SP]);
        if (reg[R_SP] && reg[R_SP] < stack_low)
            stack_low = reg[R_SP];
#endif
        if (verbose) printf("%04X ", TRIM(ip));

        modrm = -1;
        addr = 0;
        dsize = 1;

        int rep = 0;

    Restart: ;
        const int inst = ReadIByte();

        if (inst == 0xF3) {
            rep = inst;
            goto Restart;
        }

        if (inst < 0x40) {
            const int type = inst&6;
            const int op = (inst >> 3) & 7;
            dsize = inst&1;
            if (type < 4) {
                ALUOp(op, inst & 2);
            } else if (type == 4) {
                int Imm;
                if (dsize)
                    Imm = Imm16();
                else
                    Imm = (char)ReadIByte();
                modrm = 0xc0;
                DoOp(op, reg[R_AX], Imm);
                if (verbose) printf("%s A%c, 0x%04X", OpName(op), dsize?'X':'L', Imm);
            } else {
                Fatal("TODO: %02X", inst);
            }
        } else if (inst >= 0x40 && inst < 0x48) {
            int r = inst - 0x40;
            modrm = 0xc0 | r << 3 | r;
            INC();
        } else if (inst >= 0x48 && inst < 0x50) {
            int r = inst - 0x48;
            modrm = 0xc0 | r << 3 | r;
            DEC();
        } else if (inst >= 0x50 && inst < 0x58) {
            PUSH(inst-0x50);
        } else if (inst >= 0x58 && inst < 0x60) {
            POP(inst-0x58);
        } else if (inst >= 0x70 && inst < 0x80) {
            JCC(inst-0x70);
        } else if (inst >= 0x90 && inst < 0x98) {
            const int r = inst - 0x90;
            if (verbose) printf("XCHG AX, %s", RegName(r));
            int rv = reg[r];
            reg[r] = reg[R_AX];
            reg[R_AX] = rv;
        } else if (inst >= 0xb8 && inst < 0xc0) {
            const int r = inst-0xb8;
            reg[r] = Imm16();
            if (verbose) printf("MOV %s, 0x%04X", RegName(inst-0xb8), reg[r]);
        } else {
            switch (inst) {
            case 0x80:
            case 0x81:
            case 0x82:
            case 0x83:
                {
                    dsize = inst&1;
                    ModRM();
                    const int imm = inst == 0x81 ? Imm16() : (char)ReadIByte();
                    DoOp(modrm>>3&7, ReadRM(), imm);
                    if (verbose) printf("%s %s, %04X", OpName(modrm>>3&7), rmtext, imm);
                }
                break;
            case 0x88:
            case 0x89:
            case 0x8A:
            case 0x8B:
                dsize = inst&1;
                MOV(inst&2);
                break;
            case 0x8D:
                {
                    ModRM();
                    if (verbose) printf("LEA %s, %s", RegName(modrm>>3&7), rmtext);
                    reg[modrm>>3&7] = addr;
                }
                break;
            case 0x98:
                reg[R_AX] = (char)reg[R_AX];
                if (verbose) printf("CBW");
                break;
            case 0x99:
                reg[R_DX] = reg[R_AX]&0x8000 ? -1 : 0;
                if (verbose) printf("CWD");
                break;
            case 0xA0:
            case 0xA1:
            case 0xA2:
            case 0xA3:
                {
                    int Off = ReadIByte();
                    Off |= ReadIByte()<<8;
                    if (verbose) {
                        if (inst & 2) printf("MOV [0x%04X], A%c", Off, inst&1?'X':'L');
                        else          printf("MOV A%c, [0x%04X]", inst&1?'X':'L', Off);
                    }
                    if (inst & 2) {
                        // Store
                        if (inst & 1) {
                            Write16(SR_DS, Off, reg[R_AX]);
                        } else {
                            Write8(SR_DS, Off, reg[R_AX] & 0xff);
                        }
                    } else {
                        if (inst & 1) {
                            reg[R_AX] = Read16(SR_DS, Off);
                        } else {
                            reg[R_AX] = (reg[R_AX]&0xff00) | Read8(SR_DS, Off);
                        }
                    }
                }
                break;
            case 0xA4:
                {
                    assert(rep == 0xF3);
                    if (verbose) printf("REP MOVSB");
                    while (reg[R_CX]) {
                        Write8(SR_ES, reg[R_DI], Read8(SR_DS, reg[R_SI]));
                        reg[R_CX] -= 1;
                        reg[R_DI] += 1;
                        reg[R_SI] += 1;
                    }
                }
                break;
            case 0xA5:
                {
                    assert(rep == 0xF3);
                    if (verbose) printf("REP MOVSW");
                    while (reg[R_CX]) {
                        Write16(SR_ES, reg[R_DI], Read16(SR_DS, reg[R_SI]));
                        reg[R_CX] -= 1;
                        reg[R_DI] += 2;
                        reg[R_SI] += 2;
                    }
                }
                break;
            case 0xA6:
                {
                    assert(rep == 0xF3);
                    if (verbose) printf("REPE CMPSB");
                    flags = FZ;
                    while ((flags & FZ) && reg[R_CX]) {
                        DoOp(OP_CMP, Read8(SR_DS, reg[R_SI]), Read8(SR_ES, reg[R_DI]));
                        reg[R_CX] -= 1;
                        reg[R_DI] += 1;
                        reg[R_SI] += 1;
                    }
                }
                break;
            case 0xAA:
                {
                    assert(rep == 0xF3);
                    if (verbose) printf("REP STOSB");
                    const int v = reg[R_AX];
                    while (reg[R_CX]) {
                        Write8(SR_ES, reg[R_DI], v);
                        reg[R_CX] -= 1;
                        reg[R_DI] += 1;
                    }
                }
                break;
            case 0xC3:
                if (verbose) printf("RET");
                ip = DoPop();
                break;
            case 0xC6:
                {
                    dsize = 0;
                    ModRM();
                    int val = ReadIByte();
                    if (verbose) printf("MOV %s, 0x%04X", rmtext, val);
                    WriteRM(val);
                }
                break;
            case 0xC7:
                {
                    ModRM();
                    int val = Imm16();
                    if (verbose) printf("MOV %s, 0x%04X", rmtext, val);
                    WriteRM(val);
                }
                break;
            case 0xCD:
                INT();
                break;
            case 0xD1:
                {
                    ModRM();
                    int v = ReadRM();
                    // TODO: Handle flags
                    switch (modrm>>3&7) {
                    case 7: // SAR R/M16, 1
                        WriteRM(v >> 1);
                        break;
                    default:
                        Fatal("TODO: D1/%d", modrm>>3&7);
                    }
                }
                break;
            case 0xD3:
                {
                    ModRM();
                    int v = ReadRM();
                    // TODO: Handle flags
                    switch (modrm>>3&7) {
                    case 4: // SHL R/M16, CL
                        WriteRM(v << (reg[R_CX]&15));
                        break;
                    case 7: // SAR R/M16, CL
                        WriteRM(v >> (reg[R_CX]&15));
                        break;
                    default:
                        Fatal("TODO: D3/%d", modrm>>3&7);
                    }
                }
                break;
            case 0xE8:
                {
                    const int rel = Imm16();
                    if (verbose) printf("CALL 0x%04X", TRIM(ip+rel));
                    DoPush(ip);
                    ip += rel;
                }
                break;
            case 0xE9:
                JMP(Imm16());
                break;
            case 0xEB:
                JMP((char)ReadIByte());
                break;
            case 0xF7:
                {
                    ModRM();
                    // TODO: Flags
                    switch (modrm>>3&7) {
                    case 2: // NOT r/m16
                        WriteRM(~ReadRM());
                        break;
                    case 3: // NEG r/m16
                        WriteRM(-ReadRM());
                        break;
                    case 5: // IMUL (TODO: Full 32-bit result)
                        {
                            if (verbose) printf("IMUL %s", rmtext);
                            const int m = ReadRM();
                            reg[R_AX] *= m;
                            reg[R_DX] = 0;
                        }
                        break;
                    case 7: // IDIV (TODO: Full 32-bit divide)
                        {
                            if (verbose) printf("IDIV %s", rmtext);
                            const int d = ReadRM();
                            assert(reg[R_DX] == (reg[R_AX] & 0x8000 ? -1 : 0));
                            reg[R_DX] = reg[R_AX] % d;
                            reg[R_AX] /= d;
                        }
                        break;
                    default:
                        Fatal("TODO: F7/%X", modrm>>3&7);
                    }
                }
                break;
            case 0xFE:
                dsize = 0;
                // fallthrough
            case 0xFF:
                {
                    ModRM();
                    switch (modrm>>3&7) {
                    case 0:
                        INC();
                        break;
                    case 1:
                        DEC();
                        break;
                    default:
                        Fatal("TODO: %02X/%X", inst ,modrm>>3&7);
                    }
                }
                break;
            default:
                Fatal("Unimplemented/invalid instruction %02X", inst);
            }
        }
        if (verbose)
            PrintState();
#ifdef PROFILING
        total_cycles += cycles;
        counts[orig_ip] += cycles;
#endif
    }
}
