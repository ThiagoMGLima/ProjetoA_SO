// gantt_bmp.c - ImplementaÃ§Ã£o completa de geraÃ§Ã£o BMP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "gantt_bmp.h"



// Estrutura para o header BMP
#pragma pack(push, 1)  // Garante alinhamento de bytes
typedef struct {
    uint16_t type;              // "BM" = 0x4D42
    uint32_t size;              // Tamanho do arquivo
    uint16_t reserved1;         // 0
    uint16_t reserved2;         // 0
    uint32_t offset;            // Offset para dados = 54
} BMPFileHeader;

typedef struct {
    uint32_t header_size;       // 40
    int32_t  width;            
    int32_t  height;           
    uint16_t planes;            // 1
    uint16_t bits_per_pixel;   // 24
    uint32_t compression;       // 0 (sem compressÃ£o)
    uint32_t image_size;        
    int32_t  x_pixels_per_meter; // 2835
    int32_t  y_pixels_per_meter; // 2835
    uint32_t colors_used;       // 0
    uint32_t important_colors;  // 0
} BMPInfoHeader;
#pragma pack(pop)

// Estrutura para cores
typedef struct {
    uint8_t r, g, b;
} Color;

// Converter hex string para RGB
Color hex_to_rgb(const char* hex) {
    Color c = {0, 0, 0};
    if (hex[0] == '#') hex++;
    
    unsigned int value;
    sscanf(hex, "%06x", &value);
    
    c.r = (value >> 16) & 0xFF;
    c.g = (value >> 8) & 0xFF;
    c.b = value & 0xFF;
    
    return c;
}

// --- Simple bitmap font (3x5) for time labels ---
// Each digit is 3 columns x 5 rows, bits left-to-right
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

static void draw_pixel_buf(uint8_t *image, int row_size, int width, int height,
                          int px, int py, uint8_t r, uint8_t g, uint8_t b) {
    if (px < 0 || px >= width || py < 0 || py >= height) return;
    int pidx = py * row_size + px * 3;
    image[pidx] = b;
    image[pidx + 1] = g;
    image[pidx + 2] = r;
}

static void draw_digit_buf(uint8_t *image, int row_size, int width, int height,
                           int digit, int x0, int y0, int scale, Color col) {
    if (digit < 0 || digit > 9) return;
    for (int ry = 0; ry < 5; ry++) {
        uint8_t row = digit_font[digit][ry];
        for (int rx = 0; rx < 3; rx++) {
            if (row & (1 << (2 - rx))) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        draw_pixel_buf(image, row_size, width, height,
                                       x0 + rx * scale + sx,
                                       y0 + ry * scale + sy,
                                       col.r, col.g, col.b);
                    }
                }
            }
        }
    }
}

static void draw_text_buf(uint8_t *image, int row_size, int width, int height,
                          const char* s, int x_center, int y0, int scale, Color col) {
    int len = (int)strlen(s);
    int char_w = 3 * scale;
    int gap = scale;
    int text_w = len * char_w + (len - 1) * gap;
    int x = x_center - text_w / 2;
    for (int i = 0; i < len; i++) {
        char ch = s[i];
        if (ch >= '0' && ch <= '9') {
            draw_digit_buf(image, row_size, width, height, ch - '0', x, y0, scale, col);
        } else if (ch == '-') {
            int dash_y = y0 + 2 * scale;
            for (int dx = 0; dx < 3 * scale; dx++) draw_pixel_buf(image, row_size, width, height, x + dx, dash_y, col.r, col.g, col.b);
        }
        x += char_w + gap;
    }
}

void create_gantt_bmp(const char* filename, GanttEntry* entries, int entry_count, 
                      int total_time, int task_count) {
    
    // DimensÃµes da imagem
    int width = 800;
    int height = 50 * task_count + 100;  // 50 pixels por tarefa + margens
    int row_height = 40;
    int time_scale = width / (total_time + 1);  // pixels por unidade de tempo
    
    // Calcular padding (BMP precisa de linhas mÃºltiplas de 4 bytes)
    int padding = (4 - (width * 3) % 4) % 4;
    int row_size = width * 3 + padding;
    
    // Alocar buffer para a imagem
    uint8_t* image = calloc(row_size * height, 1);
    
    // Preencher com branco
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * row_size + x * 3;
            image[idx] = 255;     // B
            image[idx + 1] = 255; // G
            image[idx + 2] = 255; // R
        }
    }
    
    // Desenhar grade de tempo (linhas verticais)
    for (int t = 0; t <= total_time; t++) {
        int x = t * time_scale;
        for (int y = 0; y < height; y++) {
            int idx = y * row_size + x * 3;
            // Linha cinza clara
            image[idx] = 200;
            image[idx + 1] = 200;
            image[idx + 2] = 200;
        }
    }
    
    // Draw numeric labels for each time tick near the top (small, black)
    Color label_col = {0, 0, 0};
    for (int t = 0; t <= total_time; t++) {
        int x = t * time_scale;
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", t);
        int y_label = 6; // pixels from top
        int scale = 1;
        draw_text_buf(image, row_size, width, height, buf, x, y_label, scale, label_col);
    }
    
    // Desenhar execuÃ§Ãµes das tarefas
    for (int i = 0; i < entry_count; i++) {
        Color task_color = hex_to_rgb(entries[i].color);
        
        int y_start = 50 + entries[i].task_id * row_height;
        int y_end = y_start + 30;  // Altura da barra
        int x_start = entries[i].start_time * time_scale;
        int x_end = entries[i].end_time * time_scale;
        
        // Preencher retÃ¢ngulo
        for (int y = y_start; y < y_end && y < height; y++) {
            for (int x = x_start; x < x_end && x < width; x++) {
                int idx = y * row_size + x * 3;
                image[idx] = task_color.b;
                image[idx + 1] = task_color.g;
                image[idx + 2] = task_color.r;
            }
        }
    }
    

    // Preparar headers
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
        .height = -height,  // Negativo para origem top-left
        .planes = 1,
        .bits_per_pixel = 24,
        .compression = 0,
        .image_size = row_size * height,
        .x_pixels_per_meter = 2835,
        .y_pixels_per_meter = 2835,
        .colors_used = 0,
        .important_colors = 0
    };
    
    // Escrever arquivo
    FILE* f = fopen(filename, "wb");
    if (!f) {
        free(image);
        return;
    }
    
    fwrite(&file_header, sizeof(BMPFileHeader), 1, f);
    fwrite(&info_header, sizeof(BMPInfoHeader), 1, f);
    fwrite(image, 1, row_size * height, f);
    
    fclose(f);
    free(image);
    
    printf("Gantt chart salvo em: %s\n", filename);
}