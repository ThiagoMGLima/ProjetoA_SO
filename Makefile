# Makefile para Simulador de SO Multitarefa
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -static -static-libgcc

# --- Alvos Principais ---
TARGET_SIM = simulador
TARGET_INT = interface

# Detectar SO
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lm
endif
ifeq ($(UNAME_S),Darwin)
    # MacOS pode ter limitações com -static
    LDFLAGS = -lm
endif

# Windows (MinGW)
ifdef OS
    TARGET_SIM = simulador.exe
    TARGET_INT = interface.exe
    LDFLAGS += -lm
endif

# --- Fontes ---

# Fontes para o Simulador
# (Inclui os novos stats_viewer.c e gantt_ascii.c como bibliotecas)
SRCS_SIM = simulador.c gantt_bmp.c gantt_ascii.c stats_viewer.c
OBJS_SIM = $(SRCS_SIM:.c=.o)

# Fontes para a Interface
SRCS_INT = interface.c
OBJS_INT = $(SRCS_INT:.c=.o)

# --- Regras de Build ---

# Regra principal (Default)
all: $(TARGET_SIM) $(TARGET_INT)
	@echo "====================================="
	@echo "Executáveis gerados:"
	@echo "  $(TARGET_SIM)"
	@echo "  $(TARGET_INT)"
	@echo "====================================="
	@echo ""
	@echo "Para iniciar a interface, execute:"
	@echo "  ./$(TARGET_INT)"
	@echo ""
	@echo "Para usar o simulador diretamente:"
	@echo "  ./$(TARGET_SIM) <arquivo_de_configuração>"

# Regra para o Simulador
$(TARGET_SIM): $(OBJS_SIM)
	$(CC) $(OBJS_SIM) -o $(TARGET_SIM) $(LDFLAGS)

# Regra para a Interface
$(TARGET_INT): $(OBJS_INT)
	$(CC) $(OBJS_INT) -o $(TARGET_INT) $(LDFLAGS)

# Compilar objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Demo simples
demo: demo.o gantt_bmp.o
	$(CC) demo.o gantt_bmp.o -o demo $(LDFLAGS)

# Limpar
clean:
	# Limpa objetos, executáveis, demos e arquivos gerados (bmp, svg, log, csv)
	rm -f $(OBJS_SIM) $(OBJS_INT) $(TARGET_SIM) $(TARGET_INT) demo demo.o *.bmp *.svg *.log *.csv

# --- Testes (usam o $(TARGET_SIM)) ---

# Testar com FIFO
test-fifo: $(TARGET_SIM)
	@echo "FIFO;10" > test_fifo.txt
	@echo "0;#FF0000;0;20;1;" >> test_fifo.txt
	@echo "1;#00FF00;5;15;2;" >> test_fifo.txt
	@echo "2;#0000FF;10;10;3;" >> test_fifo.txt
	./$(TARGET_SIM) test_fifo.txt

# Testar com SRTF
test-srtf: $(TARGET_SIM)
	@echo "SRTF;10" > test_srtf.txt
	@echo "0;#FF0000;0;20;1;" >> test_srtf.txt
	@echo "1;#00FF00;5;15;2;" >> test_srtf.txt
	@echo "2;#0000FF;10;5;3;" >> test_srtf.txt
	./$(TARGET_SIM) test_srtf.txt

# Testar com Priority
test-priority: $(TARGET_SIM)
	@echo "PRIORITY;10" > test_priority.txt
	@echo "0;#FF0000;0;20;3;" >> test_priority.txt
	@echo "1;#00FF00;5;15;1;" >> test_priority.txt
	@echo "2;#0000FF;10;10;2;" >> test_priority.txt
	./$(TARGET_SIM) test_priority.txt

# Testar modo debug
test-debug: $(TARGET_SIM)
	./$(TARGET_SIM) test_config.txt --step

# Testar todos
test-all: test-fifo test-srtf test-priority
	@echo "Todos os testes concluídos!"

# Criar arquivo de teste complexo
test-complex: $(TARGET_SIM)
	@echo "SRTF;5" > test_complex.txt
	@echo "0;#FF0000;0;25;1;" >> test_complex.txt
	@echo "1;#00FF00;3;10;2;" >> test_complex.txt
	@echo "2;#0000FF;5;8;1;" >> test_complex.txt
	@echo "3;#FFFF00;10;15;3;" >> test_complex.txt
	@echo "4;#FF00FF;12;5;1;" >> test_complex.txt
	@echo "5;#00FFFF;15;20;2;" >> test_complex.txt
	./$(TARGET_SIM) test_complex.txt

# Ajuda
help:
	@echo "Comandos disponíveis:"
	@echo "  make (ou make all) - Compila o simulador e a interface (default)"
	@echo "  make simulador     - Compila apenas o simulador"
	@echo "  make interface     - Compila apenas a interface"
	@echo "  make demo          - Compila o demo do Gantt"
	@echo "  make test-fifo     - Testa algoritmo FIFO"
	@echo "  make test-srtf     - Testa algoritmo SRTF"
	@echo "  make test-priority - Testa algoritmo Priority"
	@echo "  make test-debug    - Testa modo passo-a-passo"
	@echo "  make test-all      - Executa todos os testes"
	@echo "  make test-complex  - Testa caso complexo"
	@echo "  make clean         - Limpa arquivos compilados"
	@echo "  make help          - Mostra esta ajuda"

.PHONY: all clean test-fifo test-srtf test-priority test-debug test-all test-complex help

# --- Regras de Dependência de Cabeçalho ---
# Garante que os arquivos .o sejam recompilados se
# os arquivos .h dos quais eles dependem mudarem.

simulador.o: simulador.c gantt_bmp.h gantt_ascii.h stats_viewer.h
interface.o: interface.c gantt_bmp.h gantt_ascii.h
gantt_ascii.o: gantt_ascii.c gantt_bmp.h
gantt_bmp.o: gantt_bmp.c gantt_bmp.h
stats_viewer.o: stats_viewer.c stats_viewer.h