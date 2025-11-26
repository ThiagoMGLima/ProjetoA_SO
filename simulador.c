/*
 * =============================================================================
 * Simulador de Escalonamento de Processos - Versão 2.0
 * =============================================================================
 * 
 * Este arquivo implementa um simulador de sistema operacional multitarefa
 * preemptivo de tempo compartilhado, conforme especificado no projeto.
 *
 * FUNCIONALIDADES IMPLEMENTADAS:
 * - Algoritmos de escalonamento: FIFO, RR (Round-Robin), SRTF, PRIORITY
 * - Modo de execução completa e passo-a-passo com depuração
 * - Sistema de histórico para avançar/retroceder a simulação (req. 1.5.2)
 * - Geração de gráfico de Gantt (BMP e ASCII)
 * - Estatísticas detalhadas de execução
 * - Estrutura preparada para eventos do Projeto B (mutex, I/O)
 *
 * FORMATO DO ARQUIVO DE CONFIGURAÇÃO:
 *   algoritmo_escalonamento;quantum
 *   id;cor;ingresso;duracao;prioridade;lista_eventos
 *
 * Autor: [Seu Nome]
 * Disciplina: Sistemas Operacionais
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "gantt_bmp.h"
#include "gantt_ascii.h"
#include "stats_viewer.h"

// =============================================================================
// CONSTANTES E DEFINIÇÕES
// =============================================================================

#define MAX_TASKS       100     // Número máximo de tarefas
#define MAX_EVENTS      50      // Número máximo de eventos por tarefa
#define MAX_HISTORY     10000   // Número máximo de snapshots no histórico
#define MAX_LINE_LEN    512     // Tamanho máximo de linha do arquivo config

// =============================================================================
// ENUMERAÇÕES
// =============================================================================

/**
 * Estados possíveis de uma tarefa durante a simulação.
 * Baseado no ciclo de vida de processos em SOs reais.
 */
typedef enum {
    STATE_NEW,          // Tarefa criada mas ainda não chegou ao sistema
    STATE_READY,        // Tarefa pronta para executar (na fila de prontos)
    STATE_RUNNING,      // Tarefa em execução na CPU
    STATE_BLOCKED,      // Tarefa bloqueada (aguardando I/O ou mutex)
    STATE_TERMINATED    // Tarefa concluída
} TaskState;

/**
 * Tipos de eventos que podem ocorrer durante a execução de uma tarefa.
 * Preparado para o Projeto B.
 */
typedef enum {
    EVENT_NONE,         // Sem evento
    EVENT_MUTEX_LOCK,   // Solicitação de mutex (MLxx:tt)
    EVENT_MUTEX_UNLOCK, // Liberação de mutex (MUxx:tt)
    EVENT_IO_START      // Início de operação de I/O (IO:tt-dd)
} EventType;

// =============================================================================
// ESTRUTURAS DE DADOS
// =============================================================================

/**
 * Estrutura para representar um evento de uma tarefa.
 * Usada para mutex e I/O no Projeto B.
 */
typedef struct {
    EventType type;     // Tipo do evento
    int time;           // Instante relativo ao início da tarefa
    int param;          // Parâmetro adicional (ID do mutex ou duração do I/O)
} TaskEvent;

/**
 * Task Control Block (TCB) - Estrutura principal de cada tarefa.
 * Contém todas as informações necessárias para gerenciar a tarefa.
 */
typedef struct {
    // Identificação
    int id;                     // ID único da tarefa
    char color[8];              // Cor em hexadecimal (#RRGGBB)
    
    // Parâmetros de tempo
    int arrival_time;           // Instante de chegada ao sistema
    int burst_time;             // Tempo total de CPU necessário
    int remaining_time;         // Tempo restante de CPU
    int priority;               // Prioridade (menor = mais prioritário)
    
    // Estado atual
    TaskState state;            // Estado atual da tarefa
    
    // Estatísticas (atualizadas durante/após simulação)
    int start_time;             // Primeiro instante de execução (-1 se não iniciou)
    int completion_time;        // Instante de término
    int turnaround_time;        // Tempo total no sistema (completion - arrival)
    int waiting_time;           // Tempo esperando na fila de prontos
    int response_time;          // Tempo até primeira execução (start - arrival)
    
    // Controle de quantum (para RR)
    int quantum_remaining;      // Ticks restantes do quantum atual
    
    // Eventos (Projeto B)
    TaskEvent events[MAX_EVENTS];
    int event_count;
    int next_event_idx;         // Índice do próximo evento a processar
    
    // I/O (Projeto B)
    int io_remaining;           // Tempo restante de I/O (0 = não está em I/O)
} TCB;

/**
 * Configuração da simulação carregada do arquivo.
 */
typedef struct {
    char algorithm[20];         // Nome do algoritmo (FIFO, RR, SRTF, PRIORITY)
    int quantum;                // Quantum para RR
    int alpha;                  // Parâmetro alpha para envelhecimento (Projeto B)
    TCB* tasks;                 // Array de tarefas
    int task_count;             // Número de tarefas
} SimConfig;

/**
 * Relógio do sistema simulado.
 */
typedef struct {
    int current_tick;           // Tick atual da simulação
    int quantum_size;           // Tamanho do quantum configurado
} SystemClock;

/**
 * Snapshot do estado do sistema em um determinado tick.
 * Usado para implementar o retrocesso da simulação (req. 1.5.2).
 */
typedef struct {
    int tick;                   // Tick deste snapshot
    TCB* tasks;                 // Cópia do array de tarefas
    int task_count;             // Número de tarefas
    int current_task_id;        // ID da tarefa em execução (-1 se nenhuma)
    GanttEntry* gantt_entries;  // Entradas do Gantt até este ponto
    int gantt_count;            // Número de entradas do Gantt
} Snapshot;

/**
 * Estrutura principal do simulador.
 */
typedef struct {
    // Estado atual
    SystemClock clock;          // Relógio do sistema
    TCB* tasks;                 // Array de tarefas
    int task_count;             // Número de tarefas
    TCB* current_task;          // Ponteiro para tarefa em execução
    char algorithm[20];         // Algoritmo de escalonamento
    
    // Gantt Chart
    GanttEntry* gantt_entries;  // Entradas para o gráfico de Gantt
    int gantt_count;            // Número de entradas
    int gantt_capacity;         // Capacidade alocada
    
    // Sistema de histórico (para retrocesso)
    Snapshot* history;          // Array de snapshots
    int history_count;          // Número de snapshots salvos
    int history_capacity;       // Capacidade alocada
    
    // Controle
    bool verbose;               // Modo verboso (imprime cada tick)
} Simulator;

// =============================================================================
// FUNÇÕES AUXILIARES DE MEMÓRIA
// =============================================================================

/**
 * Cria uma cópia profunda de um array de TCBs.
 */
TCB* copy_tasks(TCB* src, int count) {
    TCB* dst = malloc(count * sizeof(TCB));
    if (dst) {
        memcpy(dst, src, count * sizeof(TCB));
    }
    return dst;
}

/**
 * Cria uma cópia profunda de um array de GanttEntry.
 */
GanttEntry* copy_gantt(GanttEntry* src, int count) {
    if (count == 0) return NULL;
    GanttEntry* dst = malloc(count * sizeof(GanttEntry));
    if (dst) {
        memcpy(dst, src, count * sizeof(GanttEntry));
    }
    return dst;
}

// =============================================================================
// PARSER DE CONFIGURAÇÃO
// =============================================================================

/**
 * Faz o parse de uma lista de eventos de uma tarefa.
 * Formato esperado: "MLxx:tt,MUxx:tt,IO:tt-dd,..."
 * 
 * Esta função prepara a estrutura para o Projeto B.
 */
void parse_events(TCB* task, const char* event_str) {
    task->event_count = 0;
    task->next_event_idx = 0;
    
    if (!event_str || strlen(event_str) == 0) return;
    
    char buffer[256];
    strncpy(buffer, event_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* token = strtok(buffer, ",");
    while (token && task->event_count < MAX_EVENTS) {
        TaskEvent* ev = &task->events[task->event_count];
        
        if (strncmp(token, "ML", 2) == 0) {
            // Mutex Lock: MLxx:tt
            ev->type = EVENT_MUTEX_LOCK;
            sscanf(token, "ML%d:%d", &ev->param, &ev->time);
            task->event_count++;
        }
        else if (strncmp(token, "MU", 2) == 0) {
            // Mutex Unlock: MUxx:tt
            ev->type = EVENT_MUTEX_UNLOCK;
            sscanf(token, "MU%d:%d", &ev->param, &ev->time);
            task->event_count++;
        }
        else if (strncmp(token, "IO", 2) == 0) {
            // I/O: IO:tt-dd
            ev->type = EVENT_IO_START;
            sscanf(token, "IO:%d-%d", &ev->time, &ev->param);
            task->event_count++;
        }
        
        token = strtok(NULL, ",");
    }
}

/**
 * Carrega a configuração da simulação a partir de um arquivo texto.
 * 
 * Formato do arquivo:
 *   Linha 1: algoritmo;quantum[;alpha]
 *   Linhas seguintes: id;cor;ingresso;duracao;prioridade;[eventos]
 * 
 * @param filename Nome do arquivo de configuração
 * @return Ponteiro para SimConfig ou NULL em caso de erro
 */
SimConfig* parse_config(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("Erro: Não foi possível abrir o arquivo '%s'\n", filename);
        return NULL;
    }
    
    SimConfig* config = malloc(sizeof(SimConfig));
    if (!config) {
        fclose(f);
        return NULL;
    }
    
    // Inicializar valores padrão
    config->alpha = 1;
    config->quantum = 10;
    strcpy(config->algorithm, "FIFO");
    
    char line[MAX_LINE_LEN];
    
    // Primeira linha: parâmetros do sistema
    if (!fgets(line, sizeof(line), f)) {
        printf("Erro: Arquivo de configuração vazio\n");
        fclose(f);
        free(config);
        return NULL;
    }
    
    // Remover newline
    line[strcspn(line, "\r\n")] = '\0';
    
    // Parse da primeira linha: algoritmo;quantum[;alpha]
    char* tok = strtok(line, ";");
    if (tok) strncpy(config->algorithm, tok, sizeof(config->algorithm) - 1);
    
    tok = strtok(NULL, ";");
    if (tok) config->quantum = atoi(tok);
    
    tok = strtok(NULL, ";");
    if (tok) config->alpha = atoi(tok);
    
    // Alocar array de tarefas
    int capacity = 10;
    config->tasks = malloc(capacity * sizeof(TCB));
    config->task_count = 0;
    
    // Ler tarefas
    while (fgets(line, sizeof(line), f)) {
        // Remover newline
        line[strcspn(line, "\r\n")] = '\0';
        
        // Pular linhas vazias ou comentários
        if (strlen(line) < 3 || line[0] == '#') continue;
        
        // Expandir array se necessário
        if (config->task_count >= capacity) {
            capacity *= 2;
            config->tasks = realloc(config->tasks, capacity * sizeof(TCB));
        }
        
        TCB* task = &config->tasks[config->task_count];
        
        // Inicializar tarefa
        memset(task, 0, sizeof(TCB));
        task->start_time = -1;
        task->state = STATE_NEW;
        
        // Parse: id;cor;ingresso;duracao;prioridade;eventos
        char events_str[256] = "";
        int parsed = sscanf(line, "%d;%7[^;];%d;%d;%d;%255[^\n]",
                           &task->id,
                           task->color,
                           &task->arrival_time,
                           &task->burst_time,
                           &task->priority,
                           events_str);
        
        if (parsed >= 5) {
            task->remaining_time = task->burst_time;
            task->quantum_remaining = config->quantum;
            
            // Parse eventos (para Projeto B)
            if (strlen(events_str) > 0) {
                parse_events(task, events_str);
            }
            
            config->task_count++;
        }
    }
    
    fclose(f);
    
    if (config->task_count == 0) {
        printf("Erro: Nenhuma tarefa encontrada no arquivo\n");
        free(config->tasks);
        free(config);
        return NULL;
    }
    
    printf("Configuração carregada: %s, quantum=%d, %d tarefas\n",
           config->algorithm, config->quantum, config->task_count);
    
    return config;
}

// =============================================================================
// GERENCIAMENTO DO SIMULADOR
// =============================================================================

/**
 * Cria e inicializa uma nova instância do simulador.
 */
Simulator* create_simulator(SimConfig* config) {
    Simulator* sim = malloc(sizeof(Simulator));
    if (!sim) return NULL;
    
    // Inicializar relógio
    sim->clock.current_tick = 0;
    sim->clock.quantum_size = config->quantum;
    
    // Copiar tarefas
    sim->task_count = config->task_count;
    sim->tasks = malloc(sim->task_count * sizeof(TCB));
    memcpy(sim->tasks, config->tasks, sim->task_count * sizeof(TCB));
    
    // Inicializar quantum das tarefas
    for (int i = 0; i < sim->task_count; i++) {
        sim->tasks[i].quantum_remaining = config->quantum;
    }
    
    // Copiar algoritmo
    strncpy(sim->algorithm, config->algorithm, sizeof(sim->algorithm) - 1);
    sim->current_task = NULL;
    
    // Inicializar Gantt
    sim->gantt_capacity = 1000;
    sim->gantt_entries = malloc(sim->gantt_capacity * sizeof(GanttEntry));
    sim->gantt_count = 0;
    
    // Inicializar histórico
    sim->history_capacity = MAX_HISTORY;
    sim->history = malloc(sim->history_capacity * sizeof(Snapshot));
    sim->history_count = 0;
    
    sim->verbose = true;
    
    return sim;
}

/**
 * Libera toda a memória alocada pelo simulador.
 */
void destroy_simulator(Simulator* sim) {
    if (!sim) return;
    
    // Liberar histórico
    for (int i = 0; i < sim->history_count; i++) {
        free(sim->history[i].tasks);
        free(sim->history[i].gantt_entries);
    }
    free(sim->history);
    
    free(sim->tasks);
    free(sim->gantt_entries);
    free(sim);
}

// =============================================================================
// SISTEMA DE HISTÓRICO (REQUISITO 1.5.2)
// =============================================================================

/**
 * Salva um snapshot do estado atual do sistema.
 * Permite retroceder a simulação posteriormente.
 */
void save_snapshot(Simulator* sim) {
    if (sim->history_count >= sim->history_capacity) {
        // Histórico cheio - remover snapshot mais antigo
        free(sim->history[0].tasks);
        free(sim->history[0].gantt_entries);
        memmove(&sim->history[0], &sim->history[1], 
                (sim->history_count - 1) * sizeof(Snapshot));
        sim->history_count--;
    }
    
    Snapshot* snap = &sim->history[sim->history_count];
    
    snap->tick = sim->clock.current_tick;
    snap->tasks = copy_tasks(sim->tasks, sim->task_count);
    snap->task_count = sim->task_count;
    snap->current_task_id = sim->current_task ? sim->current_task->id : -1;
    snap->gantt_entries = copy_gantt(sim->gantt_entries, sim->gantt_count);
    snap->gantt_count = sim->gantt_count;
    
    sim->history_count++;
}

/**
 * Restaura o estado do sistema para um snapshot específico.
 * 
 * @param sim Simulador
 * @param target_tick Tick para o qual retroceder
 * @return true se conseguiu retroceder, false caso contrário
 */
bool restore_snapshot(Simulator* sim, int target_tick) {
    // Encontrar o snapshot mais próximo do tick desejado
    int best_idx = -1;
    for (int i = sim->history_count - 1; i >= 0; i--) {
        if (sim->history[i].tick <= target_tick) {
            best_idx = i;
            break;
        }
    }
    
    if (best_idx < 0) {
        printf("Erro: Não há histórico para o tick %d\n", target_tick);
        return false;
    }
    
    Snapshot* snap = &sim->history[best_idx];
    
    // Restaurar estado
    sim->clock.current_tick = snap->tick;
    memcpy(sim->tasks, snap->tasks, snap->task_count * sizeof(TCB));
    
    // Restaurar current_task
    sim->current_task = NULL;
    if (snap->current_task_id >= 0) {
        for (int i = 0; i < sim->task_count; i++) {
            if (sim->tasks[i].id == snap->current_task_id) {
                sim->current_task = &sim->tasks[i];
                break;
            }
        }
    }
    
    // Restaurar Gantt
    sim->gantt_count = snap->gantt_count;
    if (snap->gantt_count > 0) {
        memcpy(sim->gantt_entries, snap->gantt_entries, 
               snap->gantt_count * sizeof(GanttEntry));
    }
    
    // Remover snapshots posteriores
    for (int i = best_idx + 1; i < sim->history_count; i++) {
        free(sim->history[i].tasks);
        free(sim->history[i].gantt_entries);
    }
    sim->history_count = best_idx + 1;
    
    printf("Estado restaurado para o tick %d\n", snap->tick);
    return true;
}

// =============================================================================
// GANTT CHART
// =============================================================================

/**
 * Adiciona uma entrada ao registro do Gantt.
 */
void add_gantt_entry(Simulator* sim, int task_id, int start, int end, const char* color) {
    // Expandir array se necessário
    if (sim->gantt_count >= sim->gantt_capacity) {
        sim->gantt_capacity *= 2;
        sim->gantt_entries = realloc(sim->gantt_entries,
                                     sim->gantt_capacity * sizeof(GanttEntry));
    }
    
    GanttEntry* entry = &sim->gantt_entries[sim->gantt_count++];
    entry->task_id = task_id;
    entry->start_time = start;
    entry->end_time = end;
    strncpy(entry->color, color, sizeof(entry->color) - 1);
}

// =============================================================================
// ALGORITMOS DE ESCALONAMENTO
// =============================================================================

/**
 * Verifica se todas as tarefas terminaram.
 */
bool all_tasks_completed(Simulator* sim) {
    for (int i = 0; i < sim->task_count; i++) {
        if (sim->tasks[i].state != STATE_TERMINATED) {
            return false;
        }
    }
    return true;
}

/**
 * FIFO (First In First Out) - Escalonamento por ordem de chegada.
 * Não preemptivo: a tarefa executa até terminar.
 */
TCB* schedule_fifo(Simulator* sim) {
    // Se há uma tarefa rodando e ela não terminou, continua com ela
    if (sim->current_task && 
        sim->current_task->state == STATE_RUNNING &&
        sim->current_task->remaining_time > 0) {
        return sim->current_task;
    }
    
    // Procurar a tarefa com menor tempo de chegada entre as prontas
    TCB* next = NULL;
    int earliest_arrival = INT_MAX;
    
    for (int i = 0; i < sim->task_count; i++) {
        TCB* task = &sim->tasks[i];
        
        if (task->arrival_time <= sim->clock.current_tick &&
            task->state != STATE_TERMINATED &&
            task->state != STATE_BLOCKED) {
            
            if (task->arrival_time < earliest_arrival) {
                earliest_arrival = task->arrival_time;
                next = task;
            }
        }
    }
    
    return next;
}

/**
 * Round-Robin (RR) - Escalonamento circular com quantum.
 * Preemptivo: a tarefa perde a CPU quando seu quantum expira.
 */
TCB* schedule_rr(Simulator* sim) {
    // Se há uma tarefa rodando com quantum restante, continua com ela
    if (sim->current_task && 
        sim->current_task->state == STATE_RUNNING &&
        sim->current_task->remaining_time > 0 &&
        sim->current_task->quantum_remaining > 0) {
        return sim->current_task;
    }
    
    // Encontrar o índice da tarefa atual na lista
    int current_idx = -1;
    if (sim->current_task) {
        for (int i = 0; i < sim->task_count; i++) {
            if (&sim->tasks[i] == sim->current_task) {
                current_idx = i;
                break;
            }
        }
    }
    
    // Procurar próxima tarefa em ordem circular
    for (int offset = 1; offset <= sim->task_count; offset++) {
        int idx = (current_idx + offset) % sim->task_count;
        if (idx < 0) idx = 0;
        
        TCB* task = &sim->tasks[idx];
        
        if (task->arrival_time <= sim->clock.current_tick &&
            task->state != STATE_TERMINATED &&
            task->state != STATE_BLOCKED &&
            task->remaining_time > 0) {
            
            // Resetar quantum para nova tarefa
            task->quantum_remaining = sim->clock.quantum_size;
            return task;
        }
    }
    
    return NULL;
}

/**
 * SRTF (Shortest Remaining Time First) - Menor tempo restante primeiro.
 * Preemptivo: uma nova tarefa pode preemptar se tiver menor tempo restante.
 */
TCB* schedule_srtf(Simulator* sim) {
    TCB* next = NULL;
    int shortest_remaining = INT_MAX;
    
    for (int i = 0; i < sim->task_count; i++) {
        TCB* task = &sim->tasks[i];
        
        if (task->arrival_time <= sim->clock.current_tick &&
            task->state != STATE_TERMINATED &&
            task->state != STATE_BLOCKED &&
            task->remaining_time > 0) {
            
            if (task->remaining_time < shortest_remaining) {
                shortest_remaining = task->remaining_time;
                next = task;
            }
        }
    }
    
    return next;
}

/**
 * PRIORITY - Escalonamento por prioridade (menor valor = maior prioridade).
 * Preemptivo: uma tarefa de maior prioridade pode preemptar.
 */
TCB* schedule_priority(Simulator* sim) {
    TCB* next = NULL;
    int highest_priority = INT_MAX;
    
    for (int i = 0; i < sim->task_count; i++) {
        TCB* task = &sim->tasks[i];
        
        if (task->arrival_time <= sim->clock.current_tick &&
            task->state != STATE_TERMINATED &&
            task->state != STATE_BLOCKED &&
            task->remaining_time > 0) {
            
            if (task->priority < highest_priority) {
                highest_priority = task->priority;
                next = task;
            }
        }
    }
    
    return next;
}

/**
 * Seleciona o escalonador apropriado baseado no algoritmo configurado.
 */
TCB* schedule(Simulator* sim) {
    if (strcmp(sim->algorithm, "FIFO") == 0) {
        return schedule_fifo(sim);
    } else if (strcmp(sim->algorithm, "RR") == 0) {
        return schedule_rr(sim);
    } else if (strcmp(sim->algorithm, "SRTF") == 0) {
        return schedule_srtf(sim);
    } else if (strcmp(sim->algorithm, "PRIORITY") == 0) {
        return schedule_priority(sim);
    }
    
    // Algoritmo desconhecido - usar FIFO como padrão
    printf("Aviso: Algoritmo '%s' desconhecido, usando FIFO\n", sim->algorithm);
    return schedule_fifo(sim);
}

// =============================================================================
// SIMULAÇÃO
// =============================================================================

/**
 * Executa um tick da simulação.
 * 
 * Ordem das operações:
 * 1. Salvar snapshot para histórico (se necessário)
 * 2. Processar chegadas de tarefas
 * 3. Selecionar próxima tarefa (escalonador)
 * 4. Realizar troca de contexto se necessário
 * 5. Executar tarefa atual (decrementar remaining_time)
 * 6. Atualizar Gantt
 * 7. Verificar conclusão de tarefas
 * 8. Incrementar tick
 */
void simulate_tick(Simulator* sim) {
    // Salvar snapshot para permitir retrocesso
    save_snapshot(sim);
    
    // 1. Processar chegadas de novas tarefas
    for (int i = 0; i < sim->task_count; i++) {
        TCB* task = &sim->tasks[i];
        if (task->arrival_time == sim->clock.current_tick && 
            task->state == STATE_NEW) {
            task->state = STATE_READY;
            if (sim->verbose) {
                printf("[Tick %3d] Tarefa %d chegou ao sistema\n",
                       sim->clock.current_tick, task->id);
            }
        }
    }
    
    // 2. Selecionar próxima tarefa
    TCB* next_task = schedule(sim);
    
    // 3. Troca de contexto se necessário
    if (next_task != sim->current_task) {
        // Colocar tarefa atual de volta na fila de prontos (se ainda não terminou)
        if (sim->current_task && 
            sim->current_task->state == STATE_RUNNING &&
            sim->current_task->remaining_time > 0) {
            sim->current_task->state = STATE_READY;
            if (sim->verbose) {
                printf("[Tick %3d] Tarefa %d preemptada\n",
                       sim->clock.current_tick, sim->current_task->id);
            }
        }
        
        // Iniciar nova tarefa
        if (next_task) {
            // Registrar tempo de resposta (primeira execução)
            if (next_task->start_time == -1) {
                next_task->start_time = sim->clock.current_tick;
                next_task->response_time = next_task->start_time - next_task->arrival_time;
            }
            next_task->state = STATE_RUNNING;
            
            if (sim->verbose) {
                printf("[Tick %3d] Executando tarefa %d (restam %d ticks)\n",
                       sim->clock.current_tick, next_task->id, next_task->remaining_time);
            }
        }
        
        sim->current_task = next_task;
    }
    
    // 4. Executar tarefa atual
    if (sim->current_task) {
        sim->current_task->remaining_time--;
        
        // Decrementar quantum (para RR)
        if (strcmp(sim->algorithm, "RR") == 0) {
            sim->current_task->quantum_remaining--;
        }
        
        // 5. Atualizar Gantt
        // Verificar se podemos estender a última entrada ou precisamos criar nova
        if (sim->gantt_count > 0 &&
            sim->gantt_entries[sim->gantt_count - 1].task_id == sim->current_task->id &&
            sim->gantt_entries[sim->gantt_count - 1].end_time == sim->clock.current_tick) {
            // Estender entrada existente
            sim->gantt_entries[sim->gantt_count - 1].end_time++;
        } else {
            // Criar nova entrada
            add_gantt_entry(sim, sim->current_task->id,
                          sim->clock.current_tick,
                          sim->clock.current_tick + 1,
                          sim->current_task->color);
        }
        
        // 6. Verificar se a tarefa terminou
        if (sim->current_task->remaining_time == 0) {
            sim->current_task->state = STATE_TERMINATED;
            sim->current_task->completion_time = sim->clock.current_tick + 1;
            sim->current_task->turnaround_time = 
                sim->current_task->completion_time - sim->current_task->arrival_time;
            sim->current_task->waiting_time = 
                sim->current_task->turnaround_time - sim->current_task->burst_time;
            
            if (sim->verbose) {
                printf("[Tick %3d] Tarefa %d concluída (turnaround: %d, waiting: %d)\n",
                       sim->clock.current_tick, sim->current_task->id,
                       sim->current_task->turnaround_time,
                       sim->current_task->waiting_time);
            }
            
            sim->current_task = NULL;
        }
    }
    
    // 7. Incrementar tick
    sim->clock.current_tick++;
}

// =============================================================================
// MODOS DE EXECUÇÃO
// =============================================================================

/**
 * Execução completa sem intervenção humana (req. 1.5.3).
 */
void run_complete(Simulator* sim) {
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║           SIMULAÇÃO - Algoritmo: %-10s                  ║\n", sim->algorithm);
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    
    while (!all_tasks_completed(sim)) {
        simulate_tick(sim);
    }
    
    printf("\n✓ Simulação concluída em %d ticks\n", sim->clock.current_tick);
}

/**
 * Imprime o estado atual do sistema (para modo debug).
 */
void print_system_state(Simulator* sim) {
    printf("\n┌─────────────────────────────────────────────────────────┐\n");
    printf("│ ESTADO DO SISTEMA - Tick: %-4d                          │\n", 
           sim->clock.current_tick);
    printf("├─────────────────────────────────────────────────────────┤\n");
    printf("│ ID │ Estado     │ Chegada │ Burst │ Restante │ Prior. │\n");
    printf("├────┼────────────┼─────────┼───────┼──────────┼────────┤\n");
    
    for (int i = 0; i < sim->task_count; i++) {
        TCB* t = &sim->tasks[i];
        const char* state_str;
        switch (t->state) {
            case STATE_NEW:        state_str = "NEW       "; break;
            case STATE_READY:      state_str = "READY     "; break;
            case STATE_RUNNING:    state_str = "►RUNNING  "; break;
            case STATE_BLOCKED:    state_str = "BLOCKED   "; break;
            case STATE_TERMINATED: state_str = "TERMINATED"; break;
            default:               state_str = "???       ";
        }
        printf("│ %2d │ %s │ %7d │ %5d │ %8d │ %6d │\n",
               t->id, state_str, t->arrival_time, t->burst_time,
               t->remaining_time, t->priority);
    }
    printf("└─────────────────────────────────────────────────────────┘\n");
}

/**
 * Execução passo-a-passo com depuração (req. 1.5.1 e 1.5.2).
 */
void run_step_by_step(Simulator* sim) {
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║              MODO DEBUG - PASSO A PASSO                      ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ Comandos:                                                    ║\n");
    printf("║   [Enter] - Avançar um tick                                  ║\n");
    printf("║   [n]     - Avançar N ticks                                  ║\n");
    printf("║   [b]     - Retroceder um tick                               ║\n");
    printf("║   [g]     - Ir para tick específico                          ║\n");
    printf("║   [i]     - Inspecionar estado do sistema                    ║\n");
    printf("║   [c]     - Continuar até o fim                              ║\n");
    printf("║   [q]     - Sair                                             ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    
    char cmd[20];
    sim->verbose = true;
    
    // Mostrar estado inicial
    print_system_state(sim);
    
    while (!all_tasks_completed(sim)) {
        printf("\n[Tick %d] Comando: ", sim->clock.current_tick);
        
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
            printf("\nEntrada encerrada. Saindo.\n");
            break;
        }
        
        // Remover newline
        cmd[strcspn(cmd, "\r\n")] = '\0';
        
        if (strlen(cmd) == 0 || cmd[0] == '\n') {
            // Enter - avançar um tick
            simulate_tick(sim);
        }
        else if (cmd[0] == 'q' || cmd[0] == 'Q') {
            printf("Saindo do modo debug.\n");
            break;
        }
        else if (cmd[0] == 'c' || cmd[0] == 'C') {
            printf("Continuando execução...\n");
            sim->verbose = true;
            while (!all_tasks_completed(sim)) {
                simulate_tick(sim);
            }
            break;
        }
        else if (cmd[0] == 'i' || cmd[0] == 'I') {
            print_system_state(sim);
        }
        else if (cmd[0] == 'b' || cmd[0] == 'B') {
            // Retroceder um tick
            int target = sim->clock.current_tick - 1;
            if (target >= 0) {
                restore_snapshot(sim, target);
                print_system_state(sim);
            } else {
                printf("Já está no início da simulação.\n");
            }
        }
        else if (cmd[0] == 'g' || cmd[0] == 'G') {
            printf("Ir para qual tick? ");
            int target;
            if (scanf("%d", &target) == 1) {
                getchar(); // Consumir newline
                if (target < sim->clock.current_tick) {
                    restore_snapshot(sim, target);
                } else {
                    // Avançar até o tick desejado
                    while (sim->clock.current_tick < target && !all_tasks_completed(sim)) {
                        simulate_tick(sim);
                    }
                }
                print_system_state(sim);
            }
        }
        else if (cmd[0] == 'n' || cmd[0] == 'N') {
            printf("Quantos ticks? ");
            int n;
            if (scanf("%d", &n) == 1) {
                getchar(); // Consumir newline
                for (int i = 0; i < n && !all_tasks_completed(sim); i++) {
                    simulate_tick(sim);
                }
            }
        }
        else {
            printf("Comando desconhecido. Use: Enter, n, b, g, i, c, ou q\n");
        }
    }
    
    if (all_tasks_completed(sim)) {
        printf("\n✓ Todas as tarefas concluídas!\n");
    }
}

// =============================================================================
// ESTATÍSTICAS E SAÍDA
// =============================================================================

/**
 * Imprime estatísticas simples no console.
 */
void print_statistics_simple(Simulator* sim) {
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    ESTATÍSTICAS FINAIS                       ║\n");
    printf("╠════╦═════════╦═══════╦══════════╦════════════╦═══════════════╣\n");
    printf("║ ID ║ Chegada ║ Burst ║ Término  ║ Turnaround ║ Tempo Espera  ║\n");
    printf("╠════╬═════════╬═══════╬══════════╬════════════╬═══════════════╣\n");
    
    float avg_turnaround = 0;
    float avg_waiting = 0;
    float avg_response = 0;
    
    for (int i = 0; i < sim->task_count; i++) {
        TCB* t = &sim->tasks[i];
        printf("║ %2d ║ %7d ║ %5d ║ %8d ║ %10d ║ %13d ║\n",
               t->id, t->arrival_time, t->burst_time,
               t->completion_time, t->turnaround_time, t->waiting_time);
        
        avg_turnaround += t->turnaround_time;
        avg_waiting += t->waiting_time;
        avg_response += t->response_time;
    }
    
    avg_turnaround /= sim->task_count;
    avg_waiting /= sim->task_count;
    avg_response /= sim->task_count;
    
    printf("╠════╩═════════╩═══════╩══════════╩════════════╩═══════════════╣\n");
    printf("║ Médias:  Turnaround = %6.2f  |  Waiting = %6.2f           ║\n",
           avg_turnaround, avg_waiting);
    printf("║          Response = %6.2f    |  Throughput = %5.3f tasks/tick ║\n",
           avg_response, (float)sim->task_count / sim->clock.current_tick);
    printf("╚══════════════════════════════════════════════════════════════╝\n");
}

/**
 * Limpa o buffer de entrada.
 */
void clear_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * Pergunta sim/não ao usuário.
 */
bool ask_yes_no(const char* question) {
    printf("%s (s/n): ", question);
    char c = getchar();
    if (c != '\n') clear_stdin();
    return (c == 's' || c == 'S');
}

// =============================================================================
// FUNÇÃO PRINCIPAL
// =============================================================================

void print_usage(const char* program) {
    printf("Simulador de Escalonamento de Processos v2.0\n\n");
    printf("Uso: %s <arquivo_config> [opções]\n\n", program);
    printf("Opções:\n");
    printf("  --step       Modo passo-a-passo (debug)\n");
    printf("  --bmp        Gerar gráfico BMP automaticamente\n");
    printf("  --ascii      Exibir gráfico ASCII automaticamente\n");
    printf("  --quiet      Não mostrar mensagens de execução\n");
    printf("  --help       Mostrar esta ajuda\n");
    printf("\nFormato do arquivo de configuração:\n");
    printf("  algoritmo;quantum\n");
    printf("  id;cor;chegada;duracao;prioridade;[eventos]\n");
    printf("\nAlgoritmos suportados: FIFO, RR, SRTF, PRIORITY\n");
    printf("\nExemplo:\n");
    printf("  RR;5\n");
    printf("  0;#FF0000;0;10;1;\n");
    printf("  1;#00FF00;2;8;2;\n");
    printf("  2;#0000FF;5;6;3;\n");
}

int main(int argc, char* argv[]) {
    // Verificar argumentos
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse de argumentos
    const char* config_file = NULL;
    bool step_mode = false;
    bool auto_bmp = false;
    bool auto_ascii = false;
    bool quiet = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "--step") == 0) {
            step_mode = true;
        }
        else if (strcmp(argv[i], "--bmp") == 0) {
            auto_bmp = true;
        }
        else if (strcmp(argv[i], "--ascii") == 0) {
            auto_ascii = true;
        }
        else if (strcmp(argv[i], "--quiet") == 0) {
            quiet = true;
        }
        else if (argv[i][0] != '-') {
            config_file = argv[i];
        }
    }
    
    if (!config_file) {
        printf("Erro: Arquivo de configuração não especificado\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Carregar configuração
    SimConfig* config = parse_config(config_file);
    if (!config) {
        return 1;
    }
    
    // Criar simulador
    Simulator* sim = create_simulator(config);
    if (!sim) {
        printf("Erro: Falha ao criar simulador\n");
        free(config->tasks);
        free(config);
        return 1;
    }
    
    sim->verbose = !quiet;
    
    // Executar simulação
    if (step_mode) {
        run_step_by_step(sim);
    } else {
        run_complete(sim);
    }
    
    int max_time = sim->clock.current_tick;
    
    // Estatísticas
    if (!quiet) {
        if (!auto_bmp && !auto_ascii) {
            // Modo interativo
            if (ask_yes_no("\nExibir estatísticas detalhadas?")) {
                TaskStats* stats = malloc(sim->task_count * sizeof(TaskStats));
                for (int i = 0; i < sim->task_count; i++) {
                    TCB* t = &sim->tasks[i];
                    stats[i].id = t->id;
                    stats[i].arrival = t->arrival_time;
                    stats[i].burst = t->burst_time;
                    stats[i].completion = t->completion_time;
                    stats[i].turnaround = t->turnaround_time;
                    stats[i].waiting = t->waiting_time;
                    stats[i].response = t->response_time;
                    stats[i].priority = t->priority;
                }
                show_statistics(stats, sim->task_count, sim->algorithm);
                
                if (ask_yes_no("Exportar estatísticas para CSV?")) {
                    export_to_csv(stats, sim->task_count, sim->algorithm);
                }
                free(stats);
            } else {
                print_statistics_simple(sim);
            }
        } else {
            print_statistics_simple(sim);
        }
    }
    
    // Gantt ASCII
    if (auto_ascii || (!quiet && !auto_bmp && ask_yes_no("\nExibir Gantt Chart ASCII?"))) {
        print_gantt_ascii(sim->gantt_entries, sim->gantt_count, max_time, sim->task_count);
    }
    
    // Gantt BMP
    if (auto_bmp || (!quiet && ask_yes_no("\nGerar gráfico de Gantt (BMP)?"))) {
        create_gantt_bmp("gantt_output.bmp", sim->gantt_entries,
                        sim->gantt_count, max_time, sim->task_count);
    }
    
    // Liberar memória
    destroy_simulator(sim);
    free(config->tasks);
    free(config);
    
    return 0;
}