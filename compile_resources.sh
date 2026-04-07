#!/bin/bash
# Script para compilar recursos com SGDK
# Supondo que SGDK esteja instalado em /path/to/sgdk

export SGDK=/path/to/sgdk  # Ajuste o caminho
$SGDK/bin/rescomp resources.res src/resources.c src/resources.h