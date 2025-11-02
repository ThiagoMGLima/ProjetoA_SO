# Simulador de Sistema Operacional Multitarefa

## 📋 Status de Implementação

### ✅ Implementado:
- [x] Geração de gráfico de Gantt em BMP (standalone)
- [x] Parser de arquivo de configuração
- [x] Task Control Block (TCB)
- [x] Sistema de clock com ticks
- [x] Algoritmo FIFO
- [x] Algoritmo SRTF (Shortest Remaining Time First)
- [x] Algoritmo Priority (com preempção)
- [x] Modo de execução completa
- [x] Modo de execução passo-a-passo (debug)
- [x] Cálculo de estatísticas (turnaround, waiting time)
- [x] Visualização ASCII no terminal
- [x] Compilação 100% standalone (sem dependências)

## 🛠️ Compilação

```bash
# Compilar o simulador
make

# Ou manualmente:
gcc -static -O2 simulator.c gantt_bmp.c -o simulador -lm
```

## 🚀 Como Usar

### 1. Formato do Arquivo de Configuração

```
algoritmo_escalonamento;quantum
id;cor;ingresso;duracao;prioridade;lista_eventos
```

Exemplo (`config.txt`):
```
FIFO;10
0;#FF0000;0;20;1;
1;#00FF00;5;15;2;
2;#0000FF;10;10;3;
```

### 2. Executar Simulação

#### Modo Normal (execução completa):
```bash
./simulador config.txt
```

#### Modo Debug (passo-a-passo):
```bash
./simulador config.txt --step
```

No modo debug, comandos disponíveis:
- `Enter` - Próximo tick
- `c` - Continuar execução completa
- `i` - Inspecionar estado do sistema
- `q` - Sair

### 3. Saídas Geradas

- **Terminal**: Visualização ASCII e estatísticas
- **gantt_output.bmp**: Gráfico de Gantt visual

## 📊 Algoritmos de Escalonamento

### FIFO (First In First Out)
- Não-preemptivo
- Ordem de chegada

### SRTF (Shortest Remaining Time First)
- Preemptivo
- Menor tempo restante primeiro

### PRIORITY
- Preemptivo
- Menor valor = maior prioridade

## 🧪 Testes Rápidos

```bash
# Testar FIFO
make test-fifo

# Testar SRTF
make test-srtf

# Testar Priority
make test-priority

# Testar modo debug
make test-debug

# Executar todos os testes
make test-all

# Caso complexo com 6 tarefas
make test-complex
```

## 📂 Estrutura de Arquivos

```
projeto/
├── simulator.c      # Núcleo do simulador
├── gantt_bmp.c     # Geração de BMP
├── gantt_bmp.h     # Header do Gantt
├── Makefile        # Build do projeto
├── test_config.txt # Arquivo de teste
└── README.md       # Esta documentação
```

## 📈 Exemplo de Saída

```
=== INICIANDO SIMULAÇÃO (FIFO) ===
[Tick 0] Tarefa 0 chegou
[Tick 0] Executando tarefa 0
[Tick 5] Tarefa 1 chegou
[Tick 10] Tarefa 2 chegou
[Tick 19] Tarefa 0 concluída
[Tick 20] Executando tarefa 1
...

=== ESTATÍSTICAS DAS TAREFAS ===
ID | Arrival | Burst | Complete | Turnaround | Waiting
---|---------|-------|----------|------------|--------
 0 |       0 |    20 |       20 |         20 |       0
 1 |       5 |    15 |       35 |         30 |      15
 2 |      10 |    10 |       45 |         35 |      25

Médias: Turnaround = 28.33, Waiting = 13.33
Gantt chart salvo em: gantt_output.bmp
```

## 🎯 Requisitos Atendidos

### Requisitos Gerais:
- ✅ Simulação de SO multitarefa preemptivo
- ✅ Visualização gráfica (BMP)
- ✅ Sistema configurável
- ✅ Executável standalone
- ✅ Código comentado

### Projeto A:
- ✅ Relógio com ticks
- ✅ CPU única
- ✅ TCB para tarefas
- ✅ Características de tempo
- ✅ Modo passo-a-passo
- ✅ Modo execução completa
- ✅ Gráfico de Gantt
- ✅ Arquivo de imagem (BMP)
- ✅ Configuração por arquivo
- ✅ Algoritmos FIFO, SRTF, Priority
- ✅ Escalonador modular

## 🐛 Debugging

Para verificar se é realmente standalone:
```bash
# Linux
ldd simulador

# Windows
objdump -p simulador.exe | grep DLL

# MacOS
otool -L simulador
```

## 📝 Notas de Implementação

1. **BMP Manual**: Implementado sem bibliotecas externas
2. **Parser Robusto**: Suporta linhas vazias e campos opcionais
3. **Gantt Otimizado**: Agrupa execuções contínuas
4. **Estatísticas Completas**: Turnaround e waiting time calculados

## 🚧 Próximos Passos (Projeto B)

1. Implementar eventos (mutex, I/O)
2. Adicionar múltiplas CPUs
3. Resolver inversão de prioridade
4. Implementar Round Robin
5. Adicionar mais visualizações

## 📧 Autor

Prof. Dr. Marco Aurélio Wehrmeister
Implementação do simulador conforme especificação v0.1