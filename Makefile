# Makefile para Simulador de SO Multitarefa
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -static -static-libgcc
TARGET = simulador

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
    TARGET = simulador.exe
    LDFLAGS += -lm
endif

# Arquivos fonte
SRCS = simulador.c gantt_bmp.c
OBJS = $(SRCS:.c=.o)

# Regra principal
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "====================================="
	@echo "Executável gerado: $(TARGET)"
	@echo "====================================="
	@echo "Verificando dependências..."
	@file $(TARGET)
	@echo ""
	@echo "Para testar, execute:"
	@echo "  ./$(TARGET) test_config.txt"
	@echo "  ./$(TARGET) test_config.txt --step (modo para debug)"

# Compilar objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Demo simples
demo: demo.o gantt_bmp.o
	$(CC) demo.o gantt_bmp.o -o demo $(LDFLAGS)

# Limpar
clean:
	rm -f $(OBJS) $(TARGET) demo demo.o *.bmp *.svg

# Testar com FIFO
test-fifo: $(TARGET)
	@echo "FIFO;10" > test_fifo.txt
	@echo "0;#FF0000;0;20;1;" >> test_fifo.txt
	@echo "1;#00FF00;5;15;2;" >> test_fifo.txt
	@echo "2;#0000FF;10;10;3;" >> test_fifo.txt
	./$(TARGET) test_fifo.txt

# Testar com SRTF
test-srtf: $(TARGET)
	@echo "SRTF;10" > test_srtf.txt
	@echo "0;#FF0000;0;20;1;" >> test_srtf.txt
	@echo "1;#00FF00;5;15;2;" >> test_srtf.txt
	@echo "2;#0000FF;10;5;3;" >> test_srtf.txt
	./$(TARGET) test_srtf.txt

# Testar com Priority
test-priority: $(TARGET)
	@echo "PRIORITY;10" > test_priority.txt
	@echo "0;#FF0000;0;20;3;" >> test_priority.txt
	@echo "1;#00FF00;5;15;1;" >> test_priority.txt
	@echo "2;#0000FF;10;10;2;" >> test_priority.txt
	./$(TARGET) test_priority.txt

# Testar modo debug
test-debug: $(TARGET)
	./$(TARGET) test_config.txt --step

# Testar todos
test-all: test-fifo test-srtf test-priority
	@echo "Todos os testes concluídos!"

# Criar arquivo de teste complexo
test-complex:
	@echo "SRTF;5" > test_complex.txt
	@echo "0;#FF0000;0;25;1;" >> test_complex.txt
	@echo "1;#00FF00;3;10;2;" >> test_complex.txt
	@echo "2;#0000FF;5;8;1;" >> test_complex.txt
	@echo "3;#FFFF00;10;15;3;" >> test_complex.txt
	@echo "4;#FF00FF;12;5;1;" >> test_complex.txt
	@echo "5;#00FFFF;15;20;2;" >> test_complex.txt
	./$(TARGET) test_complex.txt

# Ajuda
help:
	@echo "Comandos disponíveis:"
	@echo "  make          - Compila o simulador"
	@echo "  make demo     - Compila o demo do Gantt"
	@echo "  make test-fifo    - Testa algoritmo FIFO"
	@echo "  make test-srtf    - Testa algoritmo SRTF"
	@echo "  make test-priority - Testa algoritmo Priority"
	@echo "  make test-debug   - Testa modo passo-a-passo"
	@echo "  make test-all     - Executa todos os testes"
	@echo "  make test-complex - Testa caso complexo"
	@echo "  make clean        - Limpa arquivos compilados"
	@echo "  make help         - Mostra esta ajuda"

.PHONY: clean test-fifo test-srtf test-priority test-debug test-all test-complex help