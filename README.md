# Simulador de Escalonamento de Processos

Simulador de escalonamento de processos multitarefa preemptivo desenvolvido para a disciplina de Sistemas Operacionais.

## Requisitos Atendidos

| Requisito | Descrição | Status |
|-----------|-----------|--------|
| 1.1 | TCB com campos obrigatórios | ✅ |
| 1.2 | Algoritmos FIFO, RR, SRTF, Priority | ✅ |
| 1.3 | Diagrama de Gantt (ASCII e BMP) | ✅ |
| 1.4 | Estatísticas (turnaround, waiting, etc.) | ✅ |
| 1.5.1 | Modo passo-a-passo | ✅ |
| 1.5.2 | Retroceder simulação | ✅ |
| 1.5.3 | Execução completa | ✅ |
| 1.6 | Código comentado | ✅ |

## Compilação

```bash
make clean
make
```

Gera dois executáveis:
- `simulador` - Simulador principal com linha de comando
- `interface` - Interface interativa com menus

## Uso

### Simulador (linha de comando)

```bash
# Execução interativa
./simulador config.txt

# Com opções
./simulador config.txt --step      # Modo debug passo-a-passo
./simulador config.txt --bmp       # Gerar BMP automaticamente
./simulador config.txt --ascii     # Mostrar Gantt ASCII
./simulador config.txt --quiet     # Modo silencioso
```

### Interface Interativa

```bash
./interface
```

## Modo Debug (Passo-a-Passo)

O modo debug (`--step`) oferece **visualização gráfica em tempo real**:

### Comandos Disponíveis

| Comando | Descrição |
|---------|-----------|
| `Enter` | Avança 1 tick e mostra gráfico |
| `n` | Avança N ticks |
| `b` | Retrocede 1 tick |
| `g` | Vai para tick específico |
| `v` | Ver gráfico de Gantt progressivo |
| `d` | Ver diagrama de estados/filas |
| `i` | Ver tabela de estados |
| `a` | Mostrar tudo (gráfico + diagrama + tabela) |
| `t` | Toggle: ativar/desativar gráfico automático |
| `c` | Continuar até o fim |
| `q` | Sair |
| `?` | Ajuda |

### Gráfico de Gantt Progressivo

Mostra o gráfico sendo construído tick a tick:

```
╔══════════════════════════════════════════════════════════════╗
║              GANTT CHART PROGRESSIVO                         ║
╚══════════════════════════════════════════════════════════════╝

      0    5    10   15   20
      │···▼│····│····│····│····  ◄ Tick 4
      ──────────────────────────
T0  R ███·░░░░░░░░░░░░░░░░░░░░  [wait]
T1  * ···█░░░░░░░░░░░░░░░░░░░░  [1/6]
T2  R ····░░░░░░░░░░░░░░░░░░░░  [wait]
      ──────────────────────────

Legenda: █=Executando  ·=Esperando  ░=Futuro  ▼=Tick atual
Estados: N=New  R=Ready  *=Running  B=Blocked  D=Done
```

### Diagrama de Estados

Mostra visualmente as filas do escalonador:

```
┌──────────────────────────────────────────────────────┐
│                    DIAGRAMA DE ESTADOS                 │
└──────────────────────────────────────────────────────────┘

  CPU: [T1]
  READY: [T0] [T2]
  WAITING: [vazia]
  DONE: [vazia]

  Progresso: [███████░░░░░░░░░░░░░░░░░░░░░░░] 25%
```

## Formato do Arquivo de Configuração

```
ALGORITMO;QUANTUM
ID;COR;CHEGADA;BURST;PRIORIDADE;
```

### Exemplo (Round-Robin com quantum 3)

```
RR;3
0;#FF0000;0;10;1;
1;#00FF00;1;6;2;
2;#0000FF;2;4;3;
```

### Algoritmos Disponíveis

| Algoritmo | Descrição |
|-----------|-----------|
| `FIFO` | First In, First Out (não-preemptivo) |
| `RR` | Round-Robin (preemptivo com quantum) |
| `SRTF` | Shortest Remaining Time First (preemptivo) |
| `PRIORITY` | Por prioridade (preemptivo, menor = maior prioridade) |

## Estrutura de Arquivos

```
├── simulador.c      # Código principal (com visualização debug)
├── interface.c      # Interface interativa
├── gantt_bmp.c/h    # Geração de BMP
├── gantt_ascii.c/h  # Visualização ASCII
├── stats_viewer.c/h # Estatísticas
├── Makefile         # Script de compilação
└── exemplo_*.txt    # Arquivos de exemplo
```

## Exemplos Incluídos

- `exemplo_fifo.txt` - FIFO com 3 tarefas
- `exemplo_rr.txt` - Round-Robin com quantum 3
- `exemplo_srtf.txt` - SRTF com preempção
- `exemplo_priority.txt` - Escalonamento por prioridade

## Preparação para Projeto B

O código já inclui estruturas preparadas para o Projeto B:
- Estrutura `TaskEvent` para eventos de mutex e I/O
- Campos `io_remaining`, `events[]` no TCB
- Parser de eventos (`parse_events()`)

## Autor

Desenvolvido para a disciplina de Sistemas Operacionais.