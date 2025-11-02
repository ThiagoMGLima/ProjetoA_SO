#ifndef GANTT_ASCII_H
#define GANTT_ASCII_H

#include "gantt_bmp.h"

// Imprimir Gantt colorido no terminal (com cores ANSI)
void print_gantt_ascii(GanttEntry* entries, int entry_count,
                       int total_time, int task_count);

// Imprimir Gantt simples (sem cores, compatível com todos os terminais)
void print_gantt_simple(GanttEntry* entries, int entry_count,
                       int total_time, int task_count);

// Salvar Gantt em arquivo texto
void save_gantt_text(const char* filename, GanttEntry* entries, int entry_count,
                    int total_time, int task_count);

#endif // GANTT_ASCII_H