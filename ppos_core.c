//-------------------------------------------------//
// Aluno:      Felipe dos Santos Dotti do Prado
// GRR:        20151127
// Trabalho:   P10
// Disciplina: Sistemas Operacionais
//-------------------------------------------------//

#define STACKSIZE 32768 // Definindo o tamanho da pilha para os contextos
#define AGING 1
#define DORMINDO 2
#define SUSPENSA 1
#define PRONTA 0
#define PRIO_MAX 20
#define PRIO_MIN -20
#define PRIO_DEFAULT 0
#define TAREFA_SISTEMA 0
#define TAREFA_USUARIO 1
#define QUANTUM_PADRAO 20
#define FINALIZADA 1
#define EXECUTANDO 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>
#include "queue.h"
#include "ppos_data.h"
#include "ppos.h"
#include <signal.h>
#include <sys/time.h>

int main (int argc, char *argv[]);
void dispatcher_body (); // Protótipos de funções
void trata_tarefas();

task_t *fila_tasks; // Fila para as tarefas prontas
task_t *fila_suspensas; // Fila para as tarefas suspensas
task_t *fila_adormecidas; // Fila para as tarefas fila_adormercidas
task_t *current = NULL; // Variável task_t para guardar a tarefa sendo executada no momento
task_t main_t;  //  Variável global para guardar a função main
task_t dispatcher; // Variável global para guardar a função dispatcher
unsigned long int conta_tempo; // Conta o tempo total de execução do processador
unsigned int id_tasks; // id_tasks para setar o tid de cada tarefa
unsigned int num_tasks;
struct sigaction action; // Tratador de sinal
struct itimerval timer; // Inicialização do timer

void ppos_init () {
  setvbuf (stdout, 0, _IONBF, 0);
  conta_tempo = 0;
  id_tasks = 0;
  num_tasks = 0;

  action.sa_handler = trata_tarefas;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;  
  sigaction (SIGALRM, &action, 0);

  timer.it_value.tv_usec = 1000;  // Definindo o tempo do temporizador como 1 ms
  timer.it_value.tv_sec  = 0; 
  timer.it_interval.tv_usec = 1000;
  timer.it_interval.tv_sec  = 0; 
  setitimer (ITIMER_REAL, &timer, 0);

  main_t.next = NULL;
  main_t.prev = NULL;
  main_t.tid = id_tasks;
  main_t.status = PRONTA;
  main_t.quantum = QUANTUM_PADRAO;
  main_t.ativacoes = 0;
  main_t.tempo_processador = 0;
  main_t.execucao = EXECUTANDO;
  task_setprio (&main_t, PRIO_DEFAULT);
  main_t.tipo = TAREFA_USUARIO;
  id_tasks++;
  queue_append ((queue_t **) &fila_tasks, (queue_t*) &main_t);

  task_create(&dispatcher, (void*)dispatcher_body, NULL);
  dispatcher.tipo = TAREFA_SISTEMA;
  queue_remove((queue_t **) &fila_tasks, (queue_t *) &dispatcher);
  current = &main_t;
}

int cria_contexto(ucontext_t *context, void (*start_func)(void *), void *arg) {
  char *stack;

  int i = getcontext (context);
  if (i != 0) return -1;
  stack = malloc (STACKSIZE);

  if (stack != NULL) {
    context->uc_stack.ss_sp = stack;
    context->uc_stack.ss_size = 32768;
    context->uc_stack.ss_flags = 0;
    context->uc_link = 0;
  }

  else {
    perror ("Erro na criação da pilha\n");
    return -1;
  }

  makecontext((context), (void*)(*start_func), 1, arg);
  return 0;
}

int task_create (task_t *task, void (*start_func)(void *), void *arg) {
  task->next = NULL;
  task->prev = NULL;
  task->tid = id_tasks;
  task->status = PRONTA;
  task->quantum = QUANTUM_PADRAO;
  task->ativacoes = 0;
  task->tempo_processador = 0;
  task->execucao = EXECUTANDO;
  task_setprio (task, PRIO_DEFAULT);
  id_tasks++;

  int i = cria_contexto(&(task->context), (void*)(*start_func), arg);

  if (i != 0) {
    #ifdef DEBUG
    printf ("task_create: falhou ao criar tarefa\n");
    #endif

    return -1;
  }

  else {
    num_tasks++;

    #ifdef DEBUG
    printf ("task_create: criou tarefa %d\n", task->tid) ;
    #endif

    task->tipo = TAREFA_USUARIO;
    queue_append ((queue_t **) &fila_tasks, (queue_t*) task);

    return task->tid;
  }
}

void acorda_suspensas (task_t **queue, int tid, int exitCode) {
	int i, size;
	task_t *aux = *queue;
	task_t *aux2;

  size = queue_size((queue_t *) *queue);

	for (i = 0; i < size; i++) {
		aux2 = aux->next;
		if (aux->id_de_espera == tid) {
			aux->exitCode = exitCode;
			task_resume (aux);
		}
		aux = aux2;
	}
}

void task_exit (int exitCode) {
  #ifdef DEBUG
  printf ("task_exit: tarefa %d sendo encerrada\n", current->tid) ;
  #endif

  if (current == &dispatcher) {
    printf("Task %d exit: execution time %ld ms, processor time %ld ms, %ld activations\n", dispatcher.tid, conta_tempo, dispatcher.tempo_processador, dispatcher.ativacoes);
    task_switch(&main_t);
  }

  acorda_suspensas(&fila_suspensas, current->tid, exitCode);

  queue_remove ((queue_t**) &fila_tasks, (queue_t*) current); // Remove tarefa da fila quando ela terminar

  current->execucao = FINALIZADA;
  printf("Task %d exit: execution time %ld ms, processor time %ld ms, %ld activations\n", current->tid, conta_tempo, current->tempo_processador, current->ativacoes);

  num_tasks--;
  task_switch(&dispatcher);
}

int task_switch (task_t *task) {
  task_t *aux = current;
  current = task;

  #ifdef DEBUG
  printf ("task_switch: trocando contexto %d -> %d\n", aux->tid, current->tid) ;
  #endif
  int i = swapcontext(&(aux->context), &(current->context));

  if (i != 0) return -1;
  else return 0; 
}

int task_id () {
  return current->tid;
}

void task_suspend (task_t *task, task_t **queue) {
  if (task == NULL) task = current;
  	task->status = SUSPENSA;
    queue_remove ((queue_t**) &fila_tasks, (queue_t*) task);
    queue_append ((queue_t **) queue, (queue_t*) task);
}

void task_resume (task_t *task) {
  if (task == NULL) return;
  task->status = PRONTA;
  queue_remove ((queue_t**) &fila_suspensas, (queue_t*) task);
  queue_append ((queue_t **) &fila_tasks, (queue_t*) task);
} 

void task_yield (){	
  if (current != &dispatcher)
  	fila_tasks = fila_tasks->next;
  task_switch(&dispatcher); // 
}

void task_setprio (task_t *task, int prio) {
  if (task == NULL) task = current;
  if (((prio >= PRIO_MIN) && (prio <= PRIO_MAX))) {
    #ifdef DEBUG
    printf ("task_setprio: setando prioridade da tarefa %d\n", task->tid);
    #endif
    task->prio_estatica = prio;
    task->prio_dinamica = prio;
  }
  else {
    #ifdef DEBUG
    printf ("task_setprio: erro ao setar prioridade da tarefa %d\n", task->tid);
    #endif
    return;
  }
}

int task_getprio (task_t *task) {
  if (task == NULL) task = current;
  #ifdef DEBUG
  printf ("task_getprio: obtendo prioridade da tarefa %d\n", task->tid) ;
  #endif  
  return task->prio_estatica;
}

int task_join (task_t *task) {
	if (task == NULL || task->execucao == FINALIZADA)
		return -1;
	else {
		task_t *aux = current;
		task_suspend(current, &fila_suspensas);
		current->id_de_espera = task->tid;
		task_switch(&dispatcher);
		return aux->exitCode;
	}
}

void task_sleep(int sleepTime) {
  current->tempo_acordar = conta_tempo + sleepTime*1000;
  current->status = DORMINDO;
  queue_remove ((queue_t**) &fila_tasks, (queue_t*) current);
  queue_append ((queue_t**) &fila_adormecidas, (queue_t*) current);
  task_switch(&dispatcher);
}

task_t * scheduler() {
  int i, j;
  task_t *proxima = fila_tasks;
  task_t *aux = fila_tasks;

  j = queue_size((queue_t *) fila_tasks);

  for (i = 0; i < j; i++) {
    if ((aux->prio_dinamica < proxima->prio_dinamica)) {
      proxima = aux;
    }
    aux = aux->next; // Percorre a fila de prontas verificando qual é a mais prioritária
  }

  if (proxima == current) return proxima;
  aux = fila_tasks;

  j = queue_size((queue_t *) fila_tasks);

  for (i = 0; i < j; i++) {
    if (aux != proxima) {
      if (aux->prio_dinamica > PRIO_MIN) {
      aux->prio_dinamica -= AGING;
    }
  }
  aux = aux->next; // Percorre a fila "envelhecendo" as tarefas em espera
  }

  task_setprio (proxima, proxima->prio_estatica); // Volta a prioridade dinâmica ao valor inicial
  return proxima  ;//
}

void trata_tarefas() {
  conta_tempo++;
  if (current->tipo != TAREFA_SISTEMA) {
    current->tempo_processador++;
    if (current->quantum > 0) {
      current->quantum--;
    }
    else {
     // current->quantum = QUANTUM_PADRAO;
      task_yield();
    }
  }
  else {
    if (current == &dispatcher)
      dispatcher.tempo_processador++;
    return;
  }
}

void dispatcher_body () {
  int i, j;
  while (num_tasks > 0) {
    if (fila_tasks) {
      dispatcher.ativacoes++;
      task_t *next = scheduler(); 

      if (next != NULL) {
        next->quantum = QUANTUM_PADRAO;
        next->ativacoes++;
        task_switch (next); 
      }
    }

    if (fila_adormecidas) {
      task_t *aux = fila_adormecidas;
      task_t *aux2;
      j = queue_size((queue_t *) fila_adormecidas);
      for (i = 0; i < j; i++) {
        aux2 = aux->next;
        if ((aux->tempo_acordar <= conta_tempo) && (aux->status == DORMINDO)) {
      	 aux->status = PRONTA;
          queue_remove ((queue_t**) &fila_adormecidas, (queue_t*) aux);
          queue_append ((queue_t**) &fila_tasks, (queue_t*) aux);     
        }
        aux = aux2; 
      }
    } 
  }
  task_exit(0);
}

unsigned int systime () {
  return conta_tempo;
}

int sem_create (semaphore_t *s, int value) {
  if (!s) return -1;
  s->value = value;
  s->fila = NULL;
  s->status = EXECUTANDO;
  return 0;
}

int sem_down (semaphore_t *s) {
  if (!s || s->status == FINALIZADA) return -1;
  s->value--;
  if (s->value < 0) {
    task_suspend(current, &(s->fila));
    task_switch(&dispatcher);
  }
  return 0;
}

int sem_up (semaphore_t *s) {
  if (!s || s->status == FINALIZADA) return -1;
  s->value++;
  if (s->value <= 0) {
    task_t *aux = s->fila;
    queue_remove ((queue_t**) &(s->fila), (queue_t*) aux);
    queue_append ((queue_t **) &fila_tasks, (queue_t*) aux);
    aux->status = PRONTA;
  }
  return 0;
}

int sem_destroy (semaphore_t *s) {
  if (!s) return -1;
  int i;
  if (s->fila) {
    task_t *aux = s->fila;
    task_t *aux2;
    int size = queue_size((queue_t *) s->fila);
    for (i = 0; i < size; i++) {
      aux2 = aux->next;
      queue_remove ((queue_t**) &(s->fila), (queue_t*) aux);
      queue_append ((queue_t **) &fila_tasks, (queue_t*) aux); 
      aux = aux2;   
    }
  }
  s->status = FINALIZADA;
  return 0;
}

int mqueue_create (mqueue_t *queue, int max_msgs, int msg_size) {
  if (sem_create (&(queue->s_buffer), 1)) return -1;
  if (sem_create (&(queue->s_item), 0)) return -1;
  if (sem_create (&(queue->s_vaga), max_msgs)) return -1;
  queue->max_msgs = max_msgs;
  queue->msg_size = msg_size;
  queue->buffer = malloc(msg_size * max_msgs);
  queue->indice = 0;
  return 0;
}

int mqueue_send (mqueue_t *queue, void *msg) {
  if (sem_down(&(queue->s_vaga))) return -1;

  if (sem_down(&(queue->s_buffer))) return -1;
  memcpy((queue->buffer + (queue->msg_size * queue->indice)), msg, queue->msg_size);
  queue->indice++;
  if (sem_up(&(queue->s_buffer))) return -1;

  if (sem_up(&(queue->s_item))) return -1;
  return 0;
}

int mqueue_recv (mqueue_t *queue, void *msg) {
  int i;
  if (sem_down(&(queue->s_item))) return -1;

  if (sem_down(&(queue->s_buffer))) return -1;
  memcpy(msg, queue->buffer, queue->msg_size);
  for (i = 0; i < queue->indice-1; i++) 
    memcpy((queue->buffer + (queue->msg_size * i)), (queue->buffer + (queue->msg_size * (i+1))), queue->msg_size);
  queue->indice--;
  if (sem_up(&(queue->s_buffer))) return -1;

  if (sem_up(&(queue->s_vaga))) return -1;
  return 0;
}

int mqueue_destroy (mqueue_t *queue) {
  if (sem_destroy(&(queue->s_buffer))) return -1;
  if (sem_destroy(&(queue->s_item))) return -1;
  if (sem_destroy(&(queue->s_vaga))) return -1;
  free(queue->buffer);
  return 0;
}