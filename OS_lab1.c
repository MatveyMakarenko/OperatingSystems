#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
bool ready = false;

void* provider(void* arg) {
    int count = *(int*)arg;
    for (int i = 0; i < count; ++i) {
        sleep(1);
        pthread_mutex_lock(&lock);
        if (ready) {
            pthread_mutex_unlock(&lock);
            continue;
            }
        ready = true;
        printf("provided\n");
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}


void* consumer(void* arg) {
    int count = *(int*)arg;
    for (int i = 0; i < count; ++i) {
        pthread_mutex_lock(&lock);
        while (!ready) {
            pthread_cond_wait(&cond, &lock);
        }
        ready = false;
        printf("consumed\n");
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    const int N = 5;  
    pthread_t prov_thr, cons_thr;

    int count = N;

    pthread_create(&prov_thr, NULL, provider, &count);
    pthread_create(&cons_thr, NULL, consumer, &count);

    pthread_join(prov_thr, NULL);
    pthread_join(cons_thr, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    return 0;
}


// while (!ready) {
//             pthread_cond_wait(&cond, &lock);  
//         } в этот момент мьютекс залочен?

// Нет.  

// В момент выполнения тела цикла — то есть когда поток находится внутри `pthread_cond_wait(&cond, &lock)` — мьютекс *разблокирован***.

// ### Почему:
// Согласно слайду 4 и POSIX-спецификации:

// > `pthread_cond_wait(&cond, &lock)` — это **атомарная операция, которая:
// > 1. освобождает мьютекс `lock`,  
// > 2. переводит поток в ожидание на условной переменной cond.

// То есть:
// - до входа в pthread_cond_wait → мьютекс заблокирован (поток его захватил в pthread_mutex_lock),
// - в момент ожидания (когда поток спит) → мьютекс разблокирован, другие потоки могут его захватить,
// - при пробуждении (после signal) → pthread_cond_wait автоматически повторно захватывает мьютекс *до* возврата управления.

// ✅ Поэтому после выхода из pthread_cond_wait (и возврата в проверку while (!ready)) мьютекс снова заблокирован — и можно безопасно читать ready.

// Это и есть ключевой механизм: ожидание без блокировки доступа другим потокам.
// pthread_cond_signal() — пробуждает один ожидающий поток.
// pthread_cond_broadcast() — пробуждает все ожидающие потоки.