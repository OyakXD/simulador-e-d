#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MEM_SIZE 256

typedef struct {
    uint16_t R[8];
    uint16_t PC;
    uint16_t IR;
    uint16_t SP;
    uint8_t FLAGS;
} CPU;

uint16_t memory[MEM_SIZE];

// Função para carregar arquivo <endereço>:<conteúdo>
void load_archive(const char *filename){
    FILE *file = fopen(filename, "r");

    if(!file){
        perror("Erro ao abrir o arquivo");
        exit(1);
    }

    uint16_t address, value;
    while(fscanf(file, "%hx: %hx", &address, &value) == 2){
        if(address < MEM_SIZE){
            memory[address / 2] = value;
            printf("Mem[%04X] = 0x%04X\n", address, value);
        } else {
            printf("Erro: Endereço %04X fora do limite da memória!\n", address);
        }
    }

    fclose(file);
}

void execute_mov_immediate(CPU *cpu, uint16_t instruction){
    uint8_t rd = (instruction >> 8) & 0x07;
    uint8_t imm = instruction & 0xFF;
    cpu->R[rd] = imm;
}

void execute (CPU *cpu) {
    while(cpu->PC < MEM_SIZE){
        cpu->IR = memory[cpu->PC / 2]; // Busca
        printf("Executando: 0x%04X no endereço: %04X\n", cpu->IR, cpu->PC);

        uint16_t opcode = (cpu->IR >> 12) & 0xF;
        switch(opcode){
            case 0x1: execute_mov_immediate(cpu, cpu->IR); break;
            case 0xF: printf("HALT detectado.\n"); return;
            default: printf("Instrução desconhecida!\n"); break;
        }

        cpu->PC += 2;
    }
}

int main() {
    CPU cpu = {0}; // Inicializa zerada
    load_archive("program.txt");

    cpu.PC = 0;
    execute(&cpu);
}