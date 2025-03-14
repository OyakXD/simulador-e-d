#include <stdio.h>
#include <string.h>
#include "decode/Decode.h"

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

// Função para buscar e decodificar e executar
void run_program(CPU *cpu){
    while(cpu->pc < MAX_MEMORY && cpu->memory[cpu->pc / 2] != 0xFFFF){
        uint16_t instruction = cpu->memory[cpu->pc / 2];
        execute_instruction(cpu, instruction);
        cpu->pc += 2;
    }
    print_simulator_state(cpu);
}


int main() {
    CPU cpu;

    init_simulator(&cpu);

    if(!load_program(&cpu, "tests/code2_hex.txt")){
        return 1;
    }
    
    run_program(&cpu);

    return 0;
}