#include <stdio.h>
#include <stdint.h>
#include "Decode.h"
#include "stdbool.h"

int is_valid_register(uint16_t reg){
    return (reg < NUM_REGISTERS); // Retorna 1 se válido (0-7), 0 se inválido
}

void execute_instruction(CPU *cpu, uint16_t instruction){

    cpu->ir = instruction;

    if(instruction == 0xFFFF){
        print_simulator_state(cpu);
    }

    uint16_t opcode = (instruction >> 12) & 0xF; // Bits 15-12

    printf("Instrução recebida: 0x%04x\n", instruction);

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
                int16_t immediate = ((int16_t) (instruction << 5)) >> 7;
                switch(instruction & 0x3) {
                    case 0x0: // JMP
                        cpu->pc += immediate;
                        break;
                    case 0x1: // JEQ
                        if(cpu->flags.zero) cpu->pc += immediate;
                        break;
                    case 0x2: // JLT
                        if(cpu->flags.carry && !cpu->flags.zero) cpu->pc += immediate;
                        break;
                    default:   // JGT
                        if(!cpu->flags.carry && !cpu->flags.zero) cpu->pc += immediate;
                        break;
                }
            }
            else {
                switch(instruction & 0x3) {
                    case 0x1: // PUSH
                        if(is_valid_register(rn)){
                            if(cpu->sp >= MAX_STACK) {
                                cpu->stack[cpu->sp--] = cpu->registers[rn];
                            }
                        }
                        break;

                    case 0x2: // POP
                        if(is_valid_register(rd)){
                            if(cpu->sp >= 0){
                                cpu->registers[rd] = cpu->stack[++cpu->sp];
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
                        print_simulator_state(cpu);
                        break;
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
            if (use_immediate){
                uint16_t immediate = instruction & 0xFF;
                cpu->memory[cpu->registers[rd] / 2] = immediate;
            } 
            else{
                cpu->memory[cpu->registers[rd] / 2] = cpu->registers[rm];
            }
            break;
        case 0x3: // LDR
            cpu->registers[rd] = cpu->memory[cpu->registers[rm] / 2];
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
            break;
        case 0x7: // AND
            cpu->registers[rd] = cpu->registers[rm] & cpu->registers[rn];
            break;
        case 0x8: // ORR
            cpu->registers[rd] = cpu->registers[rm] | cpu->registers[rn];
            break;
        case 0x9: // NOT
            cpu->registers[rd] = ~cpu->registers[rm];
            break;
        case 0xA: // XOR
            cpu->registers[rd] = cpu->registers[rm] ^ cpu->registers[rn];
            break;
        case 0xB: // SHR
            cpu->registers[rd] = cpu->registers[rm] >> 1;
            break;
        case 0xC: // SHL
            cpu->registers[rd] = cpu->registers[rm] << 1;
            break;
        case 0xD: // ROR
            cpu->registers[rd] = (cpu->registers[rm] >> 1) | (cpu->registers[rm] << 15);
            break;
        case 0xE: // ROL
            cpu->registers[rd] = (cpu->registers[rm] << 1) | (cpu->registers[rm] >> 15);
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
            cpu->flags.carry = (result < op1);
            cpu->flags.overflow = ((op1 ^ result) & (op2 ^ result)) >> 15;
            break;
        case '-': // SUB & CMP
            cpu->flags.carry = (op1 < op2);
            cpu->flags.overflow = ((op1 ^ op2) & (op1 ^ result)) >> 15;
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
            cpu->flags.carry = (result >> 16) & 0x1;
            cpu->flags.overflow = 0;
            break; 
    }
}

void print_simulator_state(CPU *cpu){
    printf("\nEstado do simulador:\n");

    // Imprimir os registradores
    for(int i = 0; i < NUM_REGISTERS; i++){
        printf("R%d: 0x%04x ", i, cpu->registers[i]);
    }

    // Imprime o contador do programa
    printf("\nPC: 0x%04x IR: 0x%04x\n", cpu->pc, cpu->ir);

    printf("Flags: Z=%d N=%d C=%d, Ov=%d\n",
    cpu->flags.zero, cpu->flags.negative, cpu->flags.carry, cpu->flags.overflow);

    printf("\nPonteiro da pilha (SP): %d\n", cpu->sp);
    if(cpu->sp >= 0){
        printf("Topo da pilha: 0x%04x\n", cpu->stack[cpu->sp]);
    }
}