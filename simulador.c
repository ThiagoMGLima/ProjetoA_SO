/*
 * Simulador de Escalonamento de Processos
 * --------------------------------------
 * Este arquivo contém a implementação do simulador de escalonamento de tarefas
 * usado no projeto. Ele define as estruturas de dados (TCB, SimConfig, SystemClock,
 * Simulator) e as funções que:
 *  - Fazem o parse do arquivo de configuração (parse_config)
 *  - Inicializam o simulador (create_simulator)
 *  - Implementam diversos escalonadores (FIFO, SRTF, PRIORITY)
 *  - Executam a simulação tick-a-tick (simulate_tick)
 *  - Fornecem um modo interativo passo-a-passo para depuração
 *  - Geram estatísticas e o gráfico de Gantt ao final
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "gantt_bmp.h"
#include "gantt_ascii.h"
#include "stats_viewer.h"

// === ESTADOS DAS TAREFAS ===
// Estado lógico das tarefas usados pelo simulador. Permite controlar o ciclo
// de vida de uma tarefa desde a chegada (NEW) até a finalização (TERMINATED).
typedef enum {
    STATE_NEW,
    STATE_READY,
    STATE_RUNNING,
    STATE_BLOCKED,
    STATE_TERMINATED
} TaskState;

// === TASK CONTROL BLOCK (TCB) ===
// Estrutura que foi aconselhada para representa uma tarefa/processo. Contém informação fixa
// (id, arrival_time, burst_time, priority) e campos de execução mutáveis
// (remaining_time, state) além de métricas para estatísticas.
// Campos de estatística (start_time, completion_time, turnaround_time,
// waiting_time, response_time) são atualizados durante a simulação.
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

// === CONFIGURAÇÃO DA SIMULAÇÃO (SimConfig) ===
// Estrutura carregada a partir do arquivo de configuração. Contém o algoritmo
// solicitado, o quantum (se aplicável) e a lista de tarefas que serão simuladas.
typedef struct {
    char algorithm[20];
    int quantum;
    TCB* tasks;
    int task_count;
} SimConfig;

// === SISTEMA DE CLOCK ===
// Relógio lógico do simulador. current_tick é o tempo discreto atual.
// quantum_counter e quantum_size são usados por algoritmos por fatia (ex.: Round-Robin).
typedef struct {
    int current_tick;
    int quantum_counter;
    int quantum_size;
} SystemClock;

// === SIMULADOR (Simulator) ===
// Agrega estado global da simulação: relógio, lista de tarefas, tarefa atual,
// entradas do Gantt (para geração do bitmap) e o algoritmo selecionado.
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
// Lê o arquivo de configuração no formato esperado e popula uma SimConfig.
// Cada linha de tarefa: id;color;arrival;burst;priority;...
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
        char events[100] = "";

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
// Cria e inicializa a estrutura Simulator a partir de uma SimConfig já
// carregada. Copia os TCBs da configuração para o simulador e inicializa
// o relógio e o buffer do Gantt.
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
// Registra um intervalo [start, end) correspondente à execução de uma tarefa
// (usado para gerar o arquivo BMP no final). O buffer cresce dinamicamente.
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
// Retorna true se todas as tasks estiverem no estado TERMINATED.
bool all_tasks_completed(Simulator* sim) {
    for (int i = 0; i < sim->task_count; i++) {
        if (sim->tasks[i].state != STATE_TERMINATED) {
            return false;
        }
    }
    return true;
}

// === ESCALONADOR FIFO ===
// Escolhe a tarefa com menor tempo de chegada entre as disponíveis.
TCB* schedule_fifo(Simulator* sim) {
    TCB* next = NULL;
    int earliest_arrival = INT_MAX;

    for (int i = 0; i < sim->task_count; i++) {
        TCB* task = &sim->tasks[i];

        // Verificar tarefas que chegaram e estão prontas
        if (task->arrival_time <= sim->clock.current_tick &&
            task->state != STATE_TERMINATED) {

            if (task->arrival_time < earliest_arrival) {
                earliest_arrival = task->arrival_time;
                next = task;
            }
        }
    }

    return next;
}

// === ESCALONADOR SRTF ===
// Escolhe a tarefa com menor remaining_time entre as disponíveis.
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
// Escolhe a tarefa com menor valor de prioridade (0 = mais alta).
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
/*
 * simulate_tick é o coração do simulador. Operações realizadas por tick:
 * 1) marca chegada de tarefas cujo arrival_time == current_tick (muda NEW -> READY)
 * 2) seleciona a próxima tarefa a executar usando o escalonador configurado
 * 3) realiza possível troca de contexto (alterando estados)
 * 4) decrementa remaining_time da tarefa em execução e atualiza Gantt
 * 5) completa tarefas quando remaining_time chega a zero e coleta métricas
 * 6) incrementa o relógio (current_tick++)
 *
 * Nota importante: a ordem das operações é deliberada. Por exemplo, chegadas
 * são processadas antes da seleção para que tarefas que chegam neste tick
 * possam ser imediatamente consideradas para execução.
 */
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
        // Somente rebaixar o estado da tarefa corrente se houver uma próxima tarefa
        if (sim->current_task && sim->current_task->state == STATE_RUNNING && next_task) {
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
// Executa ticks em laço até que todas as tarefas estejam concluídas.
void run_complete(Simulator* sim) {
    printf("\n=== INICIANDO SIMULAÇÃO (%s) ===\n", sim->algorithm);

    while (!all_tasks_completed(sim)) {
        simulate_tick(sim);
    }

    printf("\n=== SIMULAÇÃO CONCLUÍDA ===\n");
}

// === MODO PASSO-A-PASSO ===
/*
 * run_step_by_step fornece interação humana para depuração: o usuário pode
 * avançar tick-a-tick, inspecionar o estado, ou continuar em modo completo.
 * A função também trata EOF/erro de entrada (fgets retornando NULL) para
 * evitar comportamento indefinido quando a entrada termina.
 */
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
                case STATE_NEW: printf("NEW ");
                break;
                case STATE_READY: printf("READY ");
                break;
                case STATE_RUNNING: printf("RUNNING ");
                break;
                case STATE_TERMINATED: printf("DONE ");
                break;
                default: printf("? ");
            }
        }
        printf("\n");

        printf("Comando: ");
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
            /* EOF or error: exit debug mode cleanly */
            printf("\nEntrada encerrada. Saindo do modo debug.\n");
            break;
        }

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
// Gera resumo das métricas coletadas por tarefa no final da simulação.
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

// Função helper para limpar o buffer de entrada (stdin)
// Isso previne que 'newlines' de entradas anteriores afetem o getchar()
void clear_stdin_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// === MAIN ===
/*
 * main(): fluxo principal:
 *  - valida argumentos
 *  - carrega SimConfig do arquivo
 *  - cria Simulator
 *  - executa (completo ou passo-a-passo)
 *  - imprime estatísticas e Gantt
 *  - libera memória
 */
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

// Limpa o buffer de qualquer 'Enter' restante do modo debug ou simulação
    clear_stdin_buffer();

    int max_time = sim->clock.current_tick;
    printf("\nExibir estatísticas detalhadas (s/n)? ");
    char stats_choice = getchar();
    clear_stdin_buffer();

    if (stats_choice == 's' || stats_choice == 'S') {
        // 1. Alocar array para o formato TaskStats
        TaskStats* stats_array = malloc(sim->task_count * sizeof(TaskStats));

        // 2. Converter TCB para TaskStats
        for (int i = 0; i < sim->task_count; i++) {
            TCB* t = &sim->tasks[i];
            stats_array[i].id = t->id;
            stats_array[i].arrival = t->arrival_time;
            stats_array[i].burst = t->burst_time;
            stats_array[i].completion = t->completion_time;
            stats_array[i].turnaround = t->turnaround_time;
            stats_array[i].waiting = t->waiting_time;
            // O response time é (início da 1ª execução - chegada)
            stats_array[i].response = t->start_time - t->arrival_time;
            stats_array[i].priority = t->priority;
        }

        // 3. Chamar a nova função
        show_statistics(stats_array, sim->task_count, sim->algorithm);

        // 4. Oferecer exportação CSV
        printf("\nDeseja exportar estas estatísticas para .csv (s/n)? ");
        char csv_choice = getchar();
        clear_stdin_buffer();
        if (csv_choice == 's' || csv_choice == 'S') {
            export_to_csv(stats_array, sim->task_count, sim->algorithm);
        }

        // 5. Liberar memória
        free(stats_array);

    } else {
        // Usar a visualização simples
        print_statistics(sim);
    }

// --- Gantt ASCII ---
    printf("\nDeseja exibir o Gantt Chart ASCII no terminal (s/n)? ");
    char ascii_choice = getchar();
    clear_stdin_buffer();

    if (ascii_choice == 's' || ascii_choice == 'S') {
        print_gantt_ascii(sim->gantt_entries, sim->gantt_count,
                          max_time, sim->task_count);
    }

    // Gerar Gantt Chart (BMP)
    printf("Deseja gerar o gráfico de Gantt(.bmp)? (s/n): ");
    char resposta = getchar();
    clear_stdin_buffer();
    if (resposta == 's' || resposta == 'S') {
        create_gantt_bmp("gantt_output.bmp", sim->gantt_entries,
                        sim->gantt_count, max_time, sim->task_count);
    }
    // Limpar memória
    free(sim->tasks);
    free(sim->gantt_entries);
    free(sim);
    free(config->tasks);
    free(config);

    return 0;
}