/*
Project: CPU Scheduler
Author: Matthew Skovorodin
Description: This program implements a CPU scheduling simulation with four scheduling algorithms: First Come First Serve (FCFS), Shortest Job First (SJF), Priority (PR), and Round-Robin (RR). It uses a minimum of three threads to measure performance metrics like throughput, turnaround time, and waiting time. The program reads process information from an input file and simulates CPU and I/O bursts accordingly.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

struct Stats {
    int *startTime;  //[pid][start_time]
    int *lastReady;
    int *totalWait;
    int *totalTime;
};

struct Process {
    int pid;
    int cpuArraySize;
    int ioArraySize;
    int totalBurstTime;
    int priority;
    int cpuPosition;  // cpu burst element
    int ioPosition;   // io burst element
    struct Process *prev;
    struct Process *next;
    int *cpuBurst;
    int *ioBurst;
};

// Global variables
pthread_mutex_t readyQLock;
pthread_mutex_t ioQLock;
pthread_mutex_t statsLock;
struct Process *readyQ = NULL;
struct Process *ioQ = NULL;
struct Stats *timeStats = NULL;
char filename[256];
int fileFinish = 0;  // 1 when file parsing has completed
int algorithm;       // 1 = FCFS, SJF = 2, PR = 3, RR = 4
int quantum;
int processCount = 0;
int preCount = 0;
int fileBusy = 0;

void printUsage() {
    printf("Usage: ./exec -alg [FCFS|SJF|PR|RR] [-quantum [int(ms)]] -input [filename]\n");
    printf("FCFS: First Come First Serve\n");
    printf("SJF: Shortest Job Firsts\n");
    printf("PR: Priority\n");
    printf("RR: Round Robin\n");
}

void printOutput() {
    float totalTime = 0;
    float totalWaitTime = 0;
    for (int i = 0; i < processCount; i++) {
        totalTime += timeStats->totalTime[i];
        totalWaitTime += timeStats->totalWait[i];
    }

    printf("Input File Name                  :%s\n", filename);
    printf("CPU Scheduling alg.              :");
    if (algorithm == 1) {
        printf("FIFO\n");
    } else if (algorithm == 2) {
        printf("SJF\n");
    } else if (algorithm == 3) {
        printf("PR\n");
    } else {
        printf("RR, %d\n", quantum);
    }
    printf("Total Run time                   :%f ms\n", totalTime);
    printf("Throughput                       :%f proc/ms\n", processCount / totalTime);
    printf("Avg. Turnaround time             :%f ms\n", (totalTime / processCount));
    printf("Avg. Waiting time in ready queue :%f ms\n", totalWaitTime / processCount);
}


void insertReady(struct process *process) {
    //printf("transferring process[%d] from IO queue to ready queue\n",process->pid);
    if(ready_q == NULL) { // if ready_q is empty
            ready_q = process;
            ready_q->next = NULL;
            ready_q->prev = NULL;
    }
    else { // if the ready_q is not empty add process to the end of the list
        struct process *current_proc = ready_q;
        while (current_proc->next != NULL) {
            current_proc = current_proc->next;
        }
        struct process *new_proc = process;
        new_proc->next = NULL;
        new_proc->prev = current_proc;
        current_proc->next = new_proc;
    }
}

void insertIO(struct process *process) {
    //printf("transferring process[%d] from ready queue to IO queue\n",process->pid);
    if(io_q == NULL) {
        io_q = process;
        io_q->next = NULL;
        io_q->prev = NULL;
    }
    else { // if the io_q is not empty add process to the end of the list
        struct process *current_proc = io_q;
        while (current_proc->next != NULL) {
            current_proc = current_proc->next;
        }
        struct process *new_proc = process;
        new_proc->next = NULL;
        new_proc->prev = current_proc;
        current_proc->next = new_proc;
    }
}

void delete(struct process *process, int flag) {
    struct process *current_node = process;
    struct process *prev_node = current_node->prev;
    struct process *next_node = current_node->next;

    if(flag == 2) {
        free(process);
        if(process_count == 1) {
            ready_q = NULL;
        }
        return;
        }
    if (prev_node == NULL && next_node == NULL) {
        // If only element left in list
        if(flag == 1) {ready_q = NULL;}
        else if(flag == 0) {io_q = NULL;}
    } else if (next_node == NULL) {
        // If last element in list
        prev_node->next = NULL;
        if(flag == 1) {ready_q = prev_node;}
        else if (flag == 0) {io_q = prev_node;}
    } else if (prev_node == NULL) {
        // If first element in list
        next_node->prev = NULL;
        if(flag == 1) {ready_q = next_node;}
        else if(flag == 0) {io_q = next_node;}
    } else {
        // If element is in the middle of the list
        prev_node->next = next_node;
        next_node->prev = prev_node;
        while(prev_node->prev != NULL) {
            prev_node = prev_node->prev;
        }
        if(flag == 1) {ready_q = prev_node;}
        else if(flag == 0) {io_q = prev_node;}
    }
}

void *fileHandler() {
    printf("Starting file thread...\n");
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("File open failed\n");
        exit(1);
    }

    char line[256];
    struct process *new_proc = NULL;
    struct timeval start_time;
    int burst_quantity;
    int pid = 0;
    int timer = 0;
    int counter = 0;
    int total_burst = 0;
    int priority;
    int data;

    time_stats = malloc(sizeof(struct stats) + 4 * sizeof(int));

    while (fgets(line, 256, fp) != NULL) {
        char *token = strtok(line, " ");
        if (strcmp(token, "proc") == 0) {
            
            gettimeofday(&start_time, NULL); // Get start time
            while (counter < 2) { // grabbing priority and burst quantity info
                token = strtok(NULL, " ");
                if (token == NULL) {
                    break;
                }
                if(counter == 0) { // handles priority and burst count
                    priority = atoi(token);
                }
                if(counter == 1) {
                    burst_quantity = atoi(token);
                }
                counter++;
                continue;
            }

            while(pthread_mutex_lock(&ready_q_lock) != 0) {}
            if (ready_q == NULL) {
                ready_q = malloc(sizeof(struct process) + (burst_quantity + 1) * sizeof(int));
                ready_q->next = NULL;
                ready_q->prev = NULL;
                ready_q->pid = pid;
                ready_q->priority = priority;
                ready_q->cpu_position = 0;
                ready_q->io_position = 0;
                new_proc = ready_q;
            } else {
                struct process *current_proc = new_proc;
                while (current_proc->next != NULL) {
                    current_proc = current_proc->next;
                }
                new_proc = malloc(sizeof(struct process) + (burst_quantity + 1) * sizeof(int));
                new_proc->next = NULL;
                new_proc->prev = current_proc;
                new_proc->pid = pid;
                new_proc->cpu_position = 0;
                new_proc->io_position = 0;
                new_proc->priority = priority;
                current_proc->next = new_proc;
            }

            new_proc->cpu_array_size = (burst_quantity + 1) / 2;
            new_proc->io_array_size = burst_quantity / 2;
            new_proc->cpu_burst = malloc(new_proc->cpu_array_size * sizeof(int));
            new_proc->io_burst = malloc(new_proc->io_array_size * sizeof(int));

            int i = 0;
            int n = 0;
            int iter_num = 0;
            while (token != NULL) {
                token = strtok(NULL, " ");
                if (token == NULL) {
                    break;
                }
                int burst = atoi(token);
                timer++;
                if (timer == 1) {
                    time_stats->start_time = realloc(time_stats->start_time, (pre_count + 1) * sizeof(int));
                    time_stats->last_ready = realloc(time_stats->last_ready, (pre_count + 1) * sizeof(int));
                    time_stats->total_wait = realloc(time_stats->total_wait, (pre_count + 1) * sizeof(int));
                    time_stats->total_time = realloc(time_stats->total_time, (pre_count + 1) * sizeof(int));
                    time_stats->start_time[pre_count] = (start_time.tv_usec);
                    time_stats->last_ready[pre_count] = time_stats->start_time[pre_count];
                    time_stats->total_wait[pre_count] = 0;
                    time_stats->total_time[pre_count] = 0;
                    pre_count += 1;
                }
                if (iter_num % 2 == 0) {
                    new_proc->cpu_burst[i] = burst;
                    new_proc->total_burst_time += burst;
                    i++;
                } else {
                    new_proc->io_burst[n] = burst;
                    new_proc->total_burst_time += burst;
                    n++;
                }
                iter_num++;
            }
            pthread_mutex_unlock(&ready_q_lock);
            total_burst = 0;
            file_busy = 1;
            pid++;
            timer = 0;
            counter = 0;
        } else if (strcmp(token, "sleep") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                printf("Error: Missing duration for sleep command\n");
            }
            data = atoi(token);
            fflush(stdout);
            printf("Sleeping for %d ms\n", data);
            sleep(data / 1000);
        } else if (strcmp(token, "stop") == 0) {
            process_count = pid;
            file_finish = 1;
            break;
        } else {
            printf("Error: Unknown keyword\n");
            printf("%s",token);
        }
    }
    fclose(fp);
    printf("Exiting File Processing Thread\n");
    pthread_mutex_unlock(&ready_q_lock);
    pthread_exit(NULL);
} 

int check_processes() {
    if(time_stats != NULL && (pre_count == process_count)) {
        for(int i = 0; i < process_count; i++) {
            if(time_stats->total_time[i] == 0) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}
    
void *ioScheduler() {
    printf("Starting IO thread...\n");
    int burst;
    struct process *process;
    struct timeval start_time;
    while(1) {
        if (file_busy == 0) {
            continue;
        }
        if(file_finish && check_processes()) {
            break;
        }
        while(pthread_mutex_lock(&io_q_lock) != 0){}//wait for queue access
        if(io_q != NULL) {
            burst = io_q->cpu_burst[io_q->io_position];
            io_q->total_burst_time -= burst;
            process = io_q;
            process->io_position += 1;
            delete(io_q, 0);
            pthread_mutex_unlock(&io_q_lock);
            // print out a layout of bursts
/*             printf("process[%d] IO: ",process->pid);
            for (int i = 0; i < process->io_array_size; i++) {
                if (i == process->io_position-1) {
                    printf("[%d] ", process->io_burst[i]); // Print bracket around the indexed element
                } else {
                    printf("%d ", process->io_burst[i]); // Print the element without brackets
                }
            }
            printf("\n"); */
            sleep(burst/1000);

            while(pthread_mutex_lock(&stats_lock) != 0){}//wait for queue access
            gettimeofday(&start_time, NULL); // Get end time
            time_stats->last_ready[process->pid] = (start_time.tv_usec);
            pthread_mutex_unlock(&stats_lock);
            
            while(pthread_mutex_lock(&ready_q_lock) != 0){}//wait for list access
            insertReady(process); // take ready queue to IO queue
            pthread_mutex_unlock(&ready_q_lock);
        }
        pthread_mutex_unlock(&io_q_lock);
    }
    pthread_mutex_unlock(&io_q_lock);
    printf("Exiting IO Scheduling Thread\n");
    pthread_exit(NULL);
}

void *cpuScheduler() {
    printf("Starting CPU Thread...\n");
    int burst;
    int burst_compare;
    int burst_complete = 1;
    int priority_compare;
    struct process *process;
    struct process *temp;
    struct timeval end_time, wait_time;
    while(1) {
        if (file_busy == 0) {
            continue;
        }
        //check if done with thread
        if(file_finish && check_processes()) {
            break;
        }
        while(pthread_mutex_lock(&ready_q_lock) != 0){}//wait for ready queue access
        if(ready_q != NULL) {
            if(algorithm == 4) { // RR
                burst_complete = 0;
                process = ready_q;
                delete(ready_q, 1);
                pthread_mutex_unlock(&ready_q_lock);
                burst = process->cpu_burst[process->cpu_position];
                if(burst <= quantum) { // we do things as normal
                    burst_complete = 1;
                    process->cpu_position += 1;
                }
                else { // we don't add to IO queue
                    burst_complete = 0;
                    burst = quantum;
                    process->cpu_burst[process->cpu_position] -= quantum;
                }
                
            }
            else if(algorithm == 3) { // PR
                priority_compare = 0;
                temp = ready_q;
                process = ready_q;
                while(temp != NULL) {
                    if(temp->priority > priority_compare) {
                        process = temp;
                    }
                    temp = temp->next;
                    if(temp == NULL) {break;}
                    priority_compare = temp->priority; 
                }
                burst = process->cpu_burst[process->cpu_position];
                ready_q = process;
                ready_q->total_burst_time -= burst;
                process->cpu_position += 1;
                delete(ready_q, 1);
            }
            else if(algorithm == 2) { // SJF
                burst_compare = 9999999;
                temp = ready_q;
                while(temp != NULL) {
                    if(temp->total_burst_time < burst_compare) {
                        process = temp;
                    }
                    temp = temp->next;
                    if(temp == NULL) {break;}
                    burst_compare = temp->total_burst_time; 
                }
                burst = process->cpu_burst[process->cpu_position];
                ready_q = process;
                ready_q->total_burst_time -= burst;
                process->cpu_position += 1;
                delete(ready_q, 1);
            }
            else { // FCFS
                burst = ready_q->cpu_burst[ready_q->cpu_position];
                ready_q->total_burst_time -= burst;
                process = ready_q;
                process->cpu_position += 1;
                delete(ready_q, 1);
            }
            pthread_mutex_unlock(&ready_q_lock);
            
            // print out a layout of bursts
/*             printf("process[%d] CPU: ",process->pid);
            for (int i = 0; i < process->cpu_array_size; i++) {
                if (i == process->cpu_position-1) {
                    printf("[%d] ", process->cpu_burst[i]); // Print bracket around the indexed element
                } else {
                    printf("%d ", process->cpu_burst[i]); // Print the element without brackets
                }
            }
            printf("\n"); */

            while(pthread_mutex_lock(&stats_lock) != 0){}//wait for queue access
            gettimeofday(&wait_time, NULL); // Get end time
            time_stats->total_wait[process->pid] += ((wait_time.tv_usec) - time_stats->last_ready[process->pid])/1.5;
            pthread_mutex_unlock(&stats_lock);
            sleep(burst/1000);

            if((process->cpu_position) == process->cpu_array_size && burst_complete) { // reached end of the burst times
                while(pthread_mutex_lock(&stats_lock) != 0){}//wait for queue access
                gettimeofday(&end_time, NULL); // Get end time
                time_stats->total_time[process->pid] = ((end_time.tv_usec) - time_stats->start_time[process->pid])/1.5;
                printf("time stats [%d] has total time: %d\n",process->pid,time_stats->total_time[process->pid]);
                pthread_mutex_unlock(&stats_lock);
                delete(process, 2);
                continue;
            }

            if(burst_complete == 0) {
                while(pthread_mutex_lock(&ready_q_lock) != 0){}//wait for list access
                insertReady(process);
                pthread_mutex_unlock(&ready_q_lock);
            }
            else if((process->io_position) != process->io_array_size) {
                while(pthread_mutex_lock(&io_q_lock) != 0){}//wait for list access
                insertIO(process); // move ready queue process onto IO queue
                pthread_mutex_unlock(&io_q_lock);
            }
        }
        pthread_mutex_unlock(&ready_q_lock);
    }
    pthread_mutex_unlock(&ready_q_lock);
    printf("Exiting CPU Scheduling Thread\n");
    pthread_exit(NULL);
}

void threadHandler() {
    pthread_t file_handler_thread;
    pthread_t cpu_scheduler_thread;
    pthread_t io_scheduler_thread;

    //init mutexes
  	pthread_mutex_init(&ready_q_lock, NULL);
  	pthread_mutex_init(&io_q_lock, NULL);
  	pthread_mutex_init(&stats_lock, NULL);

    if((pthread_create(&file_handler_thread, NULL, fileHandler, NULL)) != 0) {
        printf("Failed to create thread: file_handler_thread\n");
        exit(1);
    }
    if((pthread_create(&cpu_scheduler_thread, NULL, cpuScheduler, NULL)) != 0) {
        printf("Failed to create thread: cpu_scheduler_thread\n");
        exit(1);
    }
    if((pthread_create(&io_scheduler_thread, NULL, ioScheduler, NULL)) != 0) {
        printf("Failed to create thread: io_scheduler_thread\n");
        exit(1);
    }

    //Join threads
    if(pthread_join(file_handler_thread, NULL) != 0) {
        printf("Failed to join thread: file_handler_thread\n");
        exit(1);
    }
    if(pthread_join(cpu_scheduler_thread, NULL) != 0) {
        printf("Failed to join thread: cpu_scheduler_thread\n");
        exit(1);
    }
    if(pthread_join(io_scheduler_thread, NULL) != 0) {
        printf("Failed to join thread: io_scheduler_thread\n");
        exit(1);
    }

    pthread_mutex_destroy(&ready_q_lock);
    pthread_mutex_destroy(&io_q_lock);
    pthread_mutex_destroy(&stats_lock);
}


int main(int argc, char *argv[]) {
    int fileLoc = 4; //file loc. comes at 4th index if quantum isn't being used

    if(!(argc == 5 || argc == 7)) {
        printUsage();
        return 1;
    }

    if(strcmp(argv[2], "RR") == 0) { //if reliant robin is selected
        quantum = atoi(argv[4]);
        if(quantum < 1) {
            printf("quantum time must be an integer greater than zero\n");
            return 1;
        }
        fileLoc = 6;
        algorithm = 4;
    }
    else if(strcmp(argv[2],  "FCFS") == 0) {
        algorithm = 1;
    }
    else if(strcmp(argv[2],  "SJF") == 0) {
        algorithm = 2;
    }
    else if(strcmp(argv[2],  "PR") == 0) {
        algorithm = 3;
    }
    else {
        printUsage();
        return 1;
    }
    
    strcpy(filename, argv[fileLoc]);
    threadHandler();
    printOutput();
    return 0;
}