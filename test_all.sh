#!/bin/bash
# test_all.sh - Script de teste automatizado para o simulador

echo "========================================"
echo "   TESTE AUTOMATIZADO DO SIMULADOR"
echo "========================================"
echo ""

# Função para criar arquivo de teste
create_test_file() {
    local algorithm=$1
    local filename=$2
    
    echo "$algorithm;10" > $filename
    echo "0;#FF0000;0;20;1;" >> $filename
    echo "1;#00FF00;5;15;2;" >> $filename
    echo "2;#0000FF;10;10;3;" >> $filename
    echo "3;#FFFF00;8;12;1;" >> $filename
    echo "4;#FF00FF;15;8;3;" >> $filename
}

# Compilar se necessário
if [ ! -f "simulador" ]; then
    echo "Compilando simulador..."
    make
    if [ $? -ne 0 ]; then
        echo "Erro na compilação!"
        exit 1
    fi
fi

echo "Verificando se é standalone..."
if command -v ldd &> /dev/null; then
    ldd simulador 2>&1 | grep -q "not a dynamic executable" && echo "✓ Executável é standalone!" || echo "⚠ Executável tem dependências"
fi
echo ""

# Teste 1: FIFO
echo "----------------------------------------"
echo "TESTE 1: Algoritmo FIFO"
echo "----------------------------------------"
create_test_file "FIFO" "test_fifo.txt"
./simulador test_fifo.txt > test_fifo.log 2>&1
if [ $? -eq 0 ]; then
    echo "✓ FIFO executou com sucesso"
    if [ -f "gantt_output.bmp" ]; then
        mv gantt_output.bmp gantt_fifo.bmp
        echo "✓ Gantt chart gerado: gantt_fifo.bmp"
    fi
else
    echo "✗ Erro ao executar FIFO"
fi
echo ""

# Teste 2: SRTF
echo "----------------------------------------"
echo "TESTE 2: Algoritmo SRTF"
echo "----------------------------------------"
create_test_file "SRTF" "test_srtf.txt"
./simulador test_srtf.txt > test_srtf.log 2>&1
if [ $? -eq 0 ]; then
    echo "✓ SRTF executou com sucesso"
    if [ -f "gantt_output.bmp" ]; then
        mv gantt_output.bmp gantt_srtf.bmp
        echo "✓ Gantt chart gerado: gantt_srtf.bmp"
    fi
else
    echo "✗ Erro ao executar SRTF"
fi
echo ""

# Teste 3: PRIORITY
echo "----------------------------------------"
echo "TESTE 3: Algoritmo PRIORITY"
echo "----------------------------------------"
create_test_file "PRIORITY" "test_priority.txt"
./simulador test_priority.txt > test_priority.log 2>&1
if [ $? -eq 0 ]; then
    echo "✓ PRIORITY executou com sucesso"
    if [ -f "gantt_output.bmp" ]; then
        mv gantt_output.bmp gantt_priority.bmp
        echo "✓ Gantt chart gerado: gantt_priority.bmp"
    fi
else
    echo "✗ Erro ao executar PRIORITY"
fi
echo ""

# Teste 4: Caso Complexo
echo "----------------------------------------"
echo "TESTE 4: Caso Complexo (10 tarefas)"
echo "----------------------------------------"
cat > test_complex.txt << EOF
SRTF;5
0;#FF0000;0;25;1;
1;#00FF00;3;10;2;
2;#0000FF;5;8;1;
3;#FFFF00;10;15;3;
4;#FF00FF;12;5;1;
5;#00FFFF;15;20;2;
6;#800080;18;7;1;
7;#FFA500;20;12;3;
8;#008000;22;9;2;
9;#000080;25;6;1;
EOF

./simulador test_complex.txt > test_complex.log 2>&1
if [ $? -eq 0 ]; then
    echo "✓ Caso complexo executou com sucesso"
    if [ -f "gantt_output.bmp" ]; then
        mv gantt_output.bmp gantt_complex.bmp
        echo "✓ Gantt chart gerado: gantt_complex.bmp"
    fi
else
    echo "✗ Erro ao executar caso complexo"
fi
echo ""

# Teste 5: Arquivo inválido
echo "----------------------------------------"
echo "TESTE 5: Tratamento de Erros"
echo "----------------------------------------"
echo "Testando arquivo inexistente..."
./simulador arquivo_inexistente.txt > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "✓ Erro tratado corretamente"
else
    echo "✗ Deveria ter retornado erro"
fi
echo ""

# Teste 6: Verificar saídas
echo "----------------------------------------"
echo "TESTE 6: Verificação de Saídas"
echo "----------------------------------------"
echo "Arquivos gerados:"
ls -la *.bmp 2>/dev/null | awk '{print "  - " $9 " (" $5 " bytes)"}'
ls -la *.log 2>/dev/null | awk '{print "  - " $9 " (" $5 " bytes)"}'
echo ""

# Resumo
echo "========================================"
echo "           RESUMO DOS TESTES"
echo "========================================"
echo ""
echo "Logs gerados:"
for log in *.log; do
    if [ -f "$log" ]; then
        echo "  $log:"
        grep "Médias:" $log | head -1 | sed 's/^/    /'
    fi
done
echo ""

# Limpar arquivos temporários (opcional)
read -p "Deseja limpar arquivos de teste? (s/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Ss]$ ]]; then
    rm -f test_*.txt test_*.log
    echo "Arquivos de teste removidos."
fi

echo ""
echo "Teste concluído!"