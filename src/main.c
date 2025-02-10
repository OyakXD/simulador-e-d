#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_MEMORY 1024
#define NUM_REGISTERS 8
#define MAX_STACK 256

typedef struct {
  uint16_t registers[NUM_REGISTERS];
  uint16_t memory[MAX_MEMORY];
  uint16_t pc;
  uint16_t stack[MAX_STACK];
  int16_t sp;

  // Flags de condição
  struct {
    uint8_t zero;
    uint8_t carry;
    uint8_t negative;
    uint8_t overflow;
  } flags;
} CPU;

int is_valid_register(uint16_t reg){
    return (reg < NUM_REGISTERS);
}

void init_simulator(CPU *cpu) {
    
    // Zeradores
    memset(cpu->registers, 0, sizeof(cpu->registers));
    memset(cpu->memory, 0, sizeof(cpu->memory));
    memset(cpu->stack, 0 , sizeof(cpu->stack));
    // ------------

    cpu->pc = 0;
    cpu->sp = -1;
    cpu->flags.zero = 0;
    cpu->flags.carry = 0;
    cpu->flags.negative = 0;
    cpu->flags.overflow = 0;
}

int load_program(CPU *cpu, const char *filename){
    FILE *file = fopen(filename, "r");

    if(file == NULL){
        printf("Erro ao abrir o arquivo.\n");
        return 0;
    }

    char line[50];
    uint16_t address, content;

    memset(cpu->memory, 0, sizeof(cpu->memory));

    while(fgets(line, sizeof(line), file)){

        if(strncmp(line, "0x", 2) == 0 || line[0] == '0'){
            if(sscanf(line, "%hx: 0x%hx", &address, &content) == 2){
                cpu->memory[address / 2] = content;
            }
        }
    }

    fclose(file);
    return 1;
}

void push(CPU *cpu, uint16_t value) {
    if(cpu->sp < MAX_STACK - 1){
        cpu->sp++;
        cpu->stack[cpu->sp] = value;
    } else {
        printf("Erro: Pilha cheia\n");
    }
}

uint16_t pop(CPU *cpu){
    if(cpu->sp >= 0){
        uint16_t value = cpu->stack[cpu->pc];
        cpu->sp--;
        return value;
    } else {
        printf("Erro: Pilha Vazia\n");
        return 0;
    }
}

void execute_instruction(CPU *cpu, uint16_t instruction){

    // Decodifica a instrução
    uint16_t opcode = (instruction >> 12) & 0xF; // Bits 15-12
    uint16_t rd = (instruction >> 8) & 0x7; // Bits 11-9 (0-7)
    uint16_t rm = (instruction >> 4) & 0x7; // Bits 7-5 (0-7)
    uint16_t rn = instruction & 0x7; // Bits 2-0 (0-7)
    uint16_t immediate = instruction & 0xFF; // Valor imediato

    if(!is_valid_register(rd) || !is_valid_register(rm) || !is_valid_register(rn)){
        printf("Erro: Registrador inválido na instrução 0x%04x\n", instruction);
        return;
    }

    switch(opcode) {
        case 0x1: // MOV
            if((instruction >> 11) & 0x1) { // Se for MOV imediato
                cpu->registers[rd] = immediate;
            } else {
                cpu->registers[rd] = cpu->registers[rm];
            }
            break;
        case 0x4: // ADD
            cpu->registers[rd] = cpu->registers[rm] + cpu->registers[rn];
            cpu->flags.zero = (cpu->registers[rd] == 0);
            break;
        case 0x5: // SUB
            cpu->registers[rd] = cpu->registers[rm] - cpu->registers[rn];
            cpu->flags.zero = (cpu->registers[rd] == 0);
            break;
        case 0x0: // PUSH/POP
            if((instruction >> 8) & 0x7) { // POP
                pop(cpu);
            } else {
                push(cpu, rd);
            }
            break;
        default:
            printf("Instrução não implementada: 0x%04x\n", instruction);
            break;
    }
    
}

void run_program(CPU *cpu){
    while(cpu->pc < MAX_MEMORY && cpu->memory[cpu->pc] != 0){
        uint16_t instruction = cpu->memory[cpu->pc];
        execute_instruction(cpu, instruction);
        cpu->pc++;
    }
}

void print_simulator_state(CPU *cpu){
    printf("\nEstado do simulador:\n");

    // Imprimir os registradores
    for(int i = 0; i < NUM_REGISTERS; i++){
        printf("R%d: 0x%04x ", i, cpu->registers[i]);
    }

    // Imprime o contador do programa
    printf("\nPC: 0x%04x\n", cpu->pc);

    // Imprime a flag zero
    printf("\nZero flag: %d\n", cpu->flags.zero);

    printf("\nPonteiro da pilha (SP): %d\n", cpu->sp);
    if(cpu->sp >= 0){
        printf("Topo da pilha: 0x%04x\n", cpu->stack[cpu->sp]);
    }
}

int main() {
    CPU cpu;

    init_simulator(&cpu);

    if(!load_program(&cpu, "program.txt")){
        return 1;
    }

    run_program(&cpu);

    print_simulator_state(&cpu);

    return 0;
}