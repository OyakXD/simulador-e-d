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
    cpu->sp = -1;
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


// Função de decodificação de instruções
void execute_instruction(CPU *cpu, uint16_t instruction){

    uint16_t opcode = (instruction >> 12) & 0xF; // Bits 15-12

    printf("Instrução recebida: 0x%04x\n", instruction);
 
    if(opcode == 0){ // Ele pode ser pop ou push
        
        uint16_t subop = (instruction >> 8) & 0xF; // Bits 11-8
        uint16_t rd = (instruction >> 4) & 0x7; // Bits 6-4
        
        switch(subop){
            case 0x0:
            printf("PUSH R%d\n", rd);
            if(is_valid_register(rd)){
                if(cpu->sp < MAX_STACK - 1) {
                    cpu->stack[++cpu->sp] = cpu->registers[rd];
                }
            }
            break;

            case 0x7:
            printf("POP R%d\n", rd);
            if(is_valid_register(rd)){
                if(cpu->sp >= 0){
                    cpu->registers[rd] = cpu->stack[cpu->sp--];
                }
            }
            break;

            case 0xF:
            // ...
            break;

            default:
            printf("Suboperação não encontrada");
            break;
        }
        return;
    }

    // Decodifica a instrução
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
        default:
            printf("Instrução não implementada: 0x%04x\n", instruction);
            break;
        }
}

// Função para buscar e decodificar e executar
void run_program(CPU *cpu){
    while(cpu->pc < MAX_MEMORY && cpu->memory[cpu->pc] != 0){
        uint16_t instruction = cpu->memory[cpu->pc];
        execute_instruction(cpu, instruction);
        cpu->pc++;
    }
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