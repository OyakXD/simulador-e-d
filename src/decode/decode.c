#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Decode.h"
#include "stdbool.h"

void init_simulator(CPU *cpu) {
    
    // Zeradores
    memset(cpu->registers, 0, sizeof(cpu->registers));
    memset(cpu->memory, 0, sizeof(cpu->memory));
    memset(cpu->data_memory, 0, sizeof(cpu->data_memory));
    memset(cpu->stack, 0 , sizeof(cpu->stack));
    // ------------

    cpu->pc = 0;
    cpu->sp = 0x8200; 
    cpu->flags.zero = 0;
    cpu->flags.carry = 0;
    cpu->flags.negative = 0;
    cpu->flags.overflow = 0;
    cpu->prog_size = 0;
}

int is_valid_register(uint16_t reg){
    return (reg < NUM_REGISTERS); // Retorna 1 se válido (0-7), 0 se inválido
}

void detect_program_size(CPU *cpu) {
    for (uint16_t i = 0; i < MAX_MEMORY; i++) {
        if (cpu->memory[i] == 0xFFFF) { // HALT
            cpu->prog_size = i;
            return;
        }
    }
    cpu->prog_size = MAX_MEMORY - 1; 
}

void detect_data_size(CPU *cpu) {
    for (uint16_t i = 0; i < MAX_MEMORY; i++) {
        if (cpu->memory[i] == 0xFFFF) { // HALT
            cpu->prog_size = i * 2;
            return;
        }
    }
    cpu->prog_size = (MAX_MEMORY - 1) * 2; 
}

void execute_instruction(CPU *cpu, uint16_t instruction){

    cpu->ir = instruction;

    if(instruction == 0xFFFF){
        print_simulator_state(cpu);
    }

    uint16_t opcode = (instruction >> 12) & 0xF; // Bits 15-12
    uint16_t rd = (instruction >> 8) & 0x7; // Bits 10-8 (0-7)
    uint16_t rm = (instruction >> 5) & 0x7; // Bits 7-5 (0-7)
    uint16_t rn = (instruction >> 2) & 0x7; // Bits 4-2 (0-7)

    bool use_immediate = (instruction >> 11) & 0x1;

    if(opcode == 0xFFFF){
        print_simulator_state(cpu);
    }
 
    switch(opcode) {
        case 0x0000:
            if(use_immediate) {
                int16_t immediate = ((int16_t)(instruction << 5)) >> 7;
                switch (instruction & 0x3) {
                    case 0x0: // JMP
                        printf("JMP: 0x%04x\n", immediate);
                        cpu->pc += immediate;
                        break;
                    case 0x1: // JEQ
                        if (cpu->flags.zero) {
                            printf("JEQ: Salto para PC += 0x%04x\n", immediate);
                            cpu->pc += immediate;
                            break;
                        } 
                        break;
                    case 0x2: // JLT
                        if (cpu->flags.negative && !cpu->flags.zero) {
                            printf("JLT: Salto para PC += 0x%04x\n", immediate);
                            cpu->pc += immediate;
                            break;
                        }
                        break;
                    default: // JGT
                        if (!cpu->flags.carry && !cpu->flags.zero) {
                            printf("JGT: Salto para PC += 0x%04x\n", immediate);
                            cpu->pc += immediate;
                            break;
                        }
                    break;
                }
            }
            else {
                switch(instruction & 0x3) {
                    case 0x1: // PUSH
                        if(is_valid_register(rn)){
                            if(cpu->sp >= 0x8100 && cpu->sp <= 0x8200) {
                                uint16_t stack_index = (cpu->sp - 0x8100) & 0xFF;
                                cpu->stack[stack_index] = cpu->registers[rn];
                                cpu->sp -= 2;
                            } else {
                                printf("Erro: Ponteiro da pilha fora dos limites!\n");
                            }
                        }
                        break;
                    case 0x2: // POP
                        if(is_valid_register(rd)){
                            if(cpu->sp >= 0x8100 && cpu->sp < 0x8200){
                                uint16_t stack_index = (cpu->sp - 0x8100) & 0xFF;
                                cpu->registers[rd] = cpu->stack[stack_index];
                                cpu->sp += 2;
                            } else {
                                printf("Erro: Ponteiro da pilha fora dos limites!\n");
                            }
                        }
                        break;
                    case 0x3:{
                        uint16_t val1 = cpu->registers[rm];
                        uint16_t val2 = cpu->registers[rn];
                        uint16_t result = val1 - val2;

                        update_flags(cpu, result, val1, val2, '-');

                        break;
                    }
                    default:   // NOP
                        if(opcode == 0x000 && instruction == 0){
                        print_simulator_state(cpu);
                        break;
                    } else {
                        printf("\nInstrução Inválida\n");
                        print_simulator_state(cpu);
                        exit(1);
                    }
                }
            }
            break;
            
        case 0x1: // MOV
            if(use_immediate) { // Se for MOV imediato
                uint16_t immediate = instruction & 0xFF;
                cpu->registers[rd] = immediate;
            } else {
                cpu->registers[rd] = cpu->registers[rm];
            }
            break;
        case 0x2: //STR
        if (use_immediate) {
            uint16_t immediate = instruction & 0xFF;
            if (cpu->registers[rm] < DATA_MEM_SIZE) {
                cpu->data_memory[cpu->registers[rm]] = immediate;
            } else {
                printf("Erro: Endereço 0x%04x fora dos limites da memória de dados!\n", cpu->registers[rm]);
            }
        } else {
            if (cpu->registers[rm] < DATA_MEM_SIZE) {
                cpu->data_memory[cpu->registers[rm]] = cpu->registers[rn];
            } else {
                printf("Erro: Endereço 0x%04x fora dos limites da memória de dados!\n", cpu->registers[rm]);
            }
        }
        break;
        case 0x3: // LDR
            if (cpu->registers[rm] < DATA_MEM_SIZE) {
                cpu->registers[rd] = cpu->data_memory[cpu->registers[rm]];
            } else {
                printf("Erro: Endereço 0x%04x fora dos limites da memória de dados!\n", cpu->registers[rm]);
            }
        break;
        case 0x4: // ADD
            cpu->registers[rd] = cpu->registers[rm] + cpu->registers[rn];
            update_flags(cpu, cpu->registers[rd], cpu->registers[rm], cpu->registers[rn], '+');
            break;
        case 0x5: // SUB
            cpu->registers[rd] = cpu->registers[rm] - cpu->registers[rn];
            update_flags(cpu, cpu->registers[rd], cpu->registers[rm], cpu->registers[rn], '-');
            break;
        case 0x6: // MULT
            cpu->registers[rd] = cpu->registers[rm] * cpu->registers[rn];
            update_flags(cpu, cpu->registers[rd], cpu->registers[rm], cpu->registers[rn], '*');
            break;
            case 0x7: // AND
            printf("R%d, R%d, R%d:\n", rd, rm, rn);  // Depuração para AND
            cpu->registers[rd] = cpu->registers[rm] & cpu->registers[rn];
            update_flags(cpu, cpu->registers[rd], cpu->registers[rm], cpu->registers[rn], '&');
            break;
        case 0x8: // ORR
            cpu->registers[rd] = cpu->registers[rm] | cpu->registers[rn];
            update_flags(cpu, cpu->registers[rd], cpu->registers[rm], cpu->registers[rn], '|');
            break;
        case 0x9: // NOT
            cpu->registers[rd] = ~cpu->registers[rm];
            update_flags(cpu, cpu->registers[rd], cpu->registers[rm], 0, '~');
            break;
        case 0xA: // XOR
            cpu->registers[rd] = cpu->registers[rm] ^ cpu->registers[rn];
            update_flags(cpu, cpu->registers[rd], cpu->registers[rm], cpu->registers[rn], '^');
            break;
        case 0xB: // SHR
            cpu->registers[rd] = cpu->registers[rm] >> 1;
            update_flags(cpu, cpu->registers[rd], cpu->registers[rm], 0, '>');
            break;
        case 0xC: // SHL
            cpu->registers[rd] = cpu->registers[rm] << 1;
            update_flags(cpu, cpu->registers[rd], cpu->registers[rm], 0, '<');
            break;
        case 0xD: // ROR
            cpu->registers[rd] = (cpu->registers[rm] >> 1) | (cpu->registers[rm] << 15);
            update_flags(cpu, cpu->registers[rd], cpu->registers[rm], 0, 'r');
            break;
        case 0xE: // ROL
            cpu->registers[rd] = (cpu->registers[rm] << 1) | (cpu->registers[rm] >> 15);
            update_flags(cpu, cpu->registers[rd], cpu->registers[rm], 0, 'l');
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

void update_flags(CPU *cpu, uint16_t result, uint16_t op1, uint16_t op2, char operation){

    cpu->flags.zero = (result == 0);

    cpu->flags.negative = (result >> 15) & 0x1;

    switch(operation){
        case '+': // ADD
            cpu->flags.carry = ((uint32_t)op1 + (uint32_t)op2) > 0xFFFF;
            cpu->flags.overflow = ((op1 ^ result) & (op2 ^ result) & 0x8000) != 0;
            break;
        case '-': // SUB & CMP
            cpu->flags.carry = (uint32_t)op1 >= (uint32_t)op2 ? 1 : 0;
            cpu->flags.overflow = ((op1 ^ op2) & (op1 ^ result) & 0x8000) != 0;
            break;
        case '&': 
        case '|':
        case '^':
        case '~':
            cpu->flags.carry = 0;
            cpu->flags.overflow = 0;
            break;
        
        case '<': // SHL
            cpu->flags.carry = (op1 >> 15) & 0x1;
            cpu->flags.overflow = 0;
            break;
        case '>': // SHR
            cpu->flags.carry = op1 & 0x1;
            cpu->flags.overflow = 0;
            break;
        case 'r': // ROR
            cpu->flags.carry = op1 & 0x1;
            cpu->flags.overflow = 0;
            break;
        case 'l': // ROL
            cpu->flags.carry = (op1 >> 15) & 0x1;
            cpu->flags.overflow = 0;
            break;
        case '*': // MUL
            uint16_t mul_result = (uint32_t)op1 * (uint32_t)op2;
            cpu->flags.carry = (mul_result > 0xFFFF);
            cpu->flags.overflow = (mul_result > 0xFFFF);
            break; 
    }
}

void print_simulator_state(CPU *cpu){
    printf("\nEstado do simulador:\n");

    detect_program_size(cpu);
    printf("\nMemória do Programa:\n");
    for (uint16_t i = 0; i <= cpu->prog_size; i++) {
        printf("%04x: 0x%04x\n", i * 2, cpu->memory[i]);
    }
    printf("\n");
    
    for(int i = 0; i < NUM_REGISTERS; i++){
        printf("R%d: 0x%04x ", i, cpu->registers[i]);
    }

    printf("\nPC: 0x%04x IR: 0x%04x\n", cpu->pc, cpu->ir);

    printf("Flags: Z=%d N=%d C=%d, Ov=%d\n",
    cpu->flags.zero, cpu->flags.negative, cpu->flags.carry, cpu->flags.overflow);

    detect_data_size(cpu);
    printf("\nMémoria de Dados:\n");
    for(uint16_t i = 0; i <= cpu->prog_size; i += 2) {
            printf("0x%04x:0x%04x\n", i, cpu->data_memory[i]);
    }

    printf("\nEstado Final do Ponteiro da Pilha(SP):\n");
    if(cpu->sp >= 0x8100 && cpu->sp <= 0x8200){
        for(uint16_t addr = 0x8200; addr >= cpu->sp && addr >= 0x8100; addr -= 2){
            uint16_t stack_index = (addr - 0x8100) & 0xFF;
            printf("0x%04x: 0x%04x\n", addr, cpu->stack[stack_index]);
        }
    } else {
        printf("Pilha vazia ou SP fora dos limites (0x8100-0x8200)\n");
    }
}