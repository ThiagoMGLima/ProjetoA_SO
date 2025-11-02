# Simulador de Sistema Operacional Multitarefa

> Simulador educacional de escalonamento de processos com visualização em terminal e geração de Gantt (BMP). Implementação standalone sem dependências externas.

---

## Conteúdo rápido
- 📦 Compilação: `make`
- ▶️ Execução: `./simulador config.txt`
- 🧪 Testes: `make test-all`
- ✍️ Formato do arquivo de configuração: veja a seção **Configuração** abaixo

---

## Descrição
Este projeto simula um núcleo de escalonamento de processos (CPU única) com suporte a múltiplos algoritmos (FIFO, SRTF e Priority), modo passo-a-passo (debug), Gantt gráfico em BMP e saída ASCII no terminal. Foi desenvolvido para uso didático e como base para extensões (I/O, mutexes, múltiplas CPUs, etc.).

> Trabalho base produzido por Dr. Marco Aurélio Wehrmeister implementada por mim Thiago Moreira

---

## Principais recursos (implementados)
- Parser de arquivo de configuração robusto (linhas vazias e campos opcionais)
- Task Control Block (TCB) para cada tarefa
- Relógio por ticks e sistema de scheduler modular
- Algoritmos: FIFO (não preemptivo), SRTF (preemptivo), Priority (preemptivo)
- Execução completa e modo passo-a-passo (debug)
- Geração de gráfico de Gantt em BMP (implementado manualmente, sem bibliotecas)
- Visualização ASCII no terminal
- Cálculo de estatísticas: turnaround time, waiting time
- Compilação standalone (binário sem dependências dinamicamente vinculadas esperadas)

---

## Requisitos
- Compilador C (gcc recomendado)
- Make (opcional, facilita testes)
- Sistema operacional UNIX-like para os exemplos (Linux / macOS). O código é em C e pode ser portado para Windows com ajustes no Makefile.

---

## Como compilar
```bash
# Usando Makefile (recomendado)
make

# Ou compilação manual (exemplo):
gcc -static -O2 simulator.c gantt_bmp.c -o simulador -lm
```

> Verifique o resultado com `ldd simulador` (Linux) para checar bibliotecas vinculadas, se desejar.

---

## Formato do arquivo de configuração
O arquivo de configuração controla o algoritmo e lista as tarefas.

**Linha 1:** `algoritmo_escalonamento;quantum` (quantum pode ser ignorado para algoritmos que não usam quantum)

**Linhas seguintes (uma por tarefa):**
```
id;cor;ingresso;duracao;prioridade;lista_eventos
```
- `id`: identificador numérico da tarefa (ex.: `0`, `1`)
- `cor`: cor em hex para o Gantt (ex.: `#FF0000`) — opcional, usar `#000000` como padrão
- `ingresso`: tick de chegada (integer, ex.: `5`)
- `duracao`: tempo de CPU necessário (integer)
- `prioridade`: inteiro, menor valor = maior prioridade
- `lista_eventos`: campo reservado para eventos (I/O, mutex). Atualmente não usado — deixe vazio ou remova.

**Exemplo completo (`config.txt`):**
```
FIFO;10
0;#FF0000;0;20;1;
1;#00FF00;5;15;2;
2;#0000FF;10;10;3;
```

---

## Uso
### Execução (modo normal)
```bash
./visualizador
```
ou para teste rapido:
```bash
./simulador config.txt
```

### Modo debug (passo-a-passo)
```bash
./simulador config.txt --step
```
No modo debug, comandos disponíveis:
- `Enter` — avançar 1 tick
- `c` — continuar execução completa
- `i` — inspecionar estado (fila, TCBs, tick atual)
- `q` — sair

---

## Saídas
- **Terminal**: logs em ASCII (chegadas, mudanças de contexto, conclusão) e tabela de estatísticas
- **Arquivo BMP**: `gantt_output.bmp` com o gráfico de Gantt da execução

### Exemplo resumido de saída
```
=== INICIANDO SIMULAÇÃO (FIFO) ===
[Tick 0] Tarefa 0 chegou
[Tick 0] Executando tarefa 0
[Tick 5] Tarefa 1 chegou
[Tick 19] Tarefa 0 concluída
...
=== ESTATÍSTICAS DAS TAREFAS ===
ID | Arrival | Burst | Complete | Turnaround | Waiting
---|---------|-------|----------|------------|--------
 0 |       0 |    20 |       20 |         20 |       0
Médias: Turnaround = 28.33, Waiting = 13.33
Gantt chart salvo em: gantt_output.bmp
```

---

## Algoritmos suportados
- **FIFO** — First In First Out (não-preemptivo)
- **SRTF** — Shortest Remaining Time First (preemptivo)
- **PRIORITY** — Prioridade (preemptivo; menor valor = maior prioridade)

---

## Testes (Makefile)
O Makefile inclui alvos para testar cenários pré-definidos:
```bash
make test-fifo
make test-srtf
make test-priority
make test-debug
make test-all
make test-complex
```

---

## Estrutura de arquivos
```
projeto/
├── simulator.c      # Núcleo do simulador
├── gantt_bmp.c      # Geração de BMP
├── gantt_bmp.h      # Header do Gantt
├── Makefile         # Build e targets de teste
├── test_config.txt  # Exemplo de configuração/teste
└── README.md        # Documentação (esta)
```

---

## Desenvolvimento futuro (roadmap)
- Implementar eventos reais (I/O, mutex, semáforos)
- Round Robin (quantum)
- Suporte a múltiplas CPUs
- Tratamento de inversão de prioridade
- Exportação para formatos além de BMP (PNG, SVG)

---

## Licença
Este projeto não possui licença explicitada no repositório original. Recomenda-se adicionar uma licença (ex.: MIT) se desejar permitir contribuições de terceiros.

