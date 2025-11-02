// stats_viewer.c - Visualizador de estatísticas detalhadas
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stats_viewer.h"

#define BOLD    "\033[1m"
#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BLUE    "\033[34m"

// Desenhar barra horizontal
void draw_bar(int value, int max_value, int width, const char* color) {
    int bar_length = (value * width) / max_value;
    if (bar_length > width) bar_length = width;

    printf("%s", color);
    for (int i = 0; i < bar_length; i++) {
        printf("█");
    }
    printf(RESET);
    for (int i = bar_length; i < width; i++) {
        printf("░");
    }
    printf(" %d", value);
}

// Calcular e exibir estatísticas
void show_statistics(TaskStats* tasks, int count, const char* algorithm) {
    printf("\n" BOLD "═══════════════════════════════════════════════════════════\n");
    printf("                  ANÁLISE DE DESEMPENHO - %s\n", algorithm);
    printf("═══════════════════════════════════════════════════════════\n" RESET);

    // Cabeçalho da tabela
    printf("\n" CYAN "┌─────┬─────────┬───────┬──────────┬────────────┬─────────┐\n");
    printf("│ ID  │ Chegada │ Burst │ Término  │ Turnaround │ Espera  │\n");
    printf("├─────┼─────────┼───────┼──────────┼────────────┼─────────┤\n" RESET);

    int total_turnaround = 0;
    int total_waiting = 0;
    int max_turnaround = 0;
    int max_waiting = 0;

    // Dados das tarefas
    for (int i = 0; i < count; i++) {
        printf("│ T%-2d │   %3d   │  %3d  │   %3d    │    %3d     │   %3d   │\n",
               tasks[i].id,
               tasks[i].arrival,
               tasks[i].burst,
               tasks[i].completion,
               tasks[i].turnaround,
               tasks[i].waiting);

        total_turnaround += tasks[i].turnaround;
        total_waiting += tasks[i].waiting;

        if (tasks[i].turnaround > max_turnaround) max_turnaround = tasks[i].turnaround;
        if (tasks[i].waiting > max_waiting) max_waiting = tasks[i].waiting;
    }

    printf(CYAN "└─────┴─────────┴───────┴──────────┴────────────┴─────────┘\n" RESET);

    // Médias
    float avg_turnaround = (float)total_turnaround / count;
    float avg_waiting = (float)total_waiting / count;

    printf("\n" BOLD "MÉTRICAS AGREGADAS:\n" RESET);
    printf("───────────────────\n");
    printf("• Tempo médio de retorno (turnaround): " YELLOW "%.2f" RESET " ticks\n", avg_turnaround);
    printf("• Tempo médio de espera: " YELLOW "%.2f" RESET " ticks\n", avg_waiting);
    printf("• Tempo total de execução: " YELLOW "%d" RESET " ticks\n",
           tasks[count-1].completion);

    // Cálculo de throughput
    float throughput = (float)count / tasks[count-1].completion;
    printf("• Throughput: " YELLOW "%.3f" RESET " tarefas/tick\n", throughput);

    // Gráfico de barras para turnaround
    printf("\n" BOLD "GRÁFICO DE TURNAROUND:\n" RESET);
    printf("──────────────────────\n");
    for (int i = 0; i < count; i++) {
        printf("T%-2d: ", tasks[i].id);
        draw_bar(tasks[i].turnaround, max_turnaround, 30, GREEN);
        printf("\n");
    }

    // Gráfico de barras para waiting
    printf("\n" BOLD "GRÁFICO DE TEMPO DE ESPERA:\n" RESET);
    printf("────────────────────────────\n");
    for (int i = 0; i < count; i++) {
        printf("T%-2d: ", tasks[i].id);
        draw_bar(tasks[i].waiting, max_waiting, 30, CYAN);
        printf("\n");
    }

    // Análise de eficiência
    printf("\n" BOLD "ANÁLISE DE EFICIÊNCIA:\n" RESET);
    printf("──────────────────────\n");

    float cpu_usage = 0;
    for (int i = 0; i < count; i++) {
        cpu_usage += tasks[i].burst;
    }
    cpu_usage = (cpu_usage / tasks[count-1].completion) * 100;

    printf("• Utilização da CPU: ");
    if (cpu_usage > 80) {
        printf(GREEN "%.1f%%" RESET " (Excelente)\n", cpu_usage);
    } else if (cpu_usage > 60) {
        printf(YELLOW "%.1f%%" RESET " (Boa)\n", cpu_usage);
    } else {
        printf(RED "%.1f%%" RESET " (Baixa)\n", cpu_usage);
    }

    // Identificar possíveis problemas
    printf("\n" BOLD "DIAGNÓSTICO:\n" RESET);
    printf("─────────────\n");

    int starvation_detected = 0;
    for (int i = 0; i < count; i++) {
        if (tasks[i].waiting > tasks[i].burst * 3) {
            printf(RED "⚠ " RESET "Possível starvation na tarefa T%d (espera: %d, burst: %d)\n",
                   tasks[i].id, tasks[i].waiting, tasks[i].burst);
            starvation_detected = 1;
        }
    }

    if (!starvation_detected) {
        printf(GREEN "✓" RESET " Nenhum problema de starvation detectado\n");
    }

    // Convoy effect check (para FIFO)
    if (strcmp(algorithm, "FIFO") == 0) {
        int convoy = 0;
        for (int i = 1; i < count; i++) {
            if (tasks[0].burst > tasks[i].burst * 2 && tasks[i].waiting > tasks[i].burst * 2) {
                convoy = 1;
                break;
            }
        }
        if (convoy) {
            printf(YELLOW "⚠" RESET " Possível convoy effect detectado (processo longo bloqueando curtos)\n");
        }
    }

    printf("\n");
}


// Exportar estatísticas para CSV
void export_to_csv(TaskStats* tasks, int count, const char* algorithm) {
    char filename[100];
    sprintf(filename, "stats_%s.csv", algorithm);

    FILE* f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "ID,Arrival,Burst,Priority,Completion,Turnaround,Waiting\n");
    for (int i = 0; i < count; i++) {
        fprintf(f, "%d,%d,%d,%d,%d,%d,%d\n",
                tasks[i].id,
                tasks[i].arrival,
                tasks[i].burst,
                tasks[i].priority,
                tasks[i].completion,
                tasks[i].turnaround,
                tasks[i].waiting);
    }

    fclose(f);
    printf(GREEN "✓" RESET " Estatísticas exportadas para: %s\n", filename);
}

#ifdef STANDALONE_TEST
int main() {
    // Dados de exemplo
    TaskStats tasks[] = {
        {0, 0, 20, 20, 20, 0, 0, 1},
        {1, 5, 15, 35, 30, 15, 5, 2},
        {2, 10, 10, 45, 35, 25, 10, 3}
    };

    show_statistics(tasks, 3, "FIFO");
    export_to_csv(tasks, 3, "FIFO");

    return 0;
}
#endif