/*
 * gantt_ascii.h - Header para visualização ASCII do Gantt Chart
 */

#ifndef GANTT_ASCII_H
#define GANTT_ASCII_H

#include "gantt_bmp.h"  /* Para usar GanttEntry */

/* Imprime o Gantt Chart colorido no terminal (com cores ANSI) */
void print_gantt_ascii(GanttEntry* entries, int entry_count,
                       int total_time, int task_count);

/* Imprime o Gantt Chart simples (sem cores, compatível com todos os terminais) */
void print_gantt_simple(GanttEntry* entries, int entry_count,
                       int total_time, int task_count);

/* Salva o Gantt Chart em arquivo texto */
void save_gantt_text(const char* filename, GanttEntry* entries, int entry_count,
                    int total_time, int task_count);

#endif /* GANTT_ASCII_H */