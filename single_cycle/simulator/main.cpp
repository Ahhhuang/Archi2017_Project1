#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <cstdlib>
#include <cstdio>

using namespace std;

int iMemory[256]={0}, dMemory[256]={0}, reg[32]={0}, regP[32] = {0}, pc = 0, hi = 0, lo = 0, hiP = 0, loP = 0;
int stop = 0, multOverflow = 0;
int cycleCur = 0;

void readLoadMemory(const char *, const char *);
void toBigEndian(int *, int *);
void reset();
void implement();
void error(int);

void rType(int);
void iType(int);
void jType(int);

FILE *f_sshot = fopen("snapshot_m.rpt", "w");
FILE *f_error = fopen("error_dump_m.rpt", "w");

int main()
{
    readLoadMemory("../hidden_testcase/error1/iimage.bin", "../hidden_testcase/error1/dimage.bin");
    implement();
    return 0;
}

void readLoadMemory(const char *iimage, const char *dimage) {
    int tem_i[300]={0}, tem_d[300]={0};

    //READ
    FILE *fl_i = fopen(iimage, "rb");
    FILE *fl_d = fopen(dimage, "rb");
    if(fl_i == NULL || fl_d == NULL)
        return;

    //find i's size
    fseek(fl_i, 0L, SEEK_END);
    long int sizeI = ftell(fl_i);
    fseek(fl_i, 0L, SEEK_SET);
    //find d's size
    fseek(fl_d, 0L, SEEK_END);
    long int sizeD = ftell(fl_d);
    fseek(fl_d, 0L, SEEK_SET);

    fread(tem_i, sizeI, 1, fl_i);
    fread(tem_d, sizeD, 1, fl_d);

    toBigEndian(tem_i, tem_d);
    //printf("%d = 0x%0.8x\n", i, tem_i[i]);

    reset();

    pc = tem_i[0];
    for(int i=pc/4; i<min(256, (pc/4)+tem_i[1]); i++){
        iMemory[i] = tem_i[i-(pc/4)+2];
        //printf("%d = 0x%0.8x\n", i, iMemory[i]);
    }
    regP[29] = reg[29] = tem_d[0];
    //reg[2] = 0x80000000;    reg[3] = 0x00000002;
    for(int i=0; i<min(256, tem_d[1]); i++){
        dMemory[i] = tem_d[i+2];
        //printf("%d = 0x%0.8x\n", i, dMemory[i]);
    }

    fclose(fl_i);
    fclose(fl_d);
}

void toBigEndian(int *i, int *d) {
    for(int j=0; j<256; j++){
      i[j] = ((i[j] << 24 ) & 0xFF000000 ) | ((i[j] << 8 ) & 0x00FF0000 ) | ((i[j] >> 8 ) & 0x0000FF00 ) | ((i[j] >> 24 ) & 0x000000FF);
      d[j] = ((d[j] << 24 ) & 0xFF000000 ) | ((d[j] << 8 ) & 0x00FF0000 ) | ((d[j] >> 8 ) & 0x0000FF00 ) | ((d[j] >> 24 ) & 0x000000FF);
    }
}

void reset() {
    multOverflow = cycleCur = stop = pc = hiP = hi = loP = lo = 0;
    for(int i=0; i<256; i++){
        iMemory[i] = dMemory[i] = 0;
    }
    for(int i=0; i<32; i++){
        regP[i] = reg[i] = 0;
    }
    error(0); //initialize error.rpt
}

void implement() {
    if(cycleCur == 0){
        fprintf(f_sshot, "cycle %d\n",cycleCur);
        //printf("cycle %d\n",cycleCur);
        for(int i=0; i<32; i++){
          fprintf(f_sshot, "$%02d: 0x%08X\n",i ,reg[i]);
          //printf("$%0.2d: 0x%0.8x\n",i ,reg[i] );
        }
        fprintf(f_sshot, "$HI: 0x%08X\n",hi);
        //printf("$HI: 0x%0.8x\n",hi );
        fprintf(f_sshot, "$LO: 0x%08X\n",lo);
        //printf("$LO: 0x%0.8x\n",lo );
        fprintf(f_sshot, "PC: 0x%08X\n",pc);
        //printf("$PC: 0x%0.8x\n",pc );
        fprintf(f_sshot, "\n\n");
    }
    while(1){
        cycleCur++;
        int instruction = iMemory[pc/4];
        int opcode = (instruction >> 26) & 0x3F;
        pc = pc + 4;

        if(opcode == 0x3F){
          break;
        }else if(opcode == 0x00){
          rType(instruction);
        }else if(opcode == 0x02 || opcode == 0x03){
          jType(instruction);
        }else{
          iType(instruction);
        }
        if(stop)
          break;
        fprintf(f_sshot, "cycle %d\n", cycleCur);
        //printf("cycle %d\n" , cycleCur);
        for(int i=0; i<32; i++){
            if(reg[i] != regP[i]){
                fprintf(f_sshot, "$%02d: 0x%08X\n",i ,reg[i]);
                //printf("$%0.2d: 0x%0.8x\n",i ,reg[i]);
            }
            regP[i] = reg[i];
        }
        if(hi != hiP){
            fprintf(f_sshot, "$HI: 0x%08X\n",hi);
            //printf("$HI: 0x%0.8x\n",hi );
            hiP = hi;
        }
        if(lo != loP){
            fprintf(f_sshot, "$LO: 0x%08X\n",lo);
            //printf("$LO: 0x%0.8x\n",lo );
            loP = lo;
        }
        fprintf(f_sshot, "PC: 0x%08X\n",pc);
        //printf("$PC: 0x%0.8x\n",pc );
        fprintf(f_sshot, "\n\n");
    }
    fclose(f_sshot);
    fclose(f_error);
}

void rType(int instr) {
    int rs = (instr >> 21) & 0x1F;
    int rt = (instr >> 16) & 0x1F;
    int rd = (instr >> 11) & 0x1F;
    int sh = (instr >> 6) & 0x1F;
    int fu = instr & 0x3F;

    // detect $0 overwrite
    if(rd == 0 && fu == 0x00){
        if(reg[rt] != 0 || sh != 0){
          error(1);
        }
    }else{
      if(rd == 0){
        if((fu != 0x08) && (fu != 0x18) && (fu != 0x19))
          error(1);
      }
    }
    //detect overwrite $hi and $lo
    if((fu == 0x18) || (fu == 0x19)){
        if(multOverflow){
            error(3);
        }
    }

    if(fu == 0x20){ // add
        int ans = reg[rs] + reg[rt];
        if(((reg[rs] >> 31) & 1) == ((reg[rt] >> 31) & 1)){
          if(((ans >> 31) & 1) != ((reg[rt] >> 31) & 1)){
              error(2);
          }
        }
        reg[rd] = ans;
    }else if(fu == 0x21){ // addu
        reg[rd] = reg[rs] + reg[rt];
    }else if(fu == 0x22){ // sub
        int ans = reg[rs] - reg[rt];
        if(((reg[rs] >> 31) & 1) != ((reg[rt] >> 31) & 1)){
          if(((ans >> 31) & 1) == ((reg[rt] >> 31) & 1)){
              error(2);
          }
        }
        reg[rd] = ans;
    }else if(fu == 0x24){ // and
        reg[rd] = reg[rs] & reg[rt];
    }else if(fu == 0x25){ // or
        reg[rd] = reg[rs] | reg[rt];
    }else if(fu == 0x26){ // xor
        reg[rd] = reg[rs] ^ reg[rt];
    }else if(fu == 0x27){ // nor
        reg[rd] = ~(reg[rs] | reg[rt]);
    }else if(fu == 0x28){ // nand
        reg[rd] = ~(reg[rs] & reg[rt]);
    }else if(fu == 0x2A){ // slt
        reg[rd] = (reg[rs] < reg[rt]) ? 1 : 0;
    }else if(fu == 0x00){ // sll
        reg[rd] = reg[rt] << sh;
    }else if(fu == 0x02){ // srl
        if(sh == 0){
            reg[rd] = reg[rt];
        }else if(((reg[rt] >> 31) & 1) == 0){
            reg[rd] = reg[rt] >> sh;
        }else{
            reg[rd] = (reg[rt] >> sh) - (0xFFFFFFFF << (32 - sh));
        }
    }else if(fu == 0x03){ // sra
        reg[rd] = reg[rt] >> sh;
    }else if(fu == 0x08){ // jr
        pc = reg[rs];
    }else if(fu == 0x18){ // mult
        multOverflow = 1;
        int64_t temp = (int64_t)(int32_t)reg[rs] * (int64_t)(int32_t)reg[rt];
        hi = (uint32_t)((temp >> 32) & 0xFFFFFFFF);
        lo = (uint32_t)(temp & 0xFFFFFFFF);
        if(((reg[rs] >> 31) & 1) == ((reg[rt] >> 31) & 1)){
            if(((hi >> 31) & 1) != 0){
                error(2);
            }
        }else{
            if(((hi >> 31) & 1) != 1){
                error(2);
            }
        }
        //printf("%0.8x | %0.8x = %0.8x + %0.8x",hi ,lo , reg[rs], (reg[rt]));        system("pause");
    }else if(fu == 0x19){ // multu
        multOverflow = 1;
        uint64_t temp = (uint64_t)(uint32_t)reg[rs] * (uint64_t)(uint32_t)reg[rt];
        hi = (uint32_t)((temp >> 32) & 0xFFFFFFFF);
        lo = (uint32_t)(temp & 0xFFFFFFFF);
        //printf("%0.8x | %0.8x = %0.8x + %0.8x",hi ,lo , reg[rs], (reg[rt]));        system("pause");
    }else if(fu == 0x10){ // mfhi
        multOverflow = 0;
        reg[rd] = hi;
    }else if(fu == 0x12){ // mflo
        multOverflow = 0;
        reg[rd] = lo;
    }else{
       cout << "get a wrong rtype!" << endl;
    }
    reg[0] = 0;
}

void iType(int instr) {
    int opcode = (instr >> 26) & 0x3f;
    int rs = (instr >> 21) & 0x1f;
    int rt = (instr >> 16) & 0x1f;
    int im = instr & 0xffff;
    //opcode = 0x08; rt = 2; rs = 3;
    if(opcode == 0x08){ // addi
        if(rt == 0) error(1);
        if((im >> 15) == 1){
            im = im | 0xffff0000;
        }
        int ans = reg[rs] + im;
        if(((im >> 31) & 1) == ((reg[rs] >> 31) & 1)){
            if(((im >> 31) & 1) != ((ans >> 31) & 1))
                error(2);
        }
        reg[rt] = ans;
    }else if(opcode == 0x09){ // addiu
        if(rt == 0) error(1);
        if((im >> 15) == 1){
            im = im | 0xffff0000;
        }
        reg[rt] = reg[rs] + im;
    }else if(opcode == 0x23){ //lw
        if(rt == 0) error(1);
        if((im >> 15) == 1) im = im | 0xffff0000;
        int memoryAddress = reg[rs] + im;
        //number overflow
        if(((im >> 31) & 1) == ((reg[rs] >> 31) & 1)){
            if(((im >> 31) & 1) != ((memoryAddress >> 31) & 1))
                error(2);
        }
        //address overflow
        if(memoryAddress < 0 || (memoryAddress + 4) > 1024){
            error(4);
        }
        //data misaligned
        if((memoryAddress % 4) != 0){
            error(5);
        }
        reg[rt] = dMemory[memoryAddress/4];
    }else if(opcode == 0x21){ //lh
        if(rt == 0) error(1);
        if((im >> 15) == 1) im = im | 0xffff0000;
        int memoryAddress = reg[rs] + im;
        //number overflow
        if(((im >> 31) & 1) == ((reg[rs] >> 31) & 1)){
            if(((im >> 31) & 1) != ((memoryAddress >> 31) & 1))
                error(2);
        }
        //address overflow
        if(memoryAddress < 0 || (memoryAddress + 2) > 1024){
            error(4);
        }
        //data misaligned
        if((memoryAddress % 2) != 0){
            error(5);
        }
        //detect it's first-half or second-half
        if((memoryAddress % 4) == 0){
            reg[rt] = dMemory[memoryAddress/4] >> 16;
        }else{ //signed
            if(((dMemory[memoryAddress/4] & 0x0000ffff) >> 15) == 1){
                reg[rt] = ((dMemory[memoryAddress/4] & 0x0000ffff) | 0xffff0000);
            }else{
                reg[rt] = (dMemory[memoryAddress/4] & 0x0000ffff);
            }
        }
    }else if(opcode == 0x25){ //lhu
        if(rt == 0) error(1);
        if((im >> 15) == 1) im = im | 0xffff0000;
        int memoryAddress = reg[rs] + im;
        //number overflow
        if(((im >> 31) & 1) == ((reg[rs] >> 31) & 1)){
            if(((im >> 31) & 1) != ((memoryAddress >> 31) & 1))
                error(2);
        }
        //address overflow
        if(memoryAddress < 0 || (memoryAddress + 2) > 1024){
            error(4);
        }
        //data misaligned
        if((memoryAddress % 2) != 0){
            error(5);
        }
        //detect it's first-half or second-half
        if((memoryAddress % 4) == 0){
            reg[rt] = ((dMemory[memoryAddress/4] >> 16) & 0x0000ffff);
        }else{ //unsigned
            reg[rt] = (dMemory[memoryAddress/4] & 0x0000ffff);
        }
    }else if(opcode == 0x20){ //lb
        if(rt == 0) error(1);
        if((im >> 15) == 1) im = im | 0xffff0000;
        int memoryAddress = reg[rs] + im;
        //number overflow
        if(((im >> 31) & 1) == ((reg[rs] >> 31) & 1)){
            if(((im >> 31) & 1) != ((memoryAddress >> 31) & 1))
                error(2);
        }
        //address overflow
        if(memoryAddress < 0 || memoryAddress > 1024){
            error(4);
        }
        //detect it's first-half or second-half or third...
        if((memoryAddress % 4) == 0){
            reg[rt] = dMemory[memoryAddress/4] >> 24;
        }else if(memoryAddress % 4 == 1){ //signed
            reg[rt] = dMemory[memoryAddress/4] << 8 >> 24;
        }
        else if(memoryAddress % 4 == 2){
            reg[rt] = dMemory[memoryAddress/4] << 16 >> 24;
        }else{
            reg[rt] = dMemory[memoryAddress/4] << 24 >> 24;
        }
    }else if(opcode == 0x24){ //lbu
        if(rt == 0) error(1);
        if((im >> 15) == 1) im = im | 0xffff0000;
        int memoryAddress = reg[rs] + im;
        //number overflow
        if(((im >> 31) & 1) == ((reg[rs] >> 31) & 1)){
            if(((im >> 31) & 1) != ((memoryAddress >> 31) & 1))
                error(2);
        }
        //address overflow
        if(memoryAddress < 0 || memoryAddress > 1024){
            error(4);
        }
        //detect it's first-half or second-half or third...
        if((memoryAddress % 4) == 0){
            reg[rt] = ((dMemory[memoryAddress/4] >> 24) & 0x000000ff);
        }else if((memoryAddress % 4) == 1){ //signed
            reg[rt] = ((dMemory[memoryAddress/4] >> 16) & 0x000000ff);
        }
        else if((memoryAddress % 4) == 2){
            reg[rt] = ((dMemory[memoryAddress/4] >> 8) & 0x000000ff);
        }else{
            reg[rt] = (dMemory[memoryAddress/4] & 0x000000ff);
        }
    }else if(opcode == 0x2B){ //sw
        if((im >> 15) == 1) im = im | 0xffff0000;
        int memoryAddress = reg[rs] + im;
        //number overflow
        if(((im >> 31) & 1) == ((reg[rs] >> 31) & 1)){
            if(((im >> 31) & 1) != ((memoryAddress >> 31) & 1))
                error(2);
        }
        //address overflow
        if(memoryAddress < 0 || (memoryAddress + 4) > 1024){
            error(4);
        }
        //data misaligned
        if((memoryAddress % 4) != 0){
            error(5);
        }
        dMemory[memoryAddress/4] = reg[rt];
    }else if(opcode == 0x29){ //sh
        if((im >> 15) == 1) im = im | 0xffff0000;
        int memoryAddress = reg[rs] + im;
        //number overflow
        if(((im >> 31) & 1) == ((reg[rs] >> 31) & 1)){
            if(((im >> 31) & 1) != ((memoryAddress >> 31) & 1))
                error(2);
        }
        //address overflow
        if(memoryAddress < 0 || (memoryAddress + 2) > 1024){
            error(4);
        }
        //data misaligned
        if((memoryAddress % 2) != 0){
            error(5);
        }
        if((memoryAddress % 4) == 0){
            dMemory[memoryAddress/4] = (((reg[rt] & 0x0000ffff) << 16) | (dMemory[memoryAddress/4] & 0x0000ffff));
        }else{
            dMemory[memoryAddress/4] = ((reg[rt] & 0x0000ffff) | (dMemory[memoryAddress/4] & 0xffff0000));
        }
    }else if(opcode == 0x28){ //sb
        if((im >> 15) == 1) im = im | 0xffff0000;
        int memoryAddress = reg[rs] + im;
        //number overflow
        if(((im >> 31) & 1) == ((reg[rs] >> 31) & 1)){
            if(((im >> 31) & 1) != ((memoryAddress >> 31) & 1))
                error(2);
        }
        //address overflow
        if(memoryAddress < 0 || memoryAddress > 1024){
            error(4);
        }
        if((memoryAddress % 4) == 0){
            dMemory[memoryAddress/4] = (((reg[rt] & 0x000000ff) << 24) | (dMemory[memoryAddress/4] & 0x00ffffff));
        }else if((memoryAddress % 4) == 1){
            dMemory[memoryAddress/4] = (((reg[rt] & 0x000000ff) << 16) | (dMemory[memoryAddress/4] & 0xff00ffff));
        }else if((memoryAddress % 4) == 2){
            dMemory[memoryAddress/4] = (((reg[rt] & 0x000000ff) << 8) | (dMemory[memoryAddress/4] & 0xffff00ff));
        }else{
            dMemory[memoryAddress/4] = ((reg[rt] & 0x000000ff) | (dMemory[memoryAddress/4] & 0xffffff00));
        }
    }else if(opcode == 0x0F){ //lui
        if((im >> 15) == 1) im = im | 0xffff0000;
        if(rt == 0) error(1);
        reg[rt] = im << 16;
    }else if(opcode == 0x0C){ //andi
        if(rt == 0) error(1);
        reg[rt] = reg[rs] & im;
    }else if(opcode == 0x0D){ //ori
        if(rt == 0) error(1);
        reg[rt] = reg[rs] | im;
    }else if(opcode == 0x0E){ //nori
        if(rt == 0) error(1);
        reg[rt] = ~(reg[rs] | im);
    }else if(opcode == 0x0A){ //slti
        if(rt == 0) error(1);
        if((im >> 15) == 1) im = im | 0xffff0000;
        reg[rt] = (reg[rs] < im) ? 1 : 0;
    }else if(opcode == 0x04){ //beq
        if((im >> 15) == 1) im = im | 0xffff0000;
        if(reg[rs] == reg[rt]){
            pc = pc + (im << 2);
        }
    }else if(opcode == 0x05){ //bne
        if((im >> 15) == 1) im = im | 0xffff0000;
        if(reg[rs] != reg[rt]){
            pc = pc + (im << 2);
        }
    }else if(opcode == 0x07){ //bgtz
        if((im >> 15) == 1) im = im | 0xffff0000;
        if(reg[rs] > 0){
            pc = pc + (im << 2);
        }
    }else{
        cout << "get a wrong iType!" << endl;
    }
    reg[0] = 0;
}

void jType(int instr) {
    int opcode = (instr >> 26) & 0x3F;
    int address = instr & 0x3FFFFFF;


    if(opcode == 0x03) //jal
        reg[31] = pc;
    pc = (pc & 0xf0000000) | (address << 2);
}

void error(int type) {
    if(type == 0){
        return;
    }else if(type == 1){
        fprintf(f_error, "In cycle %d: Write $0 Error\n", cycleCur);
        //printf("In cycle %d: Write $0 Error\n", cycleCur);
    }else if(type == 2){
        fprintf(f_error, "In cycle %d: Number Overflow\n", cycleCur);
        //printf("In cycle %d: Number Overflow\n", cycleCur);
    }else if(type == 3){
        fprintf(f_error, "In cycle %d: Overwrite HI-LO registers\n", cycleCur);
        //printf("In cycle %d: Overwrite HI-LO registers\n", cycleCur);
    }else if(type == 4){
        fprintf(f_error, "In cycle %d: Address Overflow\n", cycleCur);
        //printf("In cycle %d: Address Overflow\n", cycleCur);
        stop = 1;
    }else if(type == 5){
        fprintf(f_error, "In cycle %d: Misalignment Error\n", cycleCur);
        //printf("In cycle %d: Misalignment Error\n", cycleCur);
        stop = 1;
    }else{
        cout << "wrong error type!" << endl;
    }
}
