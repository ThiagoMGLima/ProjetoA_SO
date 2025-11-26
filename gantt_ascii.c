/*
 * gantt_bmp.c - Gerador de Gráfico de Gantt em formato BMP
 * --------------------------------------------------------
 * Este módulo gera uma imagem BMP do diagrama de Gantt mostrando
 * a execução das tarefas ao longo do tempo. Características:
 *  - Linhas horizontais separando cada tarefa
 *  - Linhas verticais indicando unidades de tempo
 *  - Rótulos das tarefas (T0, T1, ...) na lateral esquerda
 *  - Rótulos de tempo na parte superior
 *  - Barras coloridas indicando execução das tarefas
 *  - Fundo alternado para melhor legibilidade
 *  - Legenda na parte inferior
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "gantt_bmp.h"

// ============================================================================
// ESTRUTURAS DO FORMATO BMP
// ============================================================================

#pragma pack(push, 1)  // Garante alinhamento correto de bytes para o formato BMP

// Cabeçalho do arquivo BMP (14 bytes)
typedef struct {
    uint16_t type;              // Assinatura "BM" = 0x4D42
    uint32_t size;              // Tamanho total do arquivo em bytes
    uint16_t reserved1;         // Reservado (0)
    uint16_t reserved2;         // Reservado (0)
    uint32_t offset;            // Offset para início dos dados da imagem (54)
} BMPFileHeader;

// Cabeçalho de informações da imagem BMP (40 bytes)
typedef struct {
    uint32_t header_size;       // Tamanho deste cabeçalho (40)
    int32_t  width;             // Largura da imagem em pixels
    int32_t  height;            // Altura da imagem (negativo = top-left origin)
    uint16_t planes;            // Número de planos de cor (1)
    uint16_t bits_per_pixel;    // Bits por pixel (24 = RGB)
    uint32_t compression;       // Tipo de compressão (0 = sem compressão)
    uint32_t image_size;        // Tamanho dos dados da imagem
    int32_t  x_pixels_per_meter; // Resolução horizontal (2835 ≈ 72 DPI)
    int32_t  y_pixels_per_meter; // Resolução vertical (2835 ≈ 72 DPI)
    uint32_t colors_used;       // Cores na paleta (0 = todas)
    uint32_t important_colors;  // Cores importantes (0 = todas)
} BMPInfoHeader;

#pragma pack(pop)

// ============================================================================
// ESTRUTURA PARA MANIPULAÇÃO DE CORES RGB
// ============================================================================

typedef struct {
    uint8_t r, g, b;
} Color;

// ============================================================================
// CONSTANTES DE CONFIGURAÇÃO DO LAYOUT
// ============================================================================

#define MARGIN_LEFT     80      // Margem esquerda (para rótulos das tarefas)
#define MARGIN_RIGHT    20      // Margem direita
#define MARGIN_TOP      50      // Margem superior (para escala de tempo)
#define MARGIN_BOTTOM   60      // Margem inferior (para legenda)
#define ROW_HEIGHT      40      // Altura de cada linha de tarefa
#define ROW_SPACING     5       // Espaço entre linhas
#define BAR_HEIGHT      30      // Altura das barras de execução
#define MIN_TICK_WIDTH  15      // Largura mínima por unidade de tempo

// Cores predefinidas para elementos do gráfico
#define COLOR_BACKGROUND_R  255
#define COLOR_BACKGROUND_G  255
#define COLOR_BACKGROUND_B  255

#define COLOR_GRID_R        220
#define COLOR_GRID_G        220
#define COLOR_GRID_B        220

#define COLOR_GRID_MAJOR_R  180
#define COLOR_GRID_MAJOR_G  180
#define COLOR_GRID_MAJOR_B  180

#define COLOR_BORDER_R      60
#define COLOR_BORDER_G      60
#define COLOR_BORDER_B      60

#define COLOR_TEXT_R        0
#define COLOR_TEXT_G        0
#define COLOR_TEXT_B        0

#define COLOR_ALT_ROW_R     245
#define COLOR_ALT_ROW_G     248
#define COLOR_ALT_ROW_B     255

// ============================================================================
// FUNÇÕES AUXILIARES
// ============================================================================

/**
 * Converte uma string hexadecimal de cor para estrutura RGB
 * Aceita formatos: "#RRGGBB" ou "RRGGBB"
 */
Color hex_to_rgb(const char* hex) {
    Color c = {0, 0, 0};
    if (hex[0] == '#') hex++;  // Pular o '#' se presente
    
    unsigned int value;
    if (sscanf(hex, "%06x", &value) == 1) {
        c.r = (value >> 16) & 0xFF;
        c.g = (value >> 8) & 0xFF;
        c.b = value & 0xFF;
    }
    return c;
}

/**
 * Escurece uma cor para criar efeito de borda
 * factor: 0.0 = preto, 1.0 = cor original
 */
Color darken_color(Color c, float factor) {
    Color dark;
    dark.r = (uint8_t)(c.r * factor);
    dark.g = (uint8_t)(c.g * factor);
    dark.b = (uint8_t)(c.b * factor);
    return dark;
}

// ============================================================================
// FONTE BITMAP SIMPLES (3x5 pixels por caractere)
// ============================================================================

// Cada dígito é representado por 5 linhas de 3 bits cada
static const uint8_t digit_font[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
    {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
    {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
    {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
    {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
    {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
    {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
    {0b111, 0b001, 0b010, 0b100, 0b100}, // 7
    {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
    {0b111, 0b101, 0b111, 0b001, 0b111}  // 9
};

// Letra 'T' para rótulos de tarefas (3x5)
static const uint8_t letter_T[5] = {
    0b111, 0b010, 0b010, 0b010, 0b010
};

// ============================================================================
// FUNÇÕES DE DESENHO
// ============================================================================

/**
 * Desenha um único pixel na imagem
 * Coordenadas são verificadas para evitar escrita fora dos limites
 */
static void draw_pixel(uint8_t *image, int row_size, int width, int height,
                       int px, int py, uint8_t r, uint8_t g, uint8_t b) {
    if (px < 0 || px >= width || py < 0 || py >= height) return;
    
    int idx = py * row_size + px * 3;
    image[idx] = b;      // BMP armazena em ordem BGR
    image[idx + 1] = g;
    image[idx + 2] = r;
}

/**
 * Desenha uma linha horizontal
 */
static void draw_horizontal_line(uint8_t *image, int row_size, int width, int height,
                                  int x1, int x2, int y, Color col) {
    for (int x = x1; x <= x2 && x < width; x++) {
        draw_pixel(image, row_size, width, height, x, y, col.r, col.g, col.b);
    }
}

/**
 * Desenha uma linha vertical
 */
static void draw_vertical_line(uint8_t *image, int row_size, int width, int height,
                                int x, int y1, int y2, Color col) {
    for (int y = y1; y <= y2 && y < height; y++) {
        draw_pixel(image, row_size, width, height, x, y, col.r, col.g, col.b);
    }
}

/**
 * Desenha um retângulo preenchido
 */
static void draw_filled_rect(uint8_t *image, int row_size, int width, int height,
                             int x1, int y1, int x2, int y2, Color col) {
    for (int y = y1; y <= y2 && y < height; y++) {
        for (int x = x1; x <= x2 && x < width; x++) {
            draw_pixel(image, row_size, width, height, x, y, col.r, col.g, col.b);
        }
    }
}

/**
 * Desenha um retângulo com borda (para as barras de execução)
 */
static void draw_rect_with_border(uint8_t *image, int row_size, int width, int height,
                                  int x1, int y1, int x2, int y2, 
                                  Color fill, Color border, int border_width) {
    // Desenhar preenchimento
    draw_filled_rect(image, row_size, width, height, x1, y1, x2, y2, fill);
    
    // Desenhar bordas
    for (int b = 0; b < border_width; b++) {
        // Borda superior
        draw_horizontal_line(image, row_size, width, height, x1, x2, y1 + b, border);
        // Borda inferior
        draw_horizontal_line(image, row_size, width, height, x1, x2, y2 - b, border);
        // Borda esquerda
        draw_vertical_line(image, row_size, width, height, x1 + b, y1, y2, border);
        // Borda direita
        draw_vertical_line(image, row_size, width, height, x2 - b, y1, y2, border);
    }
}

/**
 * Desenha um dígito individual usando a fonte bitmap
 */
static void draw_digit(uint8_t *image, int row_size, int width, int height,
                       int digit, int x0, int y0, int scale, Color col) {
    if (digit < 0 || digit > 9) return;
    
    for (int ry = 0; ry < 5; ry++) {
        uint8_t row = digit_font[digit][ry];
        for (int rx = 0; rx < 3; rx++) {
            if (row & (1 << (2 - rx))) {
                // Desenhar pixel escalado
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        draw_pixel(image, row_size, width, height,
                                   x0 + rx * scale + sx,
                                   y0 + ry * scale + sy,
                                   col.r, col.g, col.b);
                    }
                }
            }
        }
    }
}

/**
 * Desenha a letra 'T' usando a fonte bitmap
 */
static void draw_letter_T(uint8_t *image, int row_size, int width, int height,
                          int x0, int y0, int scale, Color col) {
    for (int ry = 0; ry < 5; ry++) {
        uint8_t row = letter_T[ry];
        for (int rx = 0; rx < 3; rx++) {
            if (row & (1 << (2 - rx))) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        draw_pixel(image, row_size, width, height,
                                   x0 + rx * scale + sx,
                                   y0 + ry * scale + sy,
                                   col.r, col.g, col.b);
                    }
                }
            }
        }
    }
}

/**
 * Desenha um número inteiro (pode ter múltiplos dígitos)
 * Retorna a largura ocupada pelo texto
 */
static int draw_number(uint8_t *image, int row_size, int width, int height,
                       int number, int x0, int y0, int scale, Color col) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", number);
    
    int char_w = 3 * scale;
    int gap = scale;
    int x = x0;
    
    for (int i = 0; buf[i] != '\0'; i++) {
        if (buf[i] >= '0' && buf[i] <= '9') {
            draw_digit(image, row_size, width, height, buf[i] - '0', x, y0, scale, col);
        }
        x += char_w + gap;
    }
    
    return x - x0;
}

/**
 * Desenha o rótulo de uma tarefa (ex: "T0", "T1", ...)
 */
static void draw_task_label(uint8_t *image, int row_size, int width, int height,
                            int task_id, int x0, int y0, int scale, Color col) {
    int char_w = 3 * scale;
    int gap = scale;
    
    // Desenhar 'T'
    draw_letter_T(image, row_size, width, height, x0, y0, scale, col);
    
    // Desenhar número do ID
    draw_number(image, row_size, width, height, task_id, 
                x0 + char_w + gap, y0, scale, col);
}

// ============================================================================
// FUNÇÃO PRINCIPAL DE GERAÇÃO DO GRÁFICO DE GANTT
// ============================================================================

/**
 * Cria o arquivo BMP do gráfico de Gantt
 * 
 * @param filename     Nome do arquivo de saída
 * @param entries      Array de entradas do Gantt (execuções)
 * @param entry_count  Número de entradas
 * @param total_time   Tempo total da simulação
 * @param task_count   Número de tarefas
 */
void create_gantt_bmp(const char* filename, GanttEntry* entries, int entry_count,
                      int total_time, int task_count) {
    
    // ========================================================================
    // CÁLCULO DAS DIMENSÕES DA IMAGEM
    // ========================================================================
    
    // Calcular largura necessária para a área do gráfico
    int time_scale = MIN_TICK_WIDTH;
    int chart_width = total_time * time_scale;
    
    // Largura total incluindo margens
    int width = MARGIN_LEFT + chart_width + MARGIN_RIGHT;
    if (width < 400) width = 400;  // Largura mínima
    
    // Altura total
    int chart_height = task_count * (ROW_HEIGHT + ROW_SPACING);
    int height = MARGIN_TOP + chart_height + MARGIN_BOTTOM;
    if (height < 200) height = 200;  // Altura mínima
    
    // Recalcular time_scale baseado na largura disponível
    time_scale = (width - MARGIN_LEFT - MARGIN_RIGHT) / (total_time > 0 ? total_time : 1);
    if (time_scale < MIN_TICK_WIDTH) time_scale = MIN_TICK_WIDTH;
    
    // ========================================================================
    // ALOCAÇÃO E INICIALIZAÇÃO DO BUFFER DA IMAGEM
    // ========================================================================
    
    // Calcular padding (linhas BMP devem ser múltiplas de 4 bytes)
    int padding = (4 - (width * 3) % 4) % 4;
    int row_size = width * 3 + padding;
    
    // Alocar buffer para a imagem
    uint8_t* image = calloc(row_size * height, 1);
    if (!image) {
        printf("Erro: Não foi possível alocar memória para a imagem.\n");
        return;
    }
    
    // ========================================================================
    // DESENHAR FUNDO
    // ========================================================================
    
    // Preencher com branco
    Color bg = {COLOR_BACKGROUND_R, COLOR_BACKGROUND_G, COLOR_BACKGROUND_B};
    draw_filled_rect(image, row_size, width, height, 0, 0, width - 1, height - 1, bg);
    
    // Desenhar fundo alternado para as linhas de tarefas (zebra stripes)
    Color alt_row = {COLOR_ALT_ROW_R, COLOR_ALT_ROW_G, COLOR_ALT_ROW_B};
    for (int i = 0; i < task_count; i++) {
        if (i % 2 == 1) {  // Linhas ímpares têm fundo diferente
            int y_start = MARGIN_TOP + i * (ROW_HEIGHT + ROW_SPACING);
            int y_end = y_start + ROW_HEIGHT;
            draw_filled_rect(image, row_size, width, height, 
                            MARGIN_LEFT, y_start, 
                            width - MARGIN_RIGHT, y_end, alt_row);
        }
    }
    
    // ========================================================================
    // DESENHAR GRADE VERTICAL (LINHAS DE TEMPO)
    // ========================================================================
    
    Color grid = {COLOR_GRID_R, COLOR_GRID_G, COLOR_GRID_B};
    Color grid_major = {COLOR_GRID_MAJOR_R, COLOR_GRID_MAJOR_G, COLOR_GRID_MAJOR_B};
    Color text_color = {COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B};
    
    for (int t = 0; t <= total_time; t++) {
        int x = MARGIN_LEFT + t * time_scale;
        
        // Linha vertical da grade
        Color line_color = (t % 5 == 0) ? grid_major : grid;  // A cada 5 ticks, linha mais forte
        draw_vertical_line(image, row_size, width, height, 
                          x, MARGIN_TOP, 
                          MARGIN_TOP + chart_height, line_color);
        
        // Rótulo de tempo na parte superior (a cada N ticks dependendo da escala)
        int label_interval = 1;
        if (total_time > 50) label_interval = 10;
        else if (total_time > 20) label_interval = 5;
        else if (total_time > 10) label_interval = 2;
        
        if (t % label_interval == 0) {
            // Centralizar o número sobre a linha
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", t);
            int text_width = strlen(buf) * 4 * 2;  // Estimativa da largura do texto
            draw_number(image, row_size, width, height, t, 
                       x - text_width / 4, MARGIN_TOP - 20, 2, text_color);
        }
    }
    
    // ========================================================================
    // DESENHAR LINHAS HORIZONTAIS (SEPARADORES DE TAREFAS)
    // ========================================================================
    
    // Linha no topo da área do gráfico
    draw_horizontal_line(image, row_size, width, height,
                        MARGIN_LEFT, width - MARGIN_RIGHT, MARGIN_TOP, grid_major);
    
    // Linhas separando cada tarefa
    for (int i = 0; i <= task_count; i++) {
        int y = MARGIN_TOP + i * (ROW_HEIGHT + ROW_SPACING);
        draw_horizontal_line(image, row_size, width, height,
                            MARGIN_LEFT, width - MARGIN_RIGHT, y, grid_major);
    }
    
    // ========================================================================
    // DESENHAR RÓTULOS DAS TAREFAS (T0, T1, ...)
    // ========================================================================
    
    for (int i = 0; i < task_count; i++) {
        int y_center = MARGIN_TOP + i * (ROW_HEIGHT + ROW_SPACING) + ROW_HEIGHT / 2;
        // Posicionar o rótulo centralizado verticalmente na linha
        draw_task_label(image, row_size, width, height, i,
                       15, y_center - 5, 2, text_color);
    }
    
    // ========================================================================
    // DESENHAR BARRAS DE EXECUÇÃO DAS TAREFAS
    // ========================================================================
    
    for (int i = 0; i < entry_count; i++) {
        GanttEntry* entry = &entries[i];
        Color task_color = hex_to_rgb(entry->color);
        Color border_color = darken_color(task_color, 0.6f);
        
        // Calcular posição da barra
        int y_start = MARGIN_TOP + entry->task_id * (ROW_HEIGHT + ROW_SPACING);
        int y_bar_start = y_start + (ROW_HEIGHT - BAR_HEIGHT) / 2;
        int y_bar_end = y_bar_start + BAR_HEIGHT;
        
        int x_start = MARGIN_LEFT + entry->start_time * time_scale;
        int x_end = MARGIN_LEFT + entry->end_time * time_scale - 1;
        
        // Garantir largura mínima da barra
        if (x_end <= x_start) x_end = x_start + 1;
        
        // Desenhar barra com borda
        draw_rect_with_border(image, row_size, width, height,
                             x_start, y_bar_start, x_end, y_bar_end,
                             task_color, border_color, 2);
    }
    
    // ========================================================================
    // DESENHAR LEGENDA
    // ========================================================================
    
    int legend_y = height - MARGIN_BOTTOM + 15;
    int legend_x = MARGIN_LEFT;
    
    // Título da legenda
    // (Aqui seria ideal ter mais letras na fonte, mas vamos simplificar)
    
    // Mostrar exemplo de cada tarefa com sua cor
    for (int i = 0; i < task_count && i < 8; i++) {  // Limitar a 8 para caber
        // Encontrar a cor desta tarefa
        Color task_color = {128, 128, 128};  // Cor padrão
        for (int j = 0; j < entry_count; j++) {
            if (entries[j].task_id == i) {
                task_color = hex_to_rgb(entries[j].color);
                break;
            }
        }
        
        // Desenhar quadrado de cor
        int sq_size = 12;
        Color border = darken_color(task_color, 0.6f);
        draw_rect_with_border(image, row_size, width, height,
                             legend_x, legend_y, 
                             legend_x + sq_size, legend_y + sq_size,
                             task_color, border, 1);
        
        // Desenhar rótulo
        draw_task_label(image, row_size, width, height, i,
                       legend_x + sq_size + 5, legend_y + 2, 1, text_color);
        
        legend_x += 50;  // Espaçamento entre itens da legenda
    }
    
    // ========================================================================
    // ESCREVER ARQUIVO BMP
    // ========================================================================
    
    // Preparar cabeçalhos
    BMPFileHeader file_header = {
        .type = 0x4D42,  // "BM"
        .size = 54 + row_size * height,
        .reserved1 = 0,
        .reserved2 = 0,
        .offset = 54
    };
    
    BMPInfoHeader info_header = {
        .header_size = 40,
        .width = width,
        .height = -height,  // Negativo para origem no canto superior esquerdo
        .planes = 1,
        .bits_per_pixel = 24,
        .compression = 0,
        .image_size = row_size * height,
        .x_pixels_per_meter = 2835,
        .y_pixels_per_meter = 2835,
        .colors_used = 0,
        .important_colors = 0
    };
    
    // Abrir arquivo para escrita
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("Erro: Não foi possível criar o arquivo %s\n", filename);
        free(image);
        return;
    }
    
    // Escrever cabeçalhos e dados
    fwrite(&file_header, sizeof(BMPFileHeader), 1, f);
    fwrite(&info_header, sizeof(BMPInfoHeader), 1, f);
    fwrite(image, 1, row_size * height, f);
    
    fclose(f);
    free(image);
    
    printf("Gráfico de Gantt salvo em: %s (%dx%d pixels)\n", filename, width, height);
}