#ifndef DECODE_H
#define DECODE_H
#include <stdint.h>
#include <stdbool.h>

#define DATA_MEM_SIZE 256
#define MAX_MEMORY 1024
#define NUM_REGISTERS 8
#define MAX_STACK 255

typedef struct {
  uint16_t registers[NUM_REGISTERS];
  uint16_t memory[MAX_MEMORY];
  uint16_t data_memory[DATA_MEM_SIZE];
  uint16_t prog_size;
  uint16_t pc;
  uint16_t stack[MAX_STACK];
  uint16_t lowest_sp;
  uint16_t sp;
  uint16_t ir;

  // Flags de condição
  struct {
    uint8_t zero;
    uint8_t carry;
    uint8_t negative;
    uint8_t overflow;
  } flags;

} CPU;

// Função de decodificação de instruções
void execute_instruction(CPU *cpu, uint16_t instruction);

// Função que muda a flag de acordo com a ultima instrução de ULA
void update_flags(CPU *cpu, uint16_t result, uint16_t op1, uint16_t op2, char operation);

// Verifica os registradores
int is_valid_register(uint16_t reg);

// Função que printa o estado do simulador
void print_simulator_state(CPU *cpu);

// Função que inicia o simulador 
void init_simulator(CPU *cpu);

// Nova função para detectar o tamanho da Memória de Programa
void detect_program_size(CPU *cpu);

// Nova função para detectar o tamanho da Memória de dados
void detect_data_size(CPU *cpu);


#endif