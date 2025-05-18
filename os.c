#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_TASKS 50

typedef struct {
    int pid;
    char name[50];
    int ramUsed;
    int hddUsed;
    pthread_t threadId;
    int isThread;
} Task;

Task readyQueue[MAX_TASKS];
int front = 0, rear = -1;

int availableRAM = 2048;
int availableHDD = 256000;
int totalCores = 8;
pthread_mutex_t resourceLock = PTHREAD_MUTEX_INITIALIZER;
sem_t cpuSemaphore;

void* roundRobinScheduler(void* arg) {
    while (1) {
        sem_wait(&cpuSemaphore);
        pthread_mutex_lock(&resourceLock);
        if (front <= rear && readyQueue[front].isThread) {
            Task task = readyQueue[front];
            printf("[Thread] %s\n", task.name);
            for (int i = front; i < rear; i++) {
                readyQueue[i] = readyQueue[i + 1];
            }
            readyQueue[rear] = task;
        }
        pthread_mutex_unlock(&resourceLock);
        sem_post(&cpuSemaphore);
        sleep(2);
    }
    return NULL;
}

void initScheduler() {
    sem_init(&cpuSemaphore, 0, 1);
    pthread_t schedThread;
    pthread_create(&schedThread, NULL, roundRobinScheduler, NULL);
    pthread_detach(schedThread);
}

void enqueueTask(Task task) {
    if (rear < MAX_TASKS - 1) {
        readyQueue[++rear] = task;
    }
}

void showRunningTasks() {
    if (rear < front) {
        printf("No tasks.\n");
        return;
    }
    for (int i = front; i <= rear; i++) {
        if (readyQueue[i].isThread)
            printf("TID: %lu %s\n", readyQueue[i].threadId, readyQueue[i].name);
        else
            printf("PID: %d %s\n", readyQueue[i].pid, readyQueue[i].name);
    }
}

int allocateMemory(int ram, int hdd) {
    if (ram <= availableRAM && hdd <= availableHDD) {
        availableRAM -= ram;
        availableHDD -= hdd;
        return 1;
    }
    return 0;
}

void freeMemory(int ram, int hdd) {
    availableRAM += ram;
    availableHDD += hdd;
}

void terminateTask() {
    int pid;
    printf("PID to terminate: ");
    scanf("%d", &pid);
    for (int i = front; i <= rear; i++) {
        if (!readyQueue[i].isThread && readyQueue[i].pid == pid) {
            kill(pid, SIGKILL);
            freeMemory(readyQueue[i].ramUsed, readyQueue[i].hddUsed);
            for (int j = i; j < rear; j++) readyQueue[j] = readyQueue[j + 1];
            rear--;
            return;
        }
    }
}

void shutdownAllTasks() {
    for (int i = front; i <= rear; i++) {
        if (!readyQueue[i].isThread) kill(readyQueue[i].pid, SIGKILL);
        freeMemory(readyQueue[i].ramUsed, readyQueue[i].hddUsed);
    }
}

void launchTask(int isKernelMode) {
    char task[50];
    int ram = 100, hdd = 50;
    printf("Task name: ");
    scanf("%s", task);

    if (!allocateMemory(ram, hdd)) {
        printf("Not enough RAM or HDD.\n");
        return;
    }

    char path[100], cmd[200];
    sprintf(path, "./tasks/%s", task);
    sprintf(cmd, "gnome-terminal -- bash -c '%s; read'", path);
    int result = system(cmd);

    if (result == 0) {
        Task newTask;
        newTask.pid = rand() % 10000 + 1000;
        strcpy(newTask.name, task);
        newTask.ramUsed = ram;
        newTask.hddUsed = hdd;
        newTask.isThread = 0;
        enqueueTask(newTask);
    } else {
        freeMemory(ram, hdd);
    }
}

void displayBootScreen() {
    printf("\nAMA OS Booting...\n");
    sleep(1);
    printf("Modules...\n");
    sleep(1);
    printf("Ready.\n\n");
}

void getSystemResources() {
    printf("RAM: ");
    scanf("%d", &availableRAM);
    printf("HDD: ");
    scanf("%d", &availableHDD);
    printf("Cores: ");
    scanf("%d", &totalCores);
}

void showMenu(int mode) {
    printf("\n--- MENU (%s) ---\n", mode == 0 ? "User" : "Kernel");
    printf("1. Launch Task\n2. Show Tasks\n3. Terminate (Kernel)\n4. Switch Mode\n5. Exit\n> ");
}

int main() {
    displayBootScreen();
    initScheduler();
    int choice, mode = 0;
    while (1) {
        showMenu(mode);
        scanf("%d", &choice);
        switch (choice) {
            case 1: launchTask(mode); break;
            case 2: showRunningTasks(); break;
            case 3: if (mode == 1) terminateTask(); break;
            case 4: mode = 1 - mode; break;
            case 5: shutdownAllTasks(); exit(0);
        }
    }
    return 0;
}

