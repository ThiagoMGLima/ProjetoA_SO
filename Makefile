# Makefile para o Simulador de Escalonamento
# ------------------------------------------
# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99

# Arquivos fonte e objeto
SOURCES = simulador.c gantt_bmp.c gantt_ascii.c stats_viewer.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = simulador

# Interface (opcional)
INTERFACE_SRC = interface.c gantt_bmp.c gantt_ascii.c
INTERFACE_TARGET = interface

# Regra principal - compila simulador e interface
all: $(TARGET) $(INTERFACE_TARGET)

# Compilar o simulador
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Compilar interface (depende dos objetos necessários)
$(INTERFACE_TARGET): interface.o gantt_bmp.o gantt_ascii.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Regra genérica para objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpar arquivos gerados
clean:
	rm -f $(OBJECTS) interface.o $(TARGET) $(INTERFACE_TARGET) *.bmp *.csv

# Teste rápido
test: $(TARGET)
	@echo "Criando arquivo de teste..."
	@echo "FIFO;10" > test_config.txt
	@echo "0;#FF0000;0;10;1;" >> test_config.txt
	@echo "1;#00FF00;2;8;2;" >> test_config.txt
	@echo "2;#0000FF;5;6;3;" >> test_config.txt
	@echo "Executando simulação..."
	@echo -e "n\nn\ns" | ./$(TARGET) test_config.txt
	@echo "Teste concluído! Verifique gantt_output.bmp"

# Dependências
simulador.o: simulador.c gantt_bmp.h gantt_ascii.h stats_viewer.h
gantt_bmp.o: gantt_bmp.c gantt_bmp.h
gantt_ascii.o: gantt_ascii.c gantt_ascii.h gantt_bmp.h
stats_viewer.o: stats_viewer.c stats_viewer.h
interface.o: interface.c gantt_bmp.h gantt_ascii.h

.PHONY: all clean test