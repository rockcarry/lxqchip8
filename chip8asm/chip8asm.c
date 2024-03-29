#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
00E0  CLR
00EE  RET
1NNN  JMP   NNN
2NNN  CALL  NNN
3XNN  SEQ   VX NN
4XNN  SNE   VX NN
5XY0  SEQ   VX VY
6XNN  MOV   VX NN
7XNN  ADD   VX NN
8XY0  MOV   VX VY
8XY1  OR    VX VY
8XY2  AND   VX VY
8XY3  XOR   VX VY
8XY4  ADD   VX VY
8XY5  SUB   VX VY
8XY6  SHR   VX VY
8XY7  NSUB  VX VY
8XYE  SHL   VX VY
9XY0  SNE   VX VY
ANNN  MOV   I  NNN
BNNN  JV0   NNN
CXNN  RAND  VX NN
DXYN  DRAW  VX VY N
EX9E  SKD   VX
EXA1  SKU   VX
FX07  GTM   VX
FX0A  GKEY  VX
FX15  STM   VX
FX18  BEEP  VX
FX1E  ADD   I  VX
FX29  IM5   VX
FX33  BCD   VX
FX55  STORE VX
FX65  LOAD  VX
 */

#ifdef _MSC_VER
#pragma warning(disable:4996)
#define strcasecmp stricmp
#endif

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#define MAX_ADDRESS  0x1000
#define MAX_ASM_NUM (MAX_ADDRESS / 2)
#define MAX_STR_NUM (MAX_ADDRESS / 2)
#define OPCODE_FOURCC(a, b, c) (((a) << 16) | ((b) << 8) | ((c) << 0))

typedef struct {
    uint16_t line;
    uint16_t addr;
    uint16_t opcode;
    uint16_t stridx;
} ASM_ITEM;

typedef struct {
    char     name[32];
    uint16_t line;
    uint16_t addr;
} STR_ITEM;

static int strisdigit(char *str)
{
    if (*str == '+' || *str == '-') str++;
    while (*str) {
        if (*str < '0' || *str > '9') return 0;
        str++;
    }
    return 1;
}

static char* parse_label(char *str)
{
    int len;
    if (!str) return NULL;
    len = (int)strlen(str);
    if (len < 2 || str[len - 1] != ':') return NULL;
    return (strstr(str, "0x") != str && strstr(str, "0X") != str) ? str : NULL;
}

static int parse_addr(char *str)
{
    int addr = 0;
    if (!str || (strstr(str, "0x") != str && strstr(str, "0X") != str) || str[strlen(str) - 1] != ':') return -1;
    (void)sscanf(str, "%x", &addr);
    return addr;
}

static int parse_opcode(char *str)
{
    static const char *OPCODE_TAB[] = {
        "CLR", "RET"  , "JMP" , "CALL", "SEQ" , "SNE", "MOV", "ADD", "OR"  , "AND", "XOR" ,
        "SUB", "SHR"  , "SHL" , "RAND", "DRAW", "SKD", "SKU", "GTM", "GKEY", "STM", "BEEP",
        "BCD", "STORE", "LOAD", "JV0" , "IM5" , NULL
    };
    int  code, i;
    if (!str) return -1;
    for (i=0; OPCODE_TAB[i]; i++) {
        if (strcasecmp(str, OPCODE_TAB[i]) == 0) break;
    }
    if (!OPCODE_TAB[i]) {
        if (strstr(str, "0x") != str && strstr(str, "0X") != str) return -1;
        (void)sscanf(str, "%x", &code);
        code &= 0xFFFF;
    } else code = OPCODE_FOURCC(OPCODE_TAB[i][0], OPCODE_TAB[i][1], OPCODE_TAB[i][2]);
    return code;
}

static int parse_reg(char *str)
{
    if (!str || (str[0] != 'V' && str[0] != 'v') || strlen(str) != 2) return -1;
    if ((str[1] >= 'A' && str[1] <= 'Z') || (str[1] >= 'a' && str[1] <= 'z') || (str[1] >= '0' && str[1] <= '9')) {
        int n; (void)sscanf(str + 1, "%x", &n);
        return n;
    } else return -1;
}

static int parse_imm(char *str)
{
    int imm = -1;
    if (!str) return -1;
    if (strstr(str, "0x") == str || strstr(str, "0X") == str) {
        (void)sscanf(str, "%x", &imm); imm &= 0xFFF;
    } else if (strisdigit(str)) {
        (void)sscanf(str, "%d", &imm); imm &= 0xFFF;
    }
    return imm;
}

static int str_find(STR_ITEM *strlst, int strnum, char *strname)
{
    char tmp[32];
    int  len, i;
    strncpy(tmp, strname, sizeof(tmp)); tmp[sizeof(tmp) - 1] = '\0';
    len = (int)strlen(tmp);
    if (len > 0 && tmp[len - 1] == ':') tmp[len - 1] = '\0';
    for (i=1; i<strnum; i++) {
        if (strcasecmp(strlst[i].name, tmp) == 0) return i;
    }
    return -1;
}

static int str_add(STR_ITEM *strlst, int *strnum, char *strname, uint16_t addr, uint16_t line)
{
    char tmp[32];
    int  len;

    if (*strnum >= MAX_STR_NUM) return -1;
    strncpy(tmp, strname, sizeof(tmp)); tmp[sizeof(tmp) - 1] = '\0';
    len = (int)strlen(tmp);
    if (len > 0 && tmp[len - 1] == ':') tmp[len - 1] = '\0';

    strncpy(strlst[*strnum].name, tmp, sizeof(strlst[*strnum].name));
    strlst[*strnum].addr = addr;
    strlst[*strnum].line = line;
    return (*strnum)++;
}

static int asm_add(ASM_ITEM *asmlst, int *asmnum, uint16_t line, uint16_t addr, uint16_t opcode, uint16_t stridx)
{
    if (*asmnum >= MAX_ASM_NUM) return -1;
    asmlst[*asmnum].line   = line;
    asmlst[*asmnum].addr   = addr;
    asmlst[*asmnum].opcode = opcode;
    asmlst[*asmnum].stridx = stridx;
    return (*asmnum)++;
}

int main(int argc, char *argv[])
{
    char    *asmfile = "input.asm";
    char    *binfile = "rom.bin";
    FILE    *fpin    = NULL;
    FILE    *fpout   = NULL;
    int      curaddr = 0x200;
    int      maxaddr = 0x200;
    int      curline = 1;
    int      asm_num = 0;
    int      str_num = 1;
    int      i;
    static ASM_ITEM asm_tab[MAX_ASM_NUM] = {0};
    static STR_ITEM str_tab[MAX_STR_NUM] = {0};
    static uint8_t  rom_buf[MAX_ADDRESS] = {0};

    if (argc >= 2) asmfile = argv[1];
    if (argc >= 3) binfile = argv[2];
    printf("input  file: %s\n", asmfile);
    printf("output file: %s\n", binfile);
    fpin  = fopen(asmfile, "rb");
    fpout = fopen(binfile, "wb");
    if (!fpin || !fpout) {
        printf("failed to open in/out file !\n");
        goto done;
    }

    while (!feof(fpin)) {
        char  buffer[256], token[5][32] = {0}, *opcode = token[1], *opnd1 = token[2], *opnd2 = token[3], *opnd3 = token[4];
        int   code, tmp1, tmp2, tmp3, idx;
        if (fgets(buffer, sizeof(buffer), fpin) != NULL) {
            for (i=0; buffer[i]; i++) buffer[i] = buffer[i] == ',' ? ' ' : buffer[i]; // replace ',' to ' '
            (void)sscanf(buffer, "%32s %32s %32s %32s %32s", token[0], token[1], token[2], token[3], token[4]); // read token
            if (parse_label(token[0])) { // is label
                idx = str_find(str_tab, str_num, token[0]);
                if (idx != -1) {
                    if (str_tab[idx].line == 0) {
                        str_tab[idx].addr = curaddr;
                        str_tab[idx].line = curline;
                    } else {
                        printf("%s (%d), error: label '%s' already defined in line %d !\n", asmfile, curline, token[0], str_tab[idx].line);
                    }
                } else {
                    str_add(str_tab, &str_num, token[0], curaddr, curline);
                }
            } else if (parse_addr(token[0]) >= 0) { // is address
                curaddr = parse_addr(token[0]);
            } else {
                opcode  = token[0]; opnd1 = token[1]; opnd2 = token[2]; opnd3 = token[3];
            }
            code = parse_opcode(opcode);
            if (code > 0xFFFF) {
                switch (code) {
                case OPCODE_FOURCC('C', 'L', 'R'): asm_add(asm_tab, &asm_num, curline, curaddr, 0x00E0, 0); break;
                case OPCODE_FOURCC('R', 'E', 'T'): asm_add(asm_tab, &asm_num, curline, curaddr, 0x00EE, 0); break;
                case OPCODE_FOURCC('J', 'M', 'P'):
                case OPCODE_FOURCC('C', 'A', 'L'):
                case OPCODE_FOURCC('J', 'V', '0'):
                    tmp1 = parse_imm(opnd1);
                    if (tmp1 < 0) { // not imm
                        tmp1= 0;
                        idx = str_find(str_tab, str_num, opnd1);
                        if (idx < 0) idx = str_add(str_tab, &str_num, opnd1, 0, 0);
                    } else idx = 0;
                    if      (code == OPCODE_FOURCC('J', 'M', 'P')) code = 0x1000;
                    else if (code == OPCODE_FOURCC('C', 'A', 'L')) code = 0x2000;
                    else if (code == OPCODE_FOURCC('J', 'V', '0')) code = 0xB000;
                    asm_add(asm_tab, &asm_num, curline, curaddr, code | (tmp1 & 0xFFF), idx);
                    break;
                case OPCODE_FOURCC('S', 'E', 'Q'):
                    if ((tmp1 = parse_reg(opnd1)) <  0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    if ((tmp2 = parse_imm(opnd2)) >= 0) {
                        asm_add(asm_tab, &asm_num, curline, curaddr, 0x3000 | ((tmp1 & 0xF) << 8) | (tmp2 & 0xFF), 0);
                    } else {
                        if ((tmp2 = parse_reg(opnd2)) >= 0) {
                            asm_add(asm_tab, &asm_num, curline, curaddr, 0x5000 | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4), 0);
                        } else {
                            printf("%s (%d), error: %s is not a imm or a reg !\n", asmfile, curline, opnd2);
                        }
                    }
                    break;
                case OPCODE_FOURCC('S', 'N', 'E'):
                    if ((tmp1 = parse_reg(opnd1)) <  0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    if ((tmp2 = parse_imm(opnd2)) >= 0) {
                        asm_add(asm_tab, &asm_num, curline, curaddr, 0x4000 | ((tmp1 & 0xF) << 8) | (tmp2 & 0xFF), 0);
                    } else {
                        if ((tmp2 = parse_reg(opnd2)) >= 0) {
                            asm_add(asm_tab, &asm_num, curline, curaddr, 0x9000 | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4), 0);
                        } else {
                            printf("%s (%d), error: %s is not a imm or a reg !\n", asmfile, curline, opnd2);
                        }
                    }
                    break;
                case OPCODE_FOURCC('M', 'O', 'V'):
                    if (strcasecmp(opnd1, "I") == 0) {
                        if ((tmp2 = parse_imm(opnd2)) >= 0) {
                            asm_add(asm_tab, &asm_num, curline, curaddr, 0xA000 | (tmp2 & 0xFFF), 0);
                        } else {
                            idx = str_find(str_tab, str_num, opnd2);
                            if (idx < 0) {
                                idx = str_add(str_tab, &str_num, opnd2, 0, 0);
                                asm_add(asm_tab, &asm_num, curline, curaddr, 0xA000, idx);
                            } else {
                                asm_add(asm_tab, &asm_num, curline, curaddr, 0xA000 | (str_tab[idx].addr & 0xFFFF), idx);
                            }
                        }
                    } else {
                        if ((tmp1 = parse_reg(opnd1)) <  0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                        if ((tmp2 = parse_imm(opnd2)) >= 0) {
                            asm_add(asm_tab, &asm_num, curline, curaddr, 0x6000 | ((tmp1 & 0xF) << 8) | (tmp2 & 0xFF), 0);
                        } else {
                            if ((tmp2 = parse_reg(opnd2)) >= 0) {
                                asm_add(asm_tab, &asm_num, curline, curaddr, 0x8000 | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4), 0);
                            } else {
                                printf("%s (%d), error: %s is not a imm or a reg !\n", asmfile, curline, opnd2);
                            }
                        }
                    }
                    break;
                case OPCODE_FOURCC('A', 'D', 'D'):
                    if (strcasecmp(opnd1, "I") == 0) {
                        if ((tmp2 = parse_reg(opnd2)) <  0) {
                            printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd2);
                        } else {
                            asm_add(asm_tab, &asm_num, curline, curaddr, 0xF01E | ((tmp2 & 0xF) << 8), 0);
                        }
                    } else {
                        if ((tmp1 = parse_reg(opnd1)) <  0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                        if ((tmp2 = parse_imm(opnd2)) >= 0) {
                            asm_add(asm_tab, &asm_num, curline, curaddr, 0x7000 | ((tmp1 & 0xF) << 8) | (tmp2 & 0xFF), 0);
                        } else {
                            if ((tmp2 = parse_reg(opnd2)) >= 0) {
                                asm_add(asm_tab, &asm_num, curline, curaddr, 0x8004 | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4), 0);
                            } else {
                                printf("%s (%d), error: %s is not a imm or a reg !\n", asmfile, curline, opnd2);
                            }
                        }
                    }
                    break;
                case OPCODE_FOURCC('O', 'R', '\0'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    if ((tmp2 = parse_reg(opnd2)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd2);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0x8001 | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4), 0);
                    break;
                case OPCODE_FOURCC('A', 'N', 'D'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    if ((tmp2 = parse_reg(opnd2)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd2);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0x8002 | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4), 0);
                    break;
                case OPCODE_FOURCC('X', 'O', 'R'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    if ((tmp2 = parse_reg(opnd2)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd2);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0x8003 | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4), 0);
                    break;
                case OPCODE_FOURCC('S', 'U', 'B'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    if ((tmp2 = parse_reg(opnd2)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd2);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0x8005 | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4), 0);
                    break;
                case OPCODE_FOURCC('S', 'H', 'R'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    if ((tmp2 = parse_reg(opnd2)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd2);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0x8006 | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4), 0);
                    break;
                case OPCODE_FOURCC('M', 'S', 'U'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    if ((tmp2 = parse_reg(opnd2)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd2);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0x8007 | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4), 0);
                    break;
                case OPCODE_FOURCC('S', 'H', 'L'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    if ((tmp2 = parse_reg(opnd2)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd2);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0x800E | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4), 0);
                    break;
                case OPCODE_FOURCC('R', 'A', 'N'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    if ((tmp2 = parse_imm(opnd2)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd2);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xC000 | ((tmp1 & 0xF) << 8) | (tmp2 & 0xFF), 0);
                    break;
                case OPCODE_FOURCC('D', 'R', 'A'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    if ((tmp2 = parse_reg(opnd2)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd2);
                    if ((tmp3 = parse_imm(opnd3)) < 0) printf("%s (%d), error: %s is not a imm !\n", asmfile, curline, opnd3);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xD000 | ((tmp1 & 0xF) << 8) | ((tmp2 & 0xF) << 4) | (tmp3 & 0xF), 0);
                    break;
                case OPCODE_FOURCC('S', 'K', 'D'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xE09E | ((tmp1 & 0xF) << 8), 0);
                    break;
                case OPCODE_FOURCC('S', 'K', 'U'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xE0A1 | ((tmp1 & 0xF) << 8), 0);
                    break;
                case OPCODE_FOURCC('G', 'T', 'M'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xF007 | ((tmp1 & 0xF) << 8), 0);
                    break;
                case OPCODE_FOURCC('G', 'K', 'E'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xF00A | ((tmp1 & 0xF) << 8), 0);
                    break;
                case OPCODE_FOURCC('S', 'T', 'M'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xF015 | ((tmp1 & 0xF) << 8), 0);
                    break;
                case OPCODE_FOURCC('B', 'E', 'E'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xF018 | ((tmp1 & 0xF) << 8), 0);
                    break;
                case OPCODE_FOURCC('I', 'M', '5'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xF029 | ((tmp1 & 0xF) << 8), 0);
                    break;
                case OPCODE_FOURCC('B', 'C', 'D'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xF033 | ((tmp1 & 0xF) << 8), 0);
                    break;
                case OPCODE_FOURCC('S', 'T', 'O'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xF055 | ((tmp1 & 0xF) << 8), 0);
                    break;
                case OPCODE_FOURCC('L', 'O', 'A'):
                    if ((tmp1 = parse_reg(opnd1)) < 0) printf("%s (%d), error: %s is not a reg !\n", asmfile, curline, opnd1);
                    asm_add(asm_tab, &asm_num, curline, curaddr, 0xF065 | ((tmp1 & 0xF) << 8), 0);
                    break;
                }
                curaddr += 2;
            } else if (code >= 0) {
                asm_add(asm_tab, &asm_num, curline, curaddr, code, 0);
                curaddr += 2;
            } else {
                if (strlen(opcode) > 0 && strstr(opcode, ";") != opcode) {
                    printf("%s (%d), error: unsupported opcode: %s !\n", asmfile, curline, opcode);
                }
            }
            if (maxaddr < curaddr) maxaddr = curaddr;
            curline++;
        }
    }

    for (i=0; i<asm_num; i++) {
        if (asm_tab[i].stridx) {
            if (str_tab[asm_tab[i].stridx].addr) {
                asm_tab[i].opcode|= str_tab[asm_tab[i].stridx].addr & 0xFFF;
                asm_tab[i].stridx = 0;
            } else {
                printf("%s (%d), error: unfixed string name %s !\n", asmfile, asm_tab[i].line, str_tab[asm_tab[i].stridx].name);
            }
        }
        if (asm_tab[i].addr + 1 < MAX_ADDRESS) {
            rom_buf[asm_tab[i].addr + 0] = (uint8_t)(asm_tab[i].opcode >> 8);
            rom_buf[asm_tab[i].addr + 1] = (uint8_t)(asm_tab[i].opcode >> 0);
        } else {
            printf("%s (%d), error: address out of range 0x%04X !\n", asmfile, asm_tab[i].line, asm_tab[i].addr);
        }
    }
    for (i = 0x200; i < maxaddr && i < sizeof(rom_buf); i++) {
        fputc(rom_buf[i], fpout);
    }
    printf("assemble finish.\n");

done:
    if (fpin ) fclose(fpin );
    if (fpout) fclose(fpout);
    return 0;
}

