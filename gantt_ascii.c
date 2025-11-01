// gantt_ascii.c - Visualização ASCII aprimorada para o Gantt Chart
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gantt_bmp.h"

// Cores ANSI para terminal
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"

// Mapear cores hex para ANSI
const char* get_ansi_color(const char* hex) {
    if (strstr(hex, "FF0000") || strstr(hex, "ff0000")) return RED;
    if (strstr(hex, "00FF00") || strstr(hex, "00ff00")) return GREEN;
    if (strstr(hex, "0000FF") || strstr(hex, "0000ff")) return BLUE;
    if (strstr(hex, "FFFF00") || strstr(hex, "ffff00")) return YELLOW;
    if (strstr(hex, "FF00FF") || strstr(hex, "ff00ff")) return MAGENTA;
    if (strstr(hex, "00FFFF") || strstr(hex, "00ffff")) return CYAN;
    return WHITE;
}

void print_gantt_ascii(GanttEntry* entries, int entry_count, 
                       int total_time, int task_count) {
    
    // Criar matriz de execução
    char** matrix = malloc(task_count * sizeof(char*));
    char** colors = malloc(task_count * sizeof(char*));
    
    for (int i = 0; i < task_count; i++) {
        matrix[i] = calloc(total_time + 1, sizeof(char));
        colors[i] = calloc(total_time * 10, sizeof(char));
        memset(matrix[i], ' ', total_time);
    }
    
    // Preencher matriz com execuções
    for (int i = 0; i < entry_count; i++) {
        for (int t = entries[i].start_time; t < entries[i].end_time && t < total_time; t++) {
            matrix[entries[i].task_id][t] = '#';
            strncpy(&colors[entries[i].task_id][t * 10], entries[i].color, 7);
        }
    }
    
    // Cabeçalho decorado
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║             " BOLD "GANTT CHART - SIMULAÇÃO DE ESCALONAMENTO" RESET "              ║\n");
    printf("╚════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    // Escala de tempo (primeira linha - dezenas)
    printf("      ");
    for (int t = 0; t < total_time && t < 60; t++) {
        if (t % 10 == 0) {
            printf("%2d        ", t);
        }
    }
    printf("\n");
    
    // Escala de tempo (segunda linha - unidades)
    printf("Time  ");
    for (int t = 0; t < total_time && t < 60; t++) {
        printf("%d", t % 10);
    }
    printf("\n");
    
    // Linha divisória
    printf("      ");
    for (int t = 0; t < total_time && t < 60; t++) {
        printf("─");
    }
    printf("\n");
    
    // Tarefas
    for (int i = 0; i < task_count; i++) {
        printf("T%-3d  ", i);
        
        for (int t = 0; t < total_time && t < 60; t++) {
            if (matrix[i][t] == '#') {
                // Obter cor da tarefa
                const char* color = get_ansi_color(&colors[i][t * 10]);
                printf("%s█" RESET, color);
            } else {
                printf("·");
            }
        }
        
        // Mostrar tempo de execução total
        int exec_time = 0;
        for (int t = 0; t < total_time; t++) {
            if (matrix[i][t] == '#') exec_time++;
        }
        printf("  [%2d ticks]", exec_time);
        
        printf("\n");
    }
    
    // Linha divisória
    printf("      ");
    for (int t = 0; t < total_time && t < 60; t++) {
        printf("─");
    }
    printf("\n");
    
    // Legenda
    printf("\n" BOLD "Legenda:" RESET "\n");
    printf("  █ = Tarefa em execução\n");
    printf("  · = Tarefa não executando\n");
    
    if (total_time > 60) {
        printf("\n" YELLOW "Nota: Mostrando apenas os primeiros 60 ticks" RESET "\n");
    }
    
    // Estatísticas do Gantt
    printf("\n" BOLD "Estatísticas do Gantt:" RESET "\n");
    
    // Calcular utilização da CPU
    int total_exec = 0;
    for (int i = 0; i < entry_count; i++) {
        total_exec += (entries[i].end_time - entries[i].start_time);
    }
    
    float cpu_usage = (float)total_exec / total_time * 100;
    printf("  Tempo total: %d ticks\n", total_time);
    printf("  Tempo de CPU usado: %d ticks\n", total_exec);
    printf("  Utilização da CPU: %.1f%%\n", cpu_usage);
    printf("  Número de tarefas: %d\n", task_count);
    printf("  Trocas de contexto: %d\n", entry_count - 1);
    
    // Limpar memória
    for (int i = 0; i < task_count; i++) {
        free(matrix[i]);
        free(colors[i]);
    }
    free(matrix);
    free(colors);
}

// Versão simplificada sem cores (para terminais sem suporte ANSI)
void print_gantt_simple(GanttEntry* entries, int entry_count, 
                       int total_time, int task_count) {
    
    printf("\n=== GANTT CHART (ASCII) ===\n\n");
    
    // Criar matriz
    char** matrix = malloc(task_count * sizeof(char*));
    for (int i = 0; i < task_count; i++) {
        matrix[i] = calloc(total_time + 1, sizeof(char));
        memset(matrix[i], '.', total_time);
    }
    
    // Preencher
    for (int i = 0; i < entry_count; i++) {
        for (int t = entries[i].start_time; t < entries[i].end_time && t < total_time; t++) {
            matrix[entries[i].task_id][t] = '#';
        }
    }
    
    // Imprimir tempo
    printf("     ");
    for (int t = 0; t < total_time && t < 50; t += 5) {
        printf("%-5d", t);
    }
    printf("\n");
    
    // Imprimir tarefas
    for (int i = 0; i < task_count; i++) {
        printf("T%2d: ", i);
        for (int t = 0; t < total_time && t < 50; t++) {
            printf("%c", matrix[i][t]);
        }
        printf("\n");
    }
    
    // Limpar
    for (int i = 0; i < task_count; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

// Função para salvar Gantt em arquivo texto
void save_gantt_text(const char* filename, GanttEntry* entries, int entry_count,
                    int total_time, int task_count) {
    
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "GANTT CHART - RELATÓRIO DE EXECUÇÃO\n");
    fprintf(f, "=====================================\n\n");
    
    // Criar matriz
    char** matrix = malloc(task_count * sizeof(char*));
    for (int i = 0; i < task_count; i++) {
        matrix[i] = calloc(total_time + 1, sizeof(char));
        memset(matrix[i], ' ', total_time);
    }
    
    // Preencher
    for (int i = 0; i < entry_count; i++) {
        for (int t = entries[i].start_time; t < entries[i].end_time; t++) {
            matrix[entries[i].task_id][t] = '*';
        }
    }
    
    // Escrever escala de tempo
    fprintf(f, "Time: ");
    for (int t = 0; t < total_time; t++) {
        fprintf(f, "%d", t % 10);
    }
    fprintf(f, "\n");
    
    // Escrever tarefas
    for (int i = 0; i < task_count; i++) {
        fprintf(f, "T%02d:  ", i);
        for (int t = 0; t < total_time; t++) {
            fprintf(f, "%c", matrix[i][t]);
        }
        fprintf(f, "\n");
    }
    
    fprintf(f, "\n");
    fprintf(f, "Legenda: * = executando, espaço = aguardando\n");
    fprintf(f, "\n");
    
    // Detalhes de execução
    fprintf(f, "DETALHES DE EXECUÇÃO:\n");
    fprintf(f, "---------------------\n");
    for (int i = 0; i < entry_count; i++) {
        fprintf(f, "Tarefa %d: tempo %d-%d (duração: %d)\n",
                entries[i].task_id,
                entries[i].start_time,
                entries[i].end_time,
                entries[i].end_time - entries[i].start_time);
    }
    
    // Limpar
    for (int i = 0; i < task_count; i++) {
        free(matrix[i]);
    }
    free(matrix);
    
    fclose(f);
    printf("Relatório salvo em: %s\n", filename);
}

// Exemplo de uso
#ifdef STANDALONE_TEST
int main() {
    GanttEntry entries[] = {
        {0, 0, 5, "#FF0000"},
        {1, 5, 10, "#00FF00"},
        {0, 10, 15, "#FF0000"},
        {2, 15, 20, "#0000FF"},
        {1, 20, 25, "#00FF00"},
    };
    
    int entry_count = 5;
    int total_time = 30;
    int task_count = 3;
    
    // Versão colorida
    print_gantt_ascii(entries, entry_count, total_time, task_count);
    
    // Versão simples
    print_gantt_simple(entries, entry_count, total_time, task_count);
    
    // Salvar em arquivo
    save_gantt_text("gantt_report.txt", entries, entry_count, total_time, task_count);
    
    return 0;
}
#endif