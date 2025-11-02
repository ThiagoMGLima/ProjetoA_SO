// interface.c - Interface interativa para o simulador
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "gantt_bmp.h"
#include "gantt_ascii.h"

// Cores ANSI para terminal
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"
#define CLEAR   "\033[2J\033[H"

// Estrutura para tarefas configuráveis
typedef struct {
    int id;
    char color[8];
    int arrival;
    int burst;
    int priority;
} TaskConfig;

// Limpar tela (multiplataforma)
void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        printf(CLEAR);
    #endif
}

// Pausar execução
void press_enter() {
    printf("\n" CYAN "Pressione ENTER para continuar..." RESET);
    getchar();
    while(getchar() != '\n');
}

// Cabeçalho decorado
void print_header() {
    clear_screen();
    printf(BOLD BLUE);
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║         SIMULADOR DE SISTEMA OPERACIONAL v1.0              ║\n");
    printf("║              Escalonamento de Processos                    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf(RESET "\n");
}

// Menu principal
int main_menu() {
    print_header();

    printf(BOLD "MENU PRINCIPAL\n" RESET);
    printf("══════════════\n\n");

    printf(GREEN "1" RESET " → Simulação Rápida (arquivo existente)\n");
    printf(GREEN "2" RESET " → Criar Nova Configuração\n");
    printf(GREEN "3" RESET " → Simulação com Exemplos Prontos\n");
    printf(GREEN "4" RESET " → Visualizar Última Simulação\n");
    printf(GREEN "5" RESET " → Ajuda e Tutorial\n");
    printf(RED "0" RESET " → Sair\n");

    printf("\n" CYAN "Escolha uma opção: " RESET);

    int choice;
    scanf("%d", &choice);
    getchar(); // limpar buffer

    return choice;
}

// Criar configuração interativamente
void create_configuration() {
    print_header();
    printf(BOLD "CRIAR NOVA CONFIGURAÇÃO\n" RESET);
    printf("═══════════════════════\n\n");

    char filename[100];
    char algorithm[20];
    int quantum, num_tasks;

    // Escolher algoritmo
    printf(YELLOW "Escolha o algoritmo de escalonamento:\n" RESET);
    printf("  1. FIFO (First In First Out)\n");
    printf("  2. SRTF (Shortest Remaining Time First)\n");
    printf("  3. PRIORITY (Por Prioridade)\n");
    printf("\nOpção: ");

    int alg_choice;
    scanf("%d", &alg_choice);

    switch(alg_choice) {
        case 1: strcpy(algorithm, "FIFO"); break;
        case 2: strcpy(algorithm, "SRTF"); break;
        case 3: strcpy(algorithm, "PRIORITY"); break;
        default: strcpy(algorithm, "FIFO");
    }

    printf("\n" YELLOW "Quantum (tempo máximo por execução): " RESET);
    scanf("%d", &quantum);

    printf("\n" YELLOW "Número de tarefas: " RESET);
    scanf("%d", &num_tasks);

    TaskConfig* tasks = malloc(num_tasks * sizeof(TaskConfig));

    // Cores predefinidas
    const char* colors[] = {
        "#FF0000", "#00FF00", "#0000FF", "#FFFF00",
        "#FF00FF", "#00FFFF", "#FFA500", "#800080"
    };

    printf("\n" BOLD "CONFIGURAR TAREFAS\n" RESET);
    printf("─────────────────\n");

    for (int i = 0; i < num_tasks; i++) {
        printf("\n" CYAN "Tarefa %d:\n" RESET, i);

        tasks[i].id = i;
        strcpy(tasks[i].color, colors[i % 8]);

        printf("  Tempo de chegada (arrival): ");
        scanf("%d", &tasks[i].arrival);

        printf("  Tempo de execução (burst): ");
        scanf("%d", &tasks[i].burst);

        printf("  Prioridade (1=alta, 9=baixa): ");
        scanf("%d", &tasks[i].priority);
    }

    // Salvar arquivo
    printf("\n" YELLOW "Nome do arquivo para salvar (ex: config.txt): " RESET);
    scanf("%s", filename);

    FILE* f = fopen(filename, "w");
    if (f) {
        fprintf(f, "%s;%d\n", algorithm, quantum);
        for (int i = 0; i < num_tasks; i++) {
            fprintf(f, "%d;%s;%d;%d;%d;\n",
                    tasks[i].id, tasks[i].color,
                    tasks[i].arrival, tasks[i].burst,
                    tasks[i].priority);
        }
        fclose(f);

        printf(GREEN "\n✓ Configuração salva em: %s\n" RESET, filename);

        printf("\n" YELLOW "Deseja executar a simulação agora? (s/n): " RESET);
        char resp;
        scanf(" %c", &resp);

        if (resp == 's' || resp == 'S') {
            char command[200];
            sprintf(command, "./simulador %s", filename);
            system(command);
        }
    } else {
        printf(RED "\n✗ Erro ao salvar arquivo!\n" RESET);
    }

    free(tasks);
    press_enter();
}

// Exemplos prontos
void run_examples() {
    print_header();
    printf(BOLD "EXEMPLOS PRONTOS\n" RESET);
    printf("════════════════\n\n");

    printf("Escolha um exemplo:\n\n");
    printf(GREEN "1" RESET " → Caso Simples (3 tarefas)\n");
    printf(GREEN "2" RESET " → Caso Médio (5 tarefas)\n");
    printf(GREEN "3" RESET " → Caso Complexo (8 tarefas)\n");


    printf("\n" CYAN "Opção: " RESET);
    int choice;
    scanf("%d", &choice);

    const char* filename = "example.txt";
    FILE* f = fopen(filename, "w");

    if (!f) {
        printf(RED "Erro ao criar arquivo!\n" RESET);
        return;
    }

    switch(choice) {
        case 1: // Caso simples
            fprintf(f, "FIFO;10\n");
            fprintf(f, "0;#FF0000;0;10;1;\n");
            fprintf(f, "1;#00FF00;2;8;2;\n");
            fprintf(f, "2;#0000FF;4;6;3;\n");
            break;

        case 2: // Caso médio
            fprintf(f, "SRTF;10\n");
            fprintf(f, "0;#FF0000;0;20;1;\n");
            fprintf(f, "1;#00FF00;5;10;2;\n");
            fprintf(f, "2;#0000FF;10;15;1;\n");
            fprintf(f, "3;#FFFF00;15;5;3;\n");
            fprintf(f, "4;#FF00FF;20;8;2;\n");
            break;

        case 3: // Caso complexo
            fprintf(f, "PRIORITY;5\n");
            for (int i = 0; i < 8; i++) {
                fprintf(f, "%d;#%02X%02X%02X;%d;%d;%d;\n",
                        i, (i*31)%256, (i*47)%256, (i*67)%256,
                        i*3, 5 + (i*7)%15, (i%3)+1);
            }
            break;
          }
    fclose(f);

    printf("\n" GREEN "Executando exemplo...\n" RESET);
    system("./simulador example.txt");

    press_enter();
}

// Visualizar última simulação
void view_last_simulation() {
    print_header();
    printf(BOLD "ÚLTIMA SIMULAÇÃO\n" RESET);
    printf("════════════════\n\n");

    // Verificar se existe gantt_output.bmp
    if (access("gantt_output.bmp", F_OK) == 0) {
        printf(GREEN "✓ Arquivo gantt_output.bmp encontrado\n" RESET);

        // Tentar abrir com visualizador padrão
        #ifdef __linux__
            system("xdg-open gantt_output.bmp 2>/dev/null &");
            printf("Tentando abrir com visualizador padrão...\n");
        #elif __APPLE__
            system("open gantt_output.bmp 2>/dev/null &");
            printf("Tentando abrir com Preview...\n");
        #elif _WIN32
            system("start gantt_output.bmp 2>NUL");
            printf("Tentando abrir com visualizador padrão...\n");
        #else
            printf(YELLOW "Não foi possível abrir automaticamente.\n" RESET);
            printf("Por favor, abra o arquivo gantt_output.bmp manualmente.\n");
        #endif

        // Mostrar informações do arquivo
        system("ls -lh gantt_output.bmp 2>/dev/null");
    } else {
        printf(RED "✗ Nenhuma simulação encontrada!\n" RESET);
        printf("Execute uma simulação primeiro.\n");
    }

    press_enter();
}

// Menu de ajuda
void show_help() {
    print_header();
    printf(BOLD "AJUDA E TUTORIAL\n" RESET);
    printf("════════════════\n\n");

    printf(YELLOW "CONCEITOS BÁSICOS:\n" RESET);
    printf("──────────────────\n");
    printf("• " BOLD "FIFO:" RESET " First In First Out - Ordem de chegada\n");
    printf("• " BOLD "SRTF:" RESET " Shortest Remaining Time First - Menor tempo restante\n");
    printf("• " BOLD "Priority:" RESET " Escalonamento por prioridade (menor valor = maior prioridade)\n");

    printf("\n" YELLOW "FORMATO DO ARQUIVO:\n" RESET);
    printf("───────────────────\n");
    printf("Linha 1: algoritmo;quantum\n");
    printf("Demais:  id;cor;chegada;burst;prioridade;\n");

    printf("\n" YELLOW "EXEMPLO:\n" RESET);
    printf("────────\n");
    printf("FIFO;10\n");
    printf("0;#FF0000;0;20;1;\n");
    printf("1;#00FF00;5;15;2;\n");

    printf("\n" YELLOW "ESTATÍSTICAS:\n" RESET);
    printf("─────────────\n");
    printf("• " BOLD "Turnaround:" RESET " Tempo total no sistema (fim - chegada)\n");
    printf("• " BOLD "Waiting:" RESET " Tempo esperando na fila\n");
    printf("• " BOLD "Response:" RESET " Tempo até primeira execução\n");

    printf("\n" YELLOW "DICAS:\n" RESET);
    printf("──────\n");
    printf("• Use modo debug (--step) para entender o escalonamento\n");
    printf("• Observe o Gantt Chart para visualizar a execução\n");

    press_enter();
}

// Executar simulação com arquivo
void run_simulation() {
    print_header();
    printf(BOLD "EXECUTAR SIMULAÇÃO\n" RESET);
    printf("══════════════════\n\n");

    char filename[100];
    printf(YELLOW "Nome do arquivo de configuração: " RESET);
    scanf("%s", filename);

    // Verificar se arquivo existe
    if (access(filename, F_OK) != 0) {
        printf(RED "\n✗ Arquivo não encontrado!\n" RESET);
        press_enter();
        return;
    }

    printf("\n" YELLOW "Modo de execução:\n" RESET);
    printf("  1. Normal (execução completa)\n");
    printf("  2. Debug (passo-a-passo)\n");
    printf("\nOpção: ");

    int mode;
    scanf("%d", &mode);

    char command[200];
    if (mode == 2) {
        sprintf(command, "./simulador %s --step", filename);
    } else {
        sprintf(command, "./simulador %s", filename);
    }

    printf("\n" GREEN "Executando simulação...\n" RESET);
    printf("─────────────────────────\n\n");

    system(command);

    press_enter();
}

// Função principal da interface
int main() {
    // Verificar se simulador existe
    if (access("simulador", F_OK) != 0) {
        printf(RED "Erro: Simulador não encontrado!\n" RESET);
        printf("Compile primeiro com: make\n");
        return 1;
    }

    int choice;

    do {
        choice = main_menu();

        switch(choice) {
            case 1:
                run_simulation();
                break;
            case 2:
                create_configuration();
                break;
            case 3:
                run_examples();
                break;
            case 4:
                view_last_simulation();
                break;
            case 5:
                show_help();
                break;
            case 0:
                clear_screen();
                printf(GREEN "Obrigado por usar o Simulador!\n" RESET);
                printf("Desenvolvido para Sistemas Operacionais\n\n");
                break;
            default:
                printf(RED "Opção inválida!\n" RESET);
                press_enter();
        }
    } while (choice != 0);

    return 0;
}