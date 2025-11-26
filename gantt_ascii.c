/*
 * gantt_ascii.c - Visualização ASCII do Gantt Chart
 * --------------------------------------------------
 * Este módulo gera representações em texto do gráfico de Gantt
 * para visualização no terminal.
 *
 * Funcionalidades:
 *  - Versão colorida com códigos ANSI
 *  - Versão simples para terminais sem suporte a cores
 *  - Exportação para arquivo texto
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gantt_ascii.h"

/* ============================================================================
 * CÓDIGOS DE CORES ANSI PARA TERMINAL
 * ============================================================================ */

#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#define ANSI_BOLD    "\033[1m"

/* ============================================================================
 * FUNÇÕES AUXILIARES (STATIC - locais ao arquivo)
 * ============================================================================ */

/**
 * Mapeia uma cor hexadecimal para código ANSI correspondente.
 * Usado para colorir a saída no terminal.
 */
static const char* ascii_get_ansi_color(const char* hex) {
    if (!hex) return ANSI_WHITE;

    /* Verificar cores principais */
    if (strstr(hex, "FF0000") || strstr(hex, "ff0000")) return ANSI_RED;
    if (strstr(hex, "00FF00") || strstr(hex, "00ff00")) return ANSI_GREEN;
    if (strstr(hex, "0000FF") || strstr(hex, "0000ff")) return ANSI_BLUE;
    if (strstr(hex, "FFFF00") || strstr(hex, "ffff00")) return ANSI_YELLOW;
    if (strstr(hex, "FF00FF") || strstr(hex, "ff00ff")) return ANSI_MAGENTA;
    if (strstr(hex, "00FFFF") || strstr(hex, "00ffff")) return ANSI_CYAN;

    return ANSI_WHITE;
}

/* ============================================================================
 * FUNÇÕES PÚBLICAS
 * ============================================================================ */

/**
 * Imprime o gráfico de Gantt no terminal com cores ANSI.
 *
 * @param entries      Array de entradas do Gantt
 * @param entry_count  Número de entradas
 * @param total_time   Tempo total da simulação
 * @param task_count   Número de tarefas
 */
void print_gantt_ascii(GanttEntry* entries, int entry_count,
                       int total_time, int task_count) {

    if (task_count <= 0 || total_time <= 0) {
        printf("Erro: Dados inválidos para o Gantt Chart\n");
        return;
    }

    /* Criar matriz de execução */
    char** matrix = malloc(task_count * sizeof(char*));
    char** colors = malloc(task_count * sizeof(char*));

    if (!matrix || !colors) {
        printf("Erro: Falha ao alocar memória\n");
        return;
    }

    for (int i = 0; i < task_count; i++) {
        matrix[i] = calloc(total_time + 1, sizeof(char));
        colors[i] = calloc(total_time * 10, sizeof(char));
        if (matrix[i]) memset(matrix[i], ' ', total_time);
    }

    /* Preencher matriz com execuções */
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].task_id >= 0 && entries[i].task_id < task_count) {
            for (int t = entries[i].start_time; t < entries[i].end_time && t < total_time; t++) {
                matrix[entries[i].task_id][t] = '#';
                strncpy(&colors[entries[i].task_id][t * 10], entries[i].color, 7);
            }
        }
    }

    /* Cabeçalho */
    printf("\n");
    printf("             " ANSI_BOLD "GANTT CHART - SIMULAÇÃO DE ESCALONAMENTO" ANSI_RESET "              \n");
    printf("\n");

    /* Limitar exibição a 60 ticks */
    int display_time = (total_time < 60) ? total_time : 60;

    /* Escala de tempo (primeira linha - dezenas) */
    printf("      ");
    for (int t = 0; t < display_time; t++) {
        if (t % 10 == 0) {
            printf("%2d        ", t);
        }
    }
    printf("\n");

    /* Escala de tempo (segunda linha - unidades) */
    printf("Time  ");
    for (int t = 0; t < display_time; t++) {
        printf("%d", t % 10);
    }
    printf("\n");

    /* Linha divisória */
    printf("      ");
    for (int t = 0; t < display_time; t++) {
        printf("─");
    }
    printf("\n");

    /* Tarefas */
    for (int i = 0; i < task_count; i++) {
        printf("T%-3d  ", i);

        for (int t = 0; t < display_time; t++) {
            if (matrix[i][t] == '#') {
                const char* color = ascii_get_ansi_color(&colors[i][t * 10]);
                printf("%s█" ANSI_RESET, color);
            } else {
                printf("·");
            }
        }

        /* Mostrar tempo de execução total */
        int exec_time = 0;
        for (int t = 0; t < total_time; t++) {
            if (matrix[i][t] == '#') exec_time++;
        }
        printf("  [%2d ticks]", exec_time);
        printf("\n");
    }

    /* Linha divisória */
    printf("      ");
    for (int t = 0; t < display_time; t++) {
        printf("─");
    }
    printf("\n");

    /* Legenda */
    printf("\n" ANSI_BOLD "Legenda:" ANSI_RESET "\n");
    printf("  █ = Tarefa em execução\n");
    printf("  · = Tarefa não executando\n");

    if (total_time > 60) {
        printf("\n" ANSI_YELLOW "Nota: Mostrando apenas os primeiros 60 ticks" ANSI_RESET "\n");
    }

    /* Estatísticas do Gantt */
    printf("\n" ANSI_BOLD "Estatísticas do Gantt:" ANSI_RESET "\n");

    int total_exec = 0;
    for (int i = 0; i < entry_count; i++) {
        total_exec += (entries[i].end_time - entries[i].start_time);
    }

    float cpu_usage = (total_time > 0) ? (float)total_exec / total_time * 100 : 0;
    printf("  Tempo total: %d ticks\n", total_time);
    printf("  Tempo de CPU usado: %d ticks\n", total_exec);
    printf("  Utilização da CPU: %.1f%%\n", cpu_usage);
    printf("  Número de tarefas: %d\n", task_count);
    printf("  Trocas de contexto: %d\n", (entry_count > 0) ? entry_count - 1 : 0);

    /* Limpar memória */
    for (int i = 0; i < task_count; i++) {
        free(matrix[i]);
        free(colors[i]);
    }
    free(matrix);
    free(colors);
}

/**
 * Imprime uma versão simplificada do Gantt (sem cores ANSI).
 * Compatível com todos os terminais.
 */
void print_gantt_simple(GanttEntry* entries, int entry_count,
                       int total_time, int task_count) {

    printf("\n=== GANTT CHART (ASCII) ===\n\n");

    if (task_count <= 0 || total_time <= 0) {
        printf("Erro: Dados inválidos\n");
        return;
    }

    /* Criar matriz */
    char** matrix = malloc(task_count * sizeof(char*));
    if (!matrix) return;

    for (int i = 0; i < task_count; i++) {
        matrix[i] = calloc(total_time + 1, sizeof(char));
        if (matrix[i]) memset(matrix[i], '.', total_time);
    }

    /* Preencher */
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].task_id >= 0 && entries[i].task_id < task_count) {
            for (int t = entries[i].start_time; t < entries[i].end_time && t < total_time; t++) {
                matrix[entries[i].task_id][t] = '#';
            }
        }
    }

    /* Limitar exibição */
    int display_time = (total_time < 50) ? total_time : 50;

    /* Imprimir tempo */
    printf("     ");
    for (int t = 0; t < display_time; t += 5) {
        printf("%-5d", t);
    }
    printf("\n");

    /* Imprimir tarefas */
    for (int i = 0; i < task_count; i++) {
        printf("T%2d: ", i);
        for (int t = 0; t < display_time; t++) {
            printf("%c", matrix[i][t]);
        }
        printf("\n");
    }

    /* Limpar */
    for (int i = 0; i < task_count; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

/**
 * Salva o gráfico de Gantt em um arquivo texto.
 *
 * @param filename     Nome do arquivo de saída
 * @param entries      Array de entradas do Gantt
 * @param entry_count  Número de entradas
 * @param total_time   Tempo total da simulação
 * @param task_count   Número de tarefas
 */
void save_gantt_text(const char* filename, GanttEntry* entries, int entry_count,
                    int total_time, int task_count) {

    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("Erro: Não foi possível criar o arquivo %s\n", filename);
        return;
    }

    fprintf(f, "GANTT CHART - RELATÓRIO DE EXECUÇÃO\n");
    fprintf(f, "=====================================\n\n");

    /* Criar matriz */
    char** matrix = malloc(task_count * sizeof(char*));
    if (!matrix) {
        fclose(f);
        return;
    }

    for (int i = 0; i < task_count; i++) {
        matrix[i] = calloc(total_time + 1, sizeof(char));
        if (matrix[i]) memset(matrix[i], ' ', total_time);
    }

    /* Preencher */
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].task_id >= 0 && entries[i].task_id < task_count) {
            for (int t = entries[i].start_time; t < entries[i].end_time && t < total_time; t++) {
                matrix[entries[i].task_id][t] = '*';
            }
        }
    }

    /* Escrever escala de tempo */
    fprintf(f, "Time: ");
    for (int t = 0; t < total_time; t++) {
        fprintf(f, "%d", t % 10);
    }
    fprintf(f, "\n");

    /* Escrever tarefas */
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

    /* Detalhes de execução */
    fprintf(f, "DETALHES DE EXECUÇÃO:\n");
    fprintf(f, "---------------------\n");
    for (int i = 0; i < entry_count; i++) {
        fprintf(f, "Tarefa %d: tempo %d-%d (duração: %d)\n",
                entries[i].task_id,
                entries[i].start_time,
                entries[i].end_time,
                entries[i].end_time - entries[i].start_time);
    }

    /* Limpar */
    for (int i = 0; i < task_count; i++) {
        free(matrix[i]);
    }
    free(matrix);

    fclose(f);
    printf("Relatório salvo em: %s\n", filename);
}