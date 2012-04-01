/*
  Author: Vlad Burca
  Date: 3/10/2012
  Last Edited: 3/26/2012
  File: proj1.c

  Description: Simulation of a priority based scheduling system.

*/
#include <stdio.h>
#include <stdbool.h>
#define MAX_SIZE_Q 10
#define MAX_SIZE_BURST 10
#define MAX_SIZE_PROCS 80

typedef struct {
  char id;
  char priority;
  int arr_time;
  int IO_burst[MAX_SIZE_BURST], CPU_burst[MAX_SIZE_BURST];
  int curr_CPU_burst;
  int CPU_bursts, IO_bursts;
  int turnaround;
  int CPU_wait, IO_wait;
  int IO_entry, CPU_entry;
  int IO_run, CPU_run;
  int quantum;
} process;

typedef struct {
  process Q[MAX_SIZE_Q];
  int head, tail;
  int quantum;
} queue;

typedef struct {
  char id;
  int turnaround;
  int CPU_wait, IO_wait;
} process_table_entry;

typedef struct {
  process_table_entry entries[MAX_SIZE_PROCS];
  int size;
} process_table;

typedef struct {
  process running;
  int free;
} device;

/* Declarations for all the Queues used in the system. */
queue Q1, Q2, Q3, Q_IO;

/* Declaration for the table that will hold all processes
   that go through the system */
process_table proc_table;

/* Declarations for the processes running on CPU and IO. */
process CPU_process, IO_process;

/* Declarations for the devices of the system. */
device CPU, IO;

/* Declares a NULL_PROCESS object. */
process NULL_PROCESS = {'*'};

/* Declares the Gantt Charts for the devices. */
char CPU_GANTT[MAX_SIZE_PROCS], IO_GANTT[MAX_SIZE_PROCS]; 

void set_quantums(int x, int y, int z) {
  Q1.quantum = x;
  Q2.quantum = y;
  Q3.quantum = z;
}

/* Initializes all necessary variables for the Queues.
*/
void init(int x, int y, int z) {
  Q1.head = 0;
  Q1.tail = 0;
  Q2.head = 0;
  Q2.tail = 0;
  Q3.head = 0;
  Q3.tail = 0; // set the initial sizes
  CPU.free = true;
  CPU.running = NULL_PROCESS;
  IO.free = true; 
  IO.running = NULL_PROCESS; // set states of devices
  proc_table.size = 0;
  set_quantums(x, y, z);
}

/* Returns the size of a Queue.
*/
int size(queue Q) {
  return ((MAX_SIZE_Q - Q.head + Q.tail) % MAX_SIZE_Q);
}

/* Returns true/false if the Queue is empty.
*/
bool is_empty(queue Q) {
  if (size(Q) == 0)
    return true;
  return false;
}

/* Enqueues a new process at the tail of the Queue.
   Returns -1 if it fails - Q is full - else 0.
*/
int enqueue(queue *Q, process P) {
  if (size(*Q) == MAX_SIZE_Q)
    return -1;
  else {
    Q->Q[Q->tail] = P;
    Q->tail = (Q->tail + 1) % MAX_SIZE_Q;
    return 0;
  }
}

/* Dequeues the process at the head of the Queue.
   Returns the dequeued process;
      if it fails, returns the NULL PROCESS (P.id = '*')
*/
process dequeue(queue *Q) {
  if (is_empty(*Q)) {
    return NULL_PROCESS;
  }    
  else {
    process P = Q->Q[Q->head];
    Q->head = (Q->head + 1) % MAX_SIZE_Q;
    return P;
  }
}   


/* Reads a process from the incoming stream;
   Returns the read process;
      if it fails, returns the NULL PROCESS (P.id = '*')
*/
process read_proc() {
  process P;
  int check;
  int cpu_b_i = 0, io_b_i = 0;
  // if I can read proc ID, priority, arr_time and
  // one CPU_burst time.
  if (scanf("%c %c %d %d", &P.id, &P.priority,
                             &P.arr_time, 
                             &P.CPU_burst[cpu_b_i++]) != EOF) {
    scanf("%d", &check);  // check if we have more CPU/IO burst times
    while (check != -1) { // while we do have more
      P.IO_burst[io_b_i++] = check; // check is a IO burst time
      scanf("%d %d", &P.CPU_burst[cpu_b_i++], &check); // read more
    } // done with reading the process
    scanf("%*[\n]");  // get rid of new line at end of line
    P.CPU_bursts = cpu_b_i;  // set the total number of CPU bursts
    P.IO_bursts = io_b_i;    // set the total number of IO bursts
    P.turnaround = 0;        // fill in all other fields of the new process
    P.CPU_wait = 0;
    P.IO_wait = 0;
    P.IO_entry = -1;
    P.CPU_entry = -1;
    P.IO_run = 0;
    P.CPU_run = 0;
    P.curr_CPU_burst = 0;
    switch (P.priority) {
      case 'P': P.quantum = Q1.quantum; break;
      case 'U': P.quantum = Q2.quantum; break;
    }
    return P;
  }
  return NULL_PROCESS;
}

/* Checks if the currently pending process can get in the system
   at this time.
   Returns the new pending process.
*/
process check_new_jobs(process P, int clock) {
  while (P.arr_time == clock) {
    switch(P.priority) {
      case 'P': enqueue(&Q1, P); break;
      case 'U': enqueue(&Q2, P); break; 
    }
    P = read_proc();
  }
  return P;
}

/* Checks if the job on the IO device has completed task.
*/
void IO_job_done_check(int clock) { 
  if (!IO.free &&
      IO.running.IO_burst[IO.running.IO_run] == 
        (clock - IO.running.IO_entry)) {
    IO.running.IO_run++;  
    switch (IO.running.priority) {     // move the job to CPU queue
      case 'P': enqueue(&Q1, IO.running); break;
      case 'U': { IO.running.quantum = Q2.quantum;
                  enqueue(&Q2, IO.running); break;
                }
    }
    IO.running = NULL_PROCESS;
    IO.free = true;
  }
}

/* Checks if the job on the CPU device has completed task.
*/
void CPU_job_done_check(int clock) {
  if (!CPU.free) {
    CPU.running.curr_CPU_burst++; // increment its current CPU burst
    if(CPU.running.CPU_burst[CPU.running.CPU_run] == 
                  CPU.running.curr_CPU_burst) {
      if (CPU.running.CPU_run == CPU.running.CPU_bursts - 1) { 
        // job ran all its bursts
        CPU.running.turnaround = clock - CPU.running.arr_time; // process is done and will exit system
        // save the relevant information
        process_table_entry pte = { CPU.running.id,
                                    CPU.running.turnaround,
                                    CPU.running.CPU_wait,
                                    CPU.running.IO_wait};
        proc_table.entries[proc_table.size] = pte;
        proc_table.size++;
      } else {// job still has to do work on system
        CPU.running.CPU_run++;
        CPU.running.curr_CPU_burst = 0;
        enqueue(&Q_IO, CPU.running);  // put it on the IO queue
      }
      CPU.running = NULL_PROCESS;     // remove it from CPU
      CPU.free = true;
    }
  }
}

/* Checks if the job on the CPU devices has exceeded the quantum.
*/
// Note: This requires INTENSE TESTING.
void quantum_check(int clock) {
  if (!CPU.free && CPU.running.priority == 'U') {
//    if ((clock - CPU.running.curr_CPU_burst) >= 6) { // job gets demoted to Q3
    if (CPU.running.curr_CPU_burst == 6) { //job gets demoted to Q3
      CPU.running.quantum = Q3.quantum;
      if (!is_empty(Q1) || !is_empty(Q2) || !is_empty(Q3)) {
        // there is at least one other job waiting in higher / equal priority
        enqueue(&Q3, CPU.running);
        CPU.running = NULL_PROCESS;
        CPU.free = true;
      }
    } else
      if ((clock - CPU.running.CPU_entry) >= CPU.running.quantum)
        if (CPU.running.quantum == Q2.quantum)  { 
          // detect the queue the job is coming from 
          if (!is_empty(Q1) || !is_empty(Q2)) {
            // there is at least one other job waiting in higher / equal
            // priority
            enqueue(&Q2, CPU.running);
            CPU.running = NULL_PROCESS;
            CPU.free = true;
          }
        else
          if (CPU.running.quantum == Q3.quantum)
            if (!is_empty(Q1) || !is_empty(Q2) || !is_empty(Q3)) {
              // there is at least one other job waiting in higher / equal
              // priority
              enqueue(&Q3, CPU.running);
              CPU.running = NULL_PROCESS;
              CPU.free = true;
            }
        }        
  }
}

/* Checks if new job has to be loaded on CPU.
   Creates Gantt Chart entry for this clock value.
*/
void CPU_check(int clock) {
  if (CPU.free) 
    if (!is_empty(Q1) || !is_empty(Q2) || !is_empty(Q3)) {
      if (!is_empty(Q1)) 
        CPU.running = dequeue(&Q1);
      else
        if (!is_empty(Q2))
          CPU.running = dequeue(&Q2);
        else
          if (!is_empty(Q3))
            CPU.running = dequeue(&Q3);
      CPU.running.CPU_entry = clock;
      CPU.free = false;
    }
  CPU_GANTT[clock] = CPU.running.id;
}      

/* Checks if new job has to be loaded on IO.
   Creates Gantt Chart entry for this clock value.
*/
void IO_check(int clock) {
  if (IO.free)
    if (!is_empty(Q_IO)) {
      IO.running = dequeue(&Q_IO);
      IO.running.IO_entry = clock;
      IO.free = false;
    }
  IO_GANTT[clock] = IO.running.id;
}

/* Increments the waiting time for jobs on CPU Queue or IO Queue.
*/
void incr_waiting() {
  int i;
  for (i = 0; i < size(Q1); i++)
    Q1.Q[(i + Q1.head) % MAX_SIZE_Q].CPU_wait++;
  for (i = 0; i < size(Q2); i++)
    Q2.Q[(i + Q2.head) % MAX_SIZE_Q].CPU_wait++;
  for (i = 0; i < size(Q3); i++)
    Q3.Q[(i + Q3.head) % MAX_SIZE_Q].CPU_wait++;
  for (i = 0; i < size(Q_IO); i++)
    Q_IO.Q[(i + Q_IO.head) % MAX_SIZE_Q].IO_wait++;
}

/* Checks if there are no more jobs on system and no more jobs
   waiting to enter the system.
*/
bool empty_system(process P) {
  if(is_empty(Q1) && is_empty(Q2) && is_empty(Q3) && is_empty(Q_IO) &&
      CPU.free && IO.free &&
      P.id == '*')
    return true;
  return false;
}

/* Prints the Gantt Chart for a particular device.
*/
void print_gantt(char GANTT[], int clock) {
  int i;
  printf(" ");
  for (i = 0; i < clock; i++) {
    printf("%c", GANTT[i]);
    if ((i + 1) % 10 == 0)
      printf("|");
    if ((i + 1) % 40 == 0)
      printf("\n ");
  }
  printf("\n");
}

/* Prints the additional information for each process that went
   through the system:
    Process ID, Turnaround Time, CPU Wait Time, IO Wait Time.
*/
void print_add_info_process() {
  int i;
  for (i = 0; i < proc_table.size; i++) {
    printf(" # Process ID: %c \n", proc_table.entries[i].id);
    printf("     Turnaround Time: %d \n", proc_table.entries[i].turnaround);
    printf("     CPU Wait Time: %d \n", proc_table.entries[i].CPU_wait);
    printf("     IO Wait Time: %d \n", proc_table.entries[i].IO_wait);
    printf("\n");
  }
}

/* Prints the additional information for the system that ran the
   processes:
    Average Turnaround Time, Total CPU Wait Time, Total IO Wait Time.
*/
void print_system_info(int clock) {
  float avg_turnaround = 0;
  float avg_CPU_wait = 0;
  float avg_IO_wait = 0;
  int i;
  float CPU_utilization = 0;
  float IO_utilization = 0;
  for (i = 0; i < proc_table.size; i++) {
    avg_turnaround += proc_table.entries[i].turnaround;
    avg_CPU_wait += proc_table.entries[i].CPU_wait;
    avg_IO_wait += proc_table.entries[i].IO_wait;
  }
  avg_turnaround /= proc_table.size;
  avg_CPU_wait /= proc_table.size;
  avg_IO_wait /= proc_table.size;
  printf(" Average Turnaround Time: %.2f \n", avg_turnaround);
  printf(" Average CPU Wait Time: %.2f \n", avg_CPU_wait);
  printf(" Average IO Wait Time: %.2f \n", avg_IO_wait);
  for (i = 0; i < clock; i++) {
    if (CPU_GANTT[i] != '*')
      CPU_utilization++;
    if (IO_GANTT[i] != '*')
      IO_utilization++;
  }
  CPU_utilization /= clock;
  IO_utilization /= clock;
  printf(" CPU Utilization: %.2f%% \n", CPU_utilization * 100);
  printf(" IO Utilization: %.2f%% \n", IO_utilization * 100);
}   

/* Testing function to print a specific queue.
*/
void print_queue(queue q) {
  int i, j;
  for (i = 0; i < size(q); i++) {
    printf("\n\n Job ID: %c\n", q.Q[i].id);
    printf(" Priority: %c\n", q.Q[i].priority);
    printf(" Arrival Time: %d\n", q.Q[i].arr_time);
    printf(" CPU Burst Times: ");
    for (j = 0; j < q.Q[i].CPU_bursts; j++)
      printf("%d ", q.Q[i].CPU_burst[j]);
    printf("\n IO Burst Times: ");
    for (j = 0; j < q.Q[i].IO_bursts; j++)
      printf("%d ", q.Q[i].IO_burst[j]);
    printf("\n");
  }
}

/* Debugging functioni that prints various information at the system runtime.
*/
void PRINT_DEBUG(int clock) {
  int i;
  printf(" # Clock: %d \n", clock);
  printf("    Q1 Size: %d \n", size(Q1));
  printf("    Q1 = ");
  for (i = 0; i < size(Q1); i++)
    printf("%c ", Q1.Q[i].id);
  printf("\n");
  printf("    Q2 Size: %d \n", size(Q2));
  printf("    Q2 = ");
  for (i = 0; i < size(Q2); i++)
    printf("%c ", Q2.Q[i].id);
  printf("\n");
  printf("    Q3 Size: %d \n", size(Q3));
  printf("    Q3 = ");
  for (i = 0; i < size(Q3); i++)
    printf("%c ", Q3.Q[i].id);
  printf("\n");
  printf("    Q_IO Size: %d \n", size(Q_IO));
  printf("    Q_IO = ");
  for (i = 0; i < size(Q_IO); i++)
    printf("%c ", Q_IO.Q[i].id);
  printf("\n");
  printf("    CPU.free = %s \n", (CPU.free)?"true":"false");
  printf("    CPU.running.id = %c - run = %d, burst = %d, curr_CPU_burst = %d, current = %d \n", 
                CPU.running.id, CPU.running.CPU_run, 
                CPU.running.CPU_burst[CPU.running.CPU_run],
                CPU.running.curr_CPU_burst,
                (clock - CPU.running.CPU_entry));
  printf("    IO.free = %s \n", (IO.free)?"true":"false");  
  printf("    IO.running.id = %c - run = %d, burst = %d, current = %d \n", 
                IO.running.id, IO.running.IO_run, 
                IO.running.IO_burst[IO.running.IO_run],
                (clock - IO.running.IO_entry));
  sleep(5);
}


int main() {

  int clock = -1;
  init(-1,3,5);
  process pending_proc = read_proc();
  while (!empty_system(pending_proc)) {
    clock++;
    if (pending_proc.id != NULL_PROCESS.id) // if there are pending jobs left
      pending_proc = check_new_jobs(pending_proc, clock);
    IO_job_done_check(clock);
    CPU_job_done_check(clock);
    quantum_check(clock);
    CPU_check(clock);
    IO_check(clock);
    incr_waiting();
//    PRINT_DEBUG(clock); // DEBUGGING PURPOSE ONLY.
  }
  // output of the gathered information
  printf("\n CPU Gantt Chart: \n");
  print_gantt(CPU_GANTT, clock);
  printf("\n IO Gantt Chart: \n");
  print_gantt(IO_GANTT, clock);
  printf("\n Additional information per process: \n");
  printf(" *********************************** \n");
  print_add_info_process();
  printf("\n System Information: \n");
  printf(" ******************* \n");
  print_system_info(clock);
  printf("\n");

  return 1;
}
