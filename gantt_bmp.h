#ifndef GANTT_BMP_H
#define GANTT_BMP_H

#include <stdint.h>
#include <limits.h>

// Estrutura pública para entradas do Gantt
typedef struct {
    int task_id;
    int start_time;
    int end_time;
    char color[8];
} GanttEntry;

// Função pública para gerar o BMP do Gantt
void create_gantt_bmp(const char* filename, GanttEntry* entries, int entry_count,
                      int total_time, int task_count);

#endif // GANTT_BMP_H