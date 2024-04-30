#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define PASSENGER_COUNT 10
#define GATE_COUNT 3
#define CHOSEN_GATE_IDX id % GATE_COUNT

//                100'000'000
#define WORK_AMOUNT 100000000

#define INV_PROBLEM_PROB_1ST 5
#define INV_PROBLEM_PROB_2ND 2
#define IS_PROBLEM seed % INV_PROBLEM_PROB_1ST == 0
#define IS_PROBLEM_2ND seed % INV_PROBLEM_PROB_2ND == 0

#ifdef DEBUG
#define DEBUG_PRINT(str, ...) printf(str, __VA_ARGS__)
#else
#define DEBUG_PRINT(str, ...)
#endif

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t condition;
    int is_active; // Czy bramka jest aktywna (1) czy nie (0)
} Gate;

typedef struct {
    int seed;
    int id;
} Args;

Gate gates[GATE_COUNT];
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
int in_personal_check = 0; // Czy którykolwiek pasażer jest obecnie na kontroli osobistej

void* personal_check(void* arg) {
    volatile double result = (double)(*(int*)arg);
    for (volatile int i = 0; i < WORK_AMOUNT; i++) {
        result = result * i / ((i % 10) + 1);
    }
    return NULL;
}

void* passenger_function(void* arg) {
    int id = ((Args*)arg)->id;
    int seed = ((Args*)arg)->seed;
    int chosen_gate_idx = CHOSEN_GATE_IDX;
    Gate* chosen_gate = &gates[chosen_gate_idx];

    // Podejście do bramki
    pthread_mutex_lock(&chosen_gate->lock);
    while (!chosen_gate->is_active) { // helgrind data race 1, 2
        pthread_cond_wait(&chosen_gate->condition, &chosen_gate->lock);
    }

    DEBUG_PRINT("Pasażer %d podchodzi do bramki %d.\n", id, chosen_gate_idx);
    
    // Losowe wykrycie problemu przy przejściu przez bramkę
    if (IS_PROBLEM) {
        DEBUG_PRINT("Pasażer %d: Wykryto problem przy bramce %d. Wymagana kontrola osobista.\n", id, chosen_gate_idx);
        
        pthread_mutex_lock(&global_lock);
        if (in_personal_check > 0) {
            DEBUG_PRINT("Pasażer %d: Inny pasażer na kontroli. Wstrzymanie bramek.\n", id);
            
            for (int i = 0; i < GATE_COUNT; i++) {
                gates[i].is_active = 0; // helgrind data race 2
            }
        }
        in_personal_check++;
        pthread_mutex_unlock(&global_lock);

        // Kontrola osobista
        DEBUG_PRINT("Pasażer %d: Początek kontroli na bramce %d.\n", id, chosen_gate_idx);
        pthread_t check;
        pthread_create(&check, NULL, personal_check, &seed);
        pthread_join(check, NULL);
        DEBUG_PRINT("Pasażer %d: Koniec kontroli na bramce %d.\n", id, chosen_gate_idx);
        
        pthread_mutex_lock(&global_lock);
        in_personal_check--;

    	if (IS_PROBLEM_2ND) {
       	    DEBUG_PRINT("Pasażer %d: Kontrola osobista wykryła problem. Pasażer nie wpuszczony na pokład.\n", id);
	} else {
	    DEBUG_PRINT("Pasażer %d: Kontrola osobista nie wykryła problemu. Pasażer wpuszczony na pokład.\n", id);
	}
        if (in_personal_check == 0) {
            DEBUG_PRINT("Pasażer %d: Nie ma więcej kontroli. Aktywacja bramek.\n", id);
            for (int i = 0; i < GATE_COUNT; i++) {
                gates[i].is_active = 1; // helgrind data race 1
                pthread_cond_broadcast(&gates[i].condition); // pthread_cond_{signal,broadcast}: dubious: associated lock is not held by any thread
            }
        }
        pthread_mutex_unlock(&global_lock);
    } else {
        DEBUG_PRINT("Pasażer %d przechodzi przez bramkę %d bez problemów.\n", id, chosen_gate_idx);
    }

    pthread_mutex_unlock(&chosen_gate->lock);

    return NULL;
}

void initialize() {
    for (int i = 0; i < GATE_COUNT; i++) {
        gates[i].is_active = 1;
        pthread_mutex_init(&gates[i].lock, NULL);
        pthread_cond_init(&gates[i].condition, NULL);
    }
}

void cleanup() {
    for (int i = 0; i < GATE_COUNT; i++) {
        pthread_mutex_destroy(&gates[i].lock);
        pthread_cond_destroy(&gates[i].condition);
    }
}

int main() {
    srand(time(NULL));

    initialize();

    pthread_t passengers[PASSENGER_COUNT];
    Args args[PASSENGER_COUNT];

    for (volatile int i = 0; i < PASSENGER_COUNT; i++) {
        args[i].seed = rand();
        args[i].id = i;
        pthread_create(&passengers[i], NULL, passenger_function, &args[i]);
    }

    for (int i = 0; i < PASSENGER_COUNT; i++) {
        pthread_join(passengers[i], NULL);
    }

    cleanup();

    return 0;
}

