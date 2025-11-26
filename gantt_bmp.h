/*
 * gantt_bmp.h - Header para geração de gráfico de Gantt em BMP
 *
 * Define a estrutura GanttEntry usada por todo o projeto e
 * declara a função pública de geração do BMP.
 */

#ifndef GANTT_BMP_H
#define GANTT_BMP_H

#include <stdint.h>

/* Estrutura que representa uma entrada no gráfico de Gantt.
 * Cada entrada corresponde a um período contínuo de execução de uma tarefa.
 */
typedef struct {
    int task_id;        /* ID da tarefa */
    int start_time;     /* Tick de início da execução */
    int end_time;       /* Tick de fim da execução */
    char color[8];      /* Cor em hexadecimal (#RRGGBB) */
} GanttEntry;

/* Gera um arquivo BMP com o gráfico de Gantt.
 *
 * @param filename     Nome do arquivo de saída
 * @param entries      Array de entradas do Gantt
 * @param entry_count  Número de entradas no array
 * @param total_time   Tempo total da simulação (eixo X)
 * @param task_count   Número total de tarefas (eixo Y)
 */
void create_gantt_bmp(const char* filename, GanttEntry* entries, int entry_count,
                      int total_time, int task_count);

#endif /* GANTT_BMP_H */