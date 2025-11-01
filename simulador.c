// simulator.c - Núcleo principal do simulador de SO
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "gantt_bmp.h"

// === ESTADOS DAS TAREFAS ===
typedef enum {
    STATE_NEW,
    STATE_READY,
    STATE_RUNNING,
    STATE_BLOCKED,
    STATE_TERMINATED
} TaskState;

// === TASK CONTROL BLOCK (TCB) ===
typedef struct {
    int id;
    char color[8];
    int arrival_time;
    int burst_time;
    int remaining_time;
    int priority;
    TaskState state;
    
    // Estatísticas
    int start_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int response_time;
} TCB;

// === CONFIGURAÇÃO DA SIMULAÇÃO ===
typedef struct {
    char algorithm[20];
    int quantum;
    TCB* tasks;
    int task_count;
} SimConfig;

// === SISTEMA DE CLOCK ===
typedef struct {
    int current_tick;
    int quantum_counter;
    int quantum_size;
} SystemClock;

// === SIMULADOR ===
typedef struct {
    SystemClock clock;
    TCB* tasks;
    int task_count;
    TCB* current_task;
    GanttEntry* gantt_entries;
    int gantt_count;
    int gantt_capacity;
    char algorithm[20];
} Simulator;

// === PARSER DE CONFIGURAÇÃO ===
SimConfig* parse_config(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("Erro ao abrir arquivo: %s\n", filename);
        return NULL;
    }
    
    SimConfig* config = malloc(sizeof(SimConfig));
    char line[256];
    
    // Primeira linha: algoritmo;quantum
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        free(config);
        return NULL;
    }
    sscanf(line, "%[^;];%d", config->algorithm, &config->quantum);
    
    // Contar linhas para alocar tarefas
    int capacity = 10;
    config->tasks = malloc(capacity * sizeof(TCB));
    config->task_count = 0;
    
    // Ler tarefas
    while (fgets(line, sizeof(line), f)) {
        if (strlen(line) < 5) continue; // Linha vazia
        
        if (config->task_count >= capacity) {
            capacity *= 2;
            config->tasks = realloc(config->tasks, capacity * sizeof(TCB));
        }
        
        TCB* task = &config->tasks[config->task_count];
        char events[100] = ""; // Ignorado no Projeto A
        
        int parsed = sscanf(line, "%d;%7[^;];%d;%d;%d;%s",
                           &task->id,
                           task->color,
                           &task->arrival_time,
                           &task->burst_time,
                           &task->priority,
                           events);
        
        if (parsed >= 5) { // Pelo menos os campos obrigatórios
            task->remaining_time = task->burst_time;
            task->state = STATE_NEW;
            task->start_time = -1;
            task->completion_time = 0;
            task->turnaround_time = 0;
            task->waiting_time = 0;
            task->response_time = 0;
            config->task_count++;
        }
    }
    
    fclose(f);
    return config;
}

// === INICIALIZAR SIMULADOR ===
Simulator* create_simulator(SimConfig* config) {
    Simulator* sim = malloc(sizeof(Simulator));
    
    sim->clock.current_tick = 0;
    sim->clock.quantum_counter = 0;
    sim->clock.quantum_size = config->quantum;
    
    sim->task_count = config->task_count;
    sim->tasks = malloc(sim->task_count * sizeof(TCB));
    memcpy(sim->tasks, config->tasks, sim->task_count * sizeof(TCB));
    
    strcpy(sim->algorithm, config->algorithm);
    sim->current_task = NULL;
    
    sim->gantt_capacity = 1000;
    sim->gantt_entries = malloc(sim->gantt_capacity * sizeof(GanttEntry));
    sim->gantt_count = 0;
    
    return sim;
}

// === ADICIONAR ENTRADA NO GANTT ===
void add_gantt_entry(Simulator* sim, int task_id, int start, int end, const char* color) {
    if (sim->gantt_count >= sim->gantt_capacity) {
        sim->gantt_capacity *= 2;
        sim->gantt_entries = realloc(sim->gantt_entries, 
                                     sim->gantt_capacity * sizeof(GanttEntry));
    }
    
    GanttEntry* entry = &sim->gantt_entries[sim->gantt_count++];
    entry->task_id = task_id;
    entry->start_time = start;
    entry->end_time = end;
    strcpy(entry->color, color);
}

// === VERIFICAR SE TODAS AS TAREFAS TERMINARAM ===
bool all_tasks_completed(Simulator* sim) {
    for (int i = 0; i < sim->task_count; i++) {
        if (sim->tasks[i].state != STATE_TERMINATED) {
            return false;
        }
    }
    return true;
}

// === ESCALONADOR FIFO ===
TCB* schedule_fifo(Simulator* sim) {
    TCB* next = NULL;
    int earliest_arrival = INT_MAX;
    
    for (int i = 0; i < sim->task_count; i++) {
        TCB* task = &sim->tasks[i];
        
        // Verificar tarefas que chegaram e estão prontas
        if (task->arrival_time <= sim->clock.current_tick &&
            task->state != STATE_TERMINATED &&
            task->state != STATE_RUNNING) {
            
            if (task->arrival_time < earliest_arrival) {
                earliest_arrival = task->arrival_time;
                next = task;
            }
        }
    }
    
    return next;
}

// === ESCALONADOR SRTF ===
TCB* schedule_srtf(Simulator* sim) {
    TCB* next = NULL;
    int shortest_remaining = INT_MAX;
    
    for (int i = 0; i < sim->task_count; i++) {
        TCB* task = &sim->tasks[i];
        
        if (task->arrival_time <= sim->clock.current_tick &&
            task->state != STATE_TERMINATED) {
            
            if (task->remaining_time < shortest_remaining) {
                shortest_remaining = task->remaining_time;
                next = task;
            }
        }
    }
    
    return next;
}

// === ESCALONADOR PRIORITY ===
TCB* schedule_priority(Simulator* sim) {
    TCB* next = NULL;
    int highest_priority = INT_MAX; // Menor valor = maior prioridade
    
    for (int i = 0; i < sim->task_count; i++) {
        TCB* task = &sim->tasks[i];
        
        if (task->arrival_time <= sim->clock.current_tick &&
            task->state != STATE_TERMINATED) {
            
            if (task->priority < highest_priority) {
                highest_priority = task->priority;
                next = task;
            }
        }
    }
    
    return next;
}

// === EXECUTAR UM TICK ===
void simulate_tick(Simulator* sim) {
    // Atualizar estados das tarefas que chegaram
    for (int i = 0; i < sim->task_count; i++) {
        if (sim->tasks[i].arrival_time == sim->clock.current_tick &&
            sim->tasks[i].state == STATE_NEW) {
            sim->tasks[i].state = STATE_READY;
            printf("[Tick %d] Tarefa %d chegou\n", 
                   sim->clock.current_tick, sim->tasks[i].id);
        }
    }
    
    // Selecionar próxima tarefa
    TCB* next_task = NULL;
    
    if (strcmp(sim->algorithm, "FIFO") == 0) {
        next_task = schedule_fifo(sim);
    } else if (strcmp(sim->algorithm, "SRTF") == 0) {
        next_task = schedule_srtf(sim);
    } else if (strcmp(sim->algorithm, "PRIORITY") == 0) {
        next_task = schedule_priority(sim);
    }
    
    // Troca de contexto se necessário
    if (next_task != sim->current_task) {
        if (sim->current_task && sim->current_task->state == STATE_RUNNING) {
            sim->current_task->state = STATE_READY;
        }
        
        if (next_task) {
            if (next_task->start_time == -1) {
                next_task->start_time = sim->clock.current_tick;
            }
            next_task->state = STATE_RUNNING;
            printf("[Tick %d] Executando tarefa %d\n", 
                   sim->clock.current_tick, next_task->id);
        }
        
        sim->current_task = next_task;
    }
    
    // Executar tarefa atual
    if (sim->current_task) {
        sim->current_task->remaining_time--;
        
        // Adicionar ao Gantt
        if (sim->gantt_count == 0 || 
            sim->gantt_entries[sim->gantt_count-1].task_id != sim->current_task->id ||
            sim->gantt_entries[sim->gantt_count-1].end_time != sim->clock.current_tick) {
            
            add_gantt_entry(sim, sim->current_task->id, 
                          sim->clock.current_tick, 
                          sim->clock.current_tick + 1,
                          sim->current_task->color);
        } else {
            // Estender entrada existente
            sim->gantt_entries[sim->gantt_count-1].end_time++;
        }
        
        // Verificar se terminou
        if (sim->current_task->remaining_time == 0) {
            sim->current_task->state = STATE_TERMINATED;
            sim->current_task->completion_time = sim->clock.current_tick + 1;
            sim->current_task->turnaround_time = 
                sim->current_task->completion_time - sim->current_task->arrival_time;
            sim->current_task->waiting_time = 
                sim->current_task->turnaround_time - sim->current_task->burst_time;
            
            printf("[Tick %d] Tarefa %d concluída\n", 
                   sim->clock.current_tick, sim->current_task->id);
            
            sim->current_task = NULL;
        }
    }
    
    sim->clock.current_tick++;
}

// === MODO DE EXECUÇÃO COMPLETA ===
void run_complete(Simulator* sim) {
    printf("\n=== INICIANDO SIMULAÇÃO (%s) ===\n", sim->algorithm);
    
    while (!all_tasks_completed(sim)) {
        simulate_tick(sim);
    }
    
    printf("\n=== SIMULAÇÃO CONCLUÍDA ===\n");
}

// === MODO PASSO-A-PASSO ===
void run_step_by_step(Simulator* sim) {
    printf("\n=== MODO DEBUG - PASSO A PASSO ===\n");
    printf("Comandos: [Enter] próximo tick, [c] continuar, [i] inspecionar, [q] sair\n\n");
    
    char cmd[10];
    while (!all_tasks_completed(sim)) {
        printf("\n--- Tick %d ---\n", sim->clock.current_tick);
        
        // Mostrar estado atual
        printf("Estados: ");
        for (int i = 0; i < sim->task_count; i++) {
            printf("T%d:", sim->tasks[i].id);
            switch(sim->tasks[i].state) {
                case STATE_NEW: printf("NEW "); break;
                case STATE_READY: printf("READY "); break;
                case STATE_RUNNING: printf("RUNNING "); break;
                case STATE_TERMINATED: printf("DONE "); break;
                default: printf("? ");
            }
        }
        printf("\n");
        
        printf("Comando: ");
        fgets(cmd, sizeof(cmd), stdin);
        
        if (cmd[0] == 'q') break;
        if (cmd[0] == 'c') {
            run_complete(sim);
            break;
        }
        if (cmd[0] == 'i') {
            printf("\n=== INSPEÇÃO DO SISTEMA ===\n");
            for (int i = 0; i < sim->task_count; i++) {
                TCB* t = &sim->tasks[i];
                printf("Task %d: arrival=%d, burst=%d, remaining=%d, priority=%d\n",
                       t->id, t->arrival_time, t->burst_time, 
                       t->remaining_time, t->priority);
            }
            continue;
        }
        
        simulate_tick(sim);
    }
}

// === EXIBIR ESTATÍSTICAS ===
void print_statistics(Simulator* sim) {
    printf("\n=== ESTATÍSTICAS DAS TAREFAS ===\n");
    printf("ID | Arrival | Burst | Complete | Turnaround | Waiting\n");
    printf("---|---------|-------|----------|------------|--------\n");
    
    float avg_turnaround = 0;
    float avg_waiting = 0;
    
    for (int i = 0; i < sim->task_count; i++) {
        TCB* t = &sim->tasks[i];
        printf("%2d | %7d | %5d | %8d | %10d | %7d\n",
               t->id, t->arrival_time, t->burst_time,
               t->completion_time, t->turnaround_time, t->waiting_time);
        
        avg_turnaround += t->turnaround_time;
        avg_waiting += t->waiting_time;
    }
    
    avg_turnaround /= sim->task_count;
    avg_waiting /= sim->task_count;
    
    printf("\nMédias: Turnaround = %.2f, Waiting = %.2f\n", 
           avg_turnaround, avg_waiting);
}

// === MAIN ===
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <arquivo_config> [--step]\n", argv[0]);
        printf("\nExemplo de arquivo de configuração:\n");
        printf("FIFO;10\n");
        printf("0;#FF0000;0;20;1;\n");
        printf("1;#00FF00;5;15;2;\n");
        printf("2;#0000FF;10;10;3;\n");
        return 1;
    }
    
    // Carregar configuração
    SimConfig* config = parse_config(argv[1]);
    if (!config) {
        return 1;
    }
    
    // Criar simulador
    Simulator* sim = create_simulator(config);
    
    // Verificar modo de execução
    bool step_mode = false;
    if (argc > 2 && strcmp(argv[2], "--step") == 0) {
        step_mode = true;
    }
    
    // Executar simulação
    if (step_mode) {
        run_step_by_step(sim);
    } else {
        run_complete(sim);
    }
    
    // Exibir estatísticas
    print_statistics(sim);
    
    // Gerar Gantt Chart
    int max_time = sim->clock.current_tick;
    create_gantt_bmp("gantt_output.bmp", sim->gantt_entries, 
                     sim->gantt_count, max_time, sim->task_count);
    
    // Limpar memória
    free(sim->tasks);
    free(sim->gantt_entries);
    free(sim);
    free(config->tasks);
    free(config);
    
    return 0;
}