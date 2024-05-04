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
    int is_active; // Czy bramka jest aktywna (1) czy nie (0)
} Gate;

typedef struct {
    int seed;
    int id;
} Args;

Gate gates[GATE_COUNT];

pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t global_cond = PTHREAD_COND_INITIALIZER;
int in_personal_check = 0; // Czy którykolwiek pasażer jest obecnie na kontroli osobistej
int multi_problem_flag = 0;

#ifdef DEBUG
pthread_mutex_t prob_cnt_lock = PTHREAD_MUTEX_INITIALIZER;
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
    //DEBUG_PRINT("Pasażer %d, bramka %d: [L][S] Lock bramki.\n", id, chosen_gate_idx);
    while (!chosen_gate->is_active) {
        DEBUG_PRINT("Pasażer %d, bramka %d: [W] czekanie na gate\n", id, chosen_gate_idx);
        pthread_cond_wait(&chosen_gate->condition, &chosen_gate->lock);
    }

    pthread_mutex_lock(&global_lock);
    //DEBUG_PRINT("Pasażer %d, bramka %d: [L][S] Lock global.\n", id, chosen_gate_idx);
    while(in_personal_check > 0 && multi_problem_flag) {
        DEBUG_PRINT("Pasażer %d, bramka %d: [W] czekanie na global\n", id, chosen_gate_idx);
        pthread_cond_wait(&global_cond, &global_lock);
    }

    DEBUG_PRINT("Pasażer %d, bramka %d: Podejście.\n", id, chosen_gate_idx);

    if (IS_PROBLEM) {
        chosen_gate->is_active = 0;

        #ifdef DEBUG
        pthread_mutex_lock(&prob_cnt_lock);
        problem_count++;
        int this_prob = problem_count;
        pthread_mutex_unlock(&prob_cnt_lock);
        #endif

        DEBUG_PRINT("Pasażer %d, bramka %d: [P%d][START] Problem przy prześwietlaniu.\n", id, chosen_gate_idx, this_prob);
        if (in_personal_check > 0 && !multi_problem_flag) {
            multi_problem_flag = 1;
            DEBUG_PRINT("Pasażer %d, bramka %d: [P%d->K>1][S] Już ktoś na kontroli. Wstrz. bram.\n", id, chosen_gate_idx, this_prob);

            for (int i = 0; i < GATE_COUNT; i++) {
                gates[i].is_active = 0;
                DEBUG_PRINT("Pasażer %d, bramka %d: Wstrz. bram. %d\n", id, chosen_gate_idx, i);
            }
            DEBUG_PRINT("Pasażer %d, bramka %d: [P%d->K>1][S] Wstrz. wszyst. bram. zakończone.\n", id, chosen_gate_idx, this_prob);
        }
        in_personal_check++;

        pthread_cond_broadcast(&global_cond);
        //DEBUG_PRINT("Pasażer %d, bramka %d: [L][E] Unlock global.\n", id, chosen_gate_idx);
        pthread_mutex_unlock(&global_lock);

        // Kontrola osobista
        DEBUG_PRINT("Pasażer %d, bramka %d: [P%d->K][S] Początek kontroli.\n", id, chosen_gate_idx, this_prob);
        pthread_t check;
        pthread_create(&check, NULL, personal_check, &seed);
        pthread_join(check, NULL);
        DEBUG_PRINT("Pasażer %d, bramka %d: [P%d->K][E] Koniec kontroli.\n", id, chosen_gate_idx, this_prob);

        pthread_mutex_lock(&global_lock);
        //DEBUG_PRINT("Pasażer %d, bramka %d: [L][S] Lock global.\n", id, chosen_gate_idx);
        in_personal_check--;

        if (IS_PROBLEM_2ND) {
            DEBUG_PRINT("Pasażer %d, bramka %d: Kontrola osobista wykryła problem. Pasażer nie wpuszczony na pokład.\n", id, chosen_gate_idx);
        } else {
            DEBUG_PRINT("Pasażer %d, bramka %d: Kontrola osobista nie wykryła problemu. Pasażer wpuszczony na pokład.\n", id, chosen_gate_idx);
        }
        if (in_personal_check == 0 && multi_problem_flag) {
            DEBUG_PRINT("Pasażer %d, bramka %d: [P%d->K>1][E] Nie ma więcej kontroli. Aktywacja bramek.\n", id, chosen_gate_idx, this_prob);
            for (int i = 0; i < GATE_COUNT; i++) {
                if(&gates[i] != chosen_gate) {
                    gates[i].is_active = 1;
                    DEBUG_PRINT("Pasażer %d, bramka %d: Aktyw. bram. %d\n", id, chosen_gate_idx, i);
                    pthread_cond_signal(&gates[i].condition);
                }
            }
            DEBUG_PRINT("Pasażer %d, bramka %d: Aktyw. bram. %d\n", id, chosen_gate_idx, chosen_gate_idx);
            DEBUG_PRINT("Pasażer %d, bramka %d: [P%d->K>1][E] Aktyw. wszyst. bram. zakończone.\n", id, chosen_gate_idx, this_prob);
            multi_problem_flag = 0;
        }
        chosen_gate->is_active = 1;
        DEBUG_PRINT("Pasażer %d, bramka %d: [P%d][END] Koniec problemów.\n", id, chosen_gate_idx, this_prob);
    } else {
        DEBUG_PRINT("Pasażer %d, bramka %d: Przejście bez problemów.\n", id, chosen_gate_idx);
    }

    pthread_cond_signal(&chosen_gate->condition);
    pthread_cond_broadcast(&global_cond);
    //DEBUG_PRINT("Pasażer %d, bramka %d: [L][E] Unlock bramki.\n", id, chosen_gate_idx);
    pthread_mutex_unlock(&chosen_gate->lock);
    //DEBUG_PRINT("Pasażer %d, bramka %d: [L][E] Unlock global.\n", id, chosen_gate_idx);
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
    pthread_mutex_destroy(&prob_cnt_lock);
    #endif
    return 0;
}

