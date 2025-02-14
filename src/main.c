#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define MAX_MEMORY 1024
#define NUM_REGISTERS 8
#define MAX_STACK 256

typedef struct {
  uint16_t registers[NUM_REGISTERS];
  uint16_t memory[MAX_MEMORY];
  uint16_t pc;
  uint16_t stack[MAX_STACK];
  int16_t sp;
  int16_t iR;

  // Flags de condição
  struct {
    uint8_t zero;
    uint8_t carry;
    uint8_t negative;
    uint8_t overflow;
  } flags;
} CPU;

// Validação de registrador
int is_valid_register(uint16_t reg){
    return (reg < NUM_REGISTERS); // Retorna 1 se válido (0-7), 0 se inválido
}

// Função que inicia o simulador
void init_simulator(CPU *cpu) {
    
    // Zeradores
    memset(cpu->registers, 0, sizeof(cpu->registers));
    memset(cpu->memory, 0, sizeof(cpu->memory));
    memset(cpu->stack, 0 , sizeof(cpu->stack));
    // ------------

    cpu->pc = 0;
    cpu->sp = MAX_MEMORY - 2;
    cpu->flags.zero = 0;
    cpu->flags.carry = 0;
    cpu->flags.negative = 0;
    cpu->flags.overflow = 0;
}

// Função para carregar o arquivo <endereço>:<conteúdo>
int load_program(CPU *cpu, const char *filename){
    FILE *file = fopen(filename, "r");

    if(file == NULL){
        printf("Erro ao abrir o arquivo.\n");
        return 0;
    }

    char line[50];
    uint16_t address, content;

    // Preencher com halt
    memset(cpu->memory, 0xFF, sizeof(cpu->memory));

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

// Função para printar o estado da CPU
void print_simulator_state(CPU *cpu){
    printf("\nEstado do simulador:\n");

    // Imprimir os registradores
    for(int i = 0; i < NUM_REGISTERS; i++){
        printf("R%d: 0x%04x ", i, cpu->registers[i]);
    }

    // Imprime o contador do programa
    printf("\nPC: 0x%04x\n", cpu->pc);

    printf("\nPonteiro da pilha (SP): %d\n", cpu->sp);
    if(cpu->sp >= 0){
        printf("Topo da pilha: 0x%04x\n", cpu->stack[cpu->sp]);
    }
}

// Função de decodificação de instruções
void execute_instruction(CPU *cpu, uint16_t instruction){

    uint16_t opcode = (instruction >> 12) & 0xF; // Bits 15-12

    printf("Instrução recebida: 0x%04x\n", instruction);

    uint16_t rd = (instruction >> 8) & 0x7; // Bits 10-8 (0-7)
    uint16_t rm = (instruction >> 5) & 0x7; // Bits 7-5 (0-7)
    uint16_t rn = (instruction >> 2) & 0x7; // Bits 4-2 (0-7)
    
    bool use_immediate = (instruction >> 11) & 0x1;
 
    switch(opcode) {
        case 0x0000:
            if(use_immediate) {
                int16_t immediate = ((int16_t) (instruction << 5)) >> 7;
                switch(instruction & 0x3) {
                    case 0b00: // JMP
                        printf("JMP #%d\n", immediate);
                        cpu->pc += immediate;
                        break;
                    case 0b01: // JEQ
                        printf("JEQ #%d\n", immediate);
                        if(cpu->flags.zero) cpu->pc += immediate;
                        break;
                    case 0b10: // JLT
                        printf("JLT #%d\n", immediate);
                        if(cpu->flags.carry && !cpu->flags.zero) cpu->pc += immediate;
                        break;
                    default:   // JGT
                        printf("JGT #%d\n", immediate);
                        if(!cpu->flags.carry && !cpu->flags.zero) cpu->pc += immediate;
                        break;
                }
            }
            else {
                switch(instruction & 0x3) {
                    case 0b01: // PUSH
                        printf("PUSH R%d\n", rn);
                        if(is_valid_register(rn)){
                            if(cpu->sp >= MAX_STACK) {
                                cpu->stack[cpu->sp--] = cpu->registers[rn];
                            }
                        }
                        break;

                    case 0b10: // POP
                        printf("POP R%d\n", rd);
                        if(is_valid_register(rd)){
                            if(cpu->sp >= 0){
                                cpu->registers[rd] = cpu->stack[++cpu->sp];
                            }
                        }
                        break;
                    case 0x3:{
                        printf("CMP R%d, R%d\n", rm, rn);
                        
                        uint16_t val1 = cpu->registers[rm];
                        uint16_t val2 = cpu->registers[rn];
                        uint16_t result = val1 - val2;

                        cpu->flags.zero = (result == 0);
                        cpu->flags.negative = (result >> 15) & 0x1;
                        cpu->flags.carry = (val1 < val2);

                        printf("Comparação: R%d (0x%04x) - R%d (0x%04x) = 0x%04x\n",
                        rm, val1, rn, val2, result);

                        printf("Flags: Z=%d N=%d C=%d\n",
                        cpu->flags.zero, cpu->flags.negative, cpu->flags.carry);
                        break;
                    }
                    default:   // NOP
                        printf("NOP\n");
                        print_simulator_state(cpu);
                        break;
                }
            }
            break;
            
        case 0x1: // MOV
            if(use_immediate) { // Se for MOV imediato
                uint16_t immediate = instruction & 0xFF;
                printf("MOV R%d, #%d\n", rd, immediate);
                cpu->registers[rd] = immediate;
            } else {
                printf("MOV R%d, R%d\n", rd, rm);
                cpu->registers[rd] = cpu->registers[rm];
            }
            break;
        case 0x4: // ADD
            printf("ADD R%d, R%d, R%d\n", rd, rm, rn);
            cpu->registers[rd] = cpu->registers[rm] + cpu->registers[rn];
            cpu->flags.zero = (cpu->registers[rd] == 0);
            break;
        case 0x5: // SUB
            printf("SUB R%d, R%d, R%d\n", rd, rm, rn);
            cpu->registers[rd] = cpu->registers[rm] - cpu->registers[rn];
            cpu->flags.zero = (cpu->registers[rd] == 0);
            break;
        default:
            printf("Instrução não implementada: 0x%04x\n", instruction);
            break;
    }

    if(!is_valid_register(rd) || !is_valid_register(rm) || !is_valid_register(rn)){
        printf("Erro: Registrador inválido na instrução 0x%04x\n", instruction);
        return;
    }
}


// Função para buscar e decodificar e executar
void run_program(CPU *cpu){
    while(cpu->pc < MAX_MEMORY && cpu->memory[cpu->pc / 2] != 0xFFFF){
        uint16_t instruction = cpu->memory[cpu->pc / 2];
        execute_instruction(cpu, instruction);
        cpu->pc += 2;
        printf("NEXT: 0x%04X\n", cpu->memory[cpu->pc / 2]);
    }
}

int main() {
    CPU cpu;

    init_simulator(&cpu);

    if(!load_program(&cpu, "program.txt")){
        return 1;
    }
    
    run_program(&cpu);

    return 0;
}