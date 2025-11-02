#!/bin/bash
# setup.sh - Script de configuração inicial do projeto

echo "╔════════════════════════════════════════════════════════════╗"
echo "║           CONFIGURAÇÃO DO SIMULADOR DE SO                 ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Copiar arquivos da interface
echo "Copiando arquivos necessários..."

# Verificar se os arquivos existem
if [ ! -f "simulador.c" ]; then
    echo "⚠ simulador.c não encontrado!"
    exit 1
fi

# Copiar arquivos novos se não existirem
if [ ! -f "interface.c" ]; then
    cp /mnt/user-data/outputs/interface.c . 2>/dev/null && echo "✓ interface.c copiado"
fi

if [ ! -f "stats_viewer.c" ]; then
    cp /mnt/user-data/outputs/stats_viewer.c . 2>/dev/null && echo "✓ stats_viewer.c copiado"
fi

# Fazer backup do Makefile atual
if [ -f "Makefile" ]; then
    mv Makefile Makefile.backup
    echo "✓ Backup do Makefile criado"
fi

# Copiar novo Makefile
cp /mnt/user-data/outputs/Makefile_interface Makefile 2>/dev/null && echo "✓ Novo Makefile instalado"

# Compilar
echo ""
echo "Compilando projeto..."
make clean > /dev/null 2>&1
make

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Instalação concluída com sucesso!"
    echo ""
    echo "Para iniciar a interface interativa:"
    echo "  ./interface"
    echo ""
    echo "Ou use diretamente:"
    echo "  ./simulador test_config.txt"
else
    echo "❌ Erro na compilação!"
    echo "Verifique os erros acima."
fi