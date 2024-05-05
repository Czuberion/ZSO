#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define PASSENGER_COUNT 20
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
    int is_active;
} Gate;

typedef struct {
    int seed;
    int id;
} Args;

Gate gates[GATE_COUNT];

pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t global_cond = PTHREAD_COND_INITIALIZER;
int in_personal_check_count = 0;
int multi_problem_flag = 0; // indicates that multiple personal checks 
                            // have been triggered

#ifdef DEBUG
pthread_mutex_t problem_count_lock = PTHREAD_MUTEX_INITIALIZER;
int problem_count = 0;
#endif

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

    pthread_mutex_lock(&chosen_gate->lock);
    //DEBUG_PRINT("Passenger %d, Gate %d: [L][S] Lock gate.\n", id, chosen_gate_idx);
    while (!chosen_gate->is_active) {
        DEBUG_PRINT("Passenger %d, Gate %d: [W] Waiting for gate\n", id, chosen_gate_idx);
        pthread_cond_wait(&chosen_gate->condition, &chosen_gate->lock);
    }

    pthread_mutex_lock(&global_lock);
    //DEBUG_PRINT("Passenger %d, Gate %d: [L][S] Lock global.\n", id, chosen_gate_idx);
    while(in_personal_check_count > 0 && multi_problem_flag) {
        DEBUG_PRINT("Passenger %d, Gate %d: [W] Waiting for global\n", id, chosen_gate_idx);
        pthread_cond_wait(&global_cond, &global_lock);
    }

    DEBUG_PRINT("Passenger %d, Gate %d: Approach gate.\n", id, chosen_gate_idx);

    if (IS_PROBLEM) {
        chosen_gate->is_active = 0;

        #ifdef DEBUG
        pthread_mutex_lock(&problem_count_lock);
        problem_count++;
        int this_prob = problem_count;
        pthread_mutex_unlock(&problem_count_lock);
        #endif

        DEBUG_PRINT("Passenger %d, Gate %d: [P%d][START] Problem detected.\n", id, chosen_gate_idx, this_prob);
        if (in_personal_check_count > 0 && !multi_problem_flag) {
            multi_problem_flag = 1;
            DEBUG_PRINT("Passenger %d, Gate %d: [P%d->C>1][S] Someone is already being checked. Halting gates.\n", id, chosen_gate_idx, this_prob);

            for (int i = 0; i < GATE_COUNT; i++) {
                gates[i].is_active = 0;
                DEBUG_PRINT("Passenger %d, Gate %d: Halting gate #%d\n", id, chosen_gate_idx, i);
            }
            DEBUG_PRINT("Passenger %d, Gate %d: [P%d->C>1][S] All gates halted.\n", id, chosen_gate_idx, this_prob);
        }
        in_personal_check_count++;

        pthread_cond_broadcast(&global_cond);
        //DEBUG_PRINT("Passenger %d, Gate %d: [L][E] Unlock global.\n", id, chosen_gate_idx);
        pthread_mutex_unlock(&global_lock);

        // Kontrola osobista
        DEBUG_PRINT("Passenger %d, Gate %d: [P%d->C][S] Personal check start.\n", id, chosen_gate_idx, this_prob);
        pthread_t check;
        pthread_create(&check, NULL, personal_check, &seed);
        pthread_join(check, NULL);
        DEBUG_PRINT("Passenger %d, Gate %d: [P%d->C][E] Personal check end.\n", id, chosen_gate_idx, this_prob);

        pthread_mutex_lock(&global_lock);
        //DEBUG_PRINT("Passenger %d, Gate %d: [L][S] Lock global.\n", id, chosen_gate_idx);
        in_personal_check_count--;

        if (IS_PROBLEM_2ND) {
            DEBUG_PRINT("Passenger %d, Gate %d: Personal check has identified a problem. Passenger detained.\n", id, chosen_gate_idx);
        } else {
            DEBUG_PRINT("Passenger %d, Gate %d: Personal check hasn't identified any problem. Passenger boards the plane.\n", id, chosen_gate_idx);
        }
        if (in_personal_check_count == 0 && multi_problem_flag) {
            DEBUG_PRINT("Passenger %d, Gate %d: [P%d->C>1][E] No more personal checks. Activating gates.\n", id, chosen_gate_idx, this_prob);
            for (int i = 0; i < GATE_COUNT; i++) {
                if(&gates[i] != chosen_gate) {
                    gates[i].is_active = 1;
                    DEBUG_PRINT("Passenger %d, Gate %d: Activating gate #%d\n", id, chosen_gate_idx, i);
                    pthread_cond_signal(&gates[i].condition);
                }
            }
            DEBUG_PRINT("Passenger %d, Gate %d: Activating gate #%d\n", id, chosen_gate_idx, chosen_gate_idx);
            DEBUG_PRINT("Passenger %d, Gate %d: [P%d->C>1][E] All gates activated.\n", id, chosen_gate_idx, this_prob);
            multi_problem_flag = 0;
        }
        chosen_gate->is_active = 1;
        DEBUG_PRINT("Passenger %d, Gate %d: [P%d][END] Problem ended.\n", id, chosen_gate_idx, this_prob);
    } else {
        DEBUG_PRINT("Passenger %d, Gate %d: No problem detected. Passenger boards the plane.\n", id, chosen_gate_idx);
    }

    pthread_cond_signal(&chosen_gate->condition);
    pthread_cond_broadcast(&global_cond);
    //DEBUG_PRINT("Passenger %d, Gate %d: [L][E] Unlock gate.\n", id, chosen_gate_idx);
    pthread_mutex_unlock(&chosen_gate->lock);
    //DEBUG_PRINT("Passenger %d, Gate %d: [L][E] Unlock global.\n", id, chosen_gate_idx);
    pthread_mutex_unlock(&global_lock);

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

    for (int i = 0; i < PASSENGER_COUNT; i++) {
        args[i].seed = rand();
        args[i].id = i;
        pthread_create(&passengers[i], NULL, passenger_function, &args[i]);
    }

    for (int i = 0; i < PASSENGER_COUNT; i++) {
        pthread_join(passengers[i], NULL);
    }

    cleanup();
    pthread_mutex_destroy(&global_lock);
    pthread_cond_destroy(&global_cond);
    #ifdef DEBUG
    pthread_mutex_destroy(&problem_count_lock);
    #endif
    return 0;
}

