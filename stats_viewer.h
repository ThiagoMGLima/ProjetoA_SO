// stats_viewer.h
#ifndef STATS_VIEWER_H
#define STATS_VIEWER_H

// Estrutura de estatísticas (definida em stats_viewer.c)
// Necessária para que o simulador.c possa criar o array
typedef struct {
    int id;
    int arrival;
    int burst;
    int completion;
    int turnaround;
    int waiting;
    int response;
    int priority;
} TaskStats;

// Funções públicas que serão chamadas pelo simulador.c
void show_statistics(TaskStats* tasks, int count, const char* algorithm);
void export_to_csv(TaskStats* tasks, int count, const char* algorithm);

#endif // STATS_VIEWER_H