/******************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos
 **
 ** Aluno: Nº: 88047  Nome: Rúben Bento
 ** Nome do Módulo: common.h
 ** Descrição/Explicação do Módulo: 
 **     Definição das estruturas de dados comuns aos módulos servidor e cidadao
 **     Os alunos não deverão alterar os nomes dos ficheiros definidos em FILE_*
 **
 ** OS ALUNOS NÃO DEVERÃO ACRESCENTAR NADA A ESTE FICHEIRO!
 **
 ******************************************************************************/
#ifndef __COMMON_H__
#define __COMMON_H__

#include <signal.h>
#include <unistd.h>

#define MIN_PROCESSAMENTO   1   // Tempo mínimo de processamento do Servidor
#define MAX_PROCESSAMENTO   7   // Tempo máximo de processamento do Servidor
#define MAX_ESPERA          7   // Tempo máximo de espera por parte do Cliente
#define NUM_PASSAGENS      20   // Tamanho máximo do buffer de Passagens

typedef struct {
    int tipo_passagem;          // Tipo de Passagem: 1-Normal, 2-Via Verde
    char matricula[9];          // Matrícula da viatura
    char lanco[50];             // Lanço da autoestrada
    int pid_cliente;            // PID do processo (Cliente) que pede passagem
    int pid_servidor_dedicado;  // PID do processo (Servidor) dedicado ao pedido
} Passagem;

typedef struct {
    int contadorNormal;         // Contador de Passagens correspondente ao Tipo de Passagem: 1-Normal
    int contadorViaVerde;       // Contador de Passagens correspondente ao Tipo de Passagem: 2-Via Verde
    int contadorAnomalias;      // Contador de Passagens detetadas com anomalias diversas
} Contadores;

#define FILE_PEDIDOS   "pedidos.fifo"       // FIFO (Named Pipe) que serve para o Cliente fazer os pedidos ao Servidor
#define FILE_SERVIDOR  "servidor.pid"       // Ficheiro que contém o PID do processo Servidor
#define FILE_STATS     "estatisticas.dat"   // Ficheiro que armazena as estatísticas necessárias para o programa

/* OS ALUNOS NÃO DEVERÃO ACRESCENTAR NADA A ESTE FICHEIRO! */

#endif
