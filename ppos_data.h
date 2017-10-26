// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016
//
// Estruturas de dados internas do sistema operacional

#include <ucontext.h>

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

// Estrutura que define uma tarefa
typedef struct task_t
{
   struct task_t *prev, *next ;   // para usar com a biblioteca de filas (cast)
   int tid ;                      // ID da tarefa
   ucontext_t context;
   int status; // 0 para pronta e 1 para suspensa
   int prio_estatica; // prioridade estatica da tarefa
   int prio_dinamica; // prioridade dinamica da tarefa
   int quantum; // contador de quantum (começa em 20)
   int tipo; // define se é tarefa de sistema (0) ou de usuário (1)
   long int tempo_processador; // conta o tempo gasto pelo processador dentro da tarefa
   long int ativacoes; // conta o número de ativações da tarefa
   int id_de_espera; // id da tarefa que está esperando concluir para sair do estado suspenso
   int execucao;
   int exitCode;
   int tempo_acordar; 
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  int value;
  int status;
  struct task_t *fila;
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando for necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando for necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  void* buffer;
  int max_msgs;
  int msg_size; 
  int indice;
  semaphore_t s_buffer, s_item, s_vaga;
} mqueue_t ;

#endif
