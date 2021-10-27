#include <pthread.h>
#include <string>
#include <random>

typedef struct
{
   int thread_id;
   long long int start;
   long long int end;
   long long int *number_in_circle;
} Arg;

std::random_device rd;
std::default_random_engine eng(rd());
std::uniform_real_distribution<double> distr(-1, 1);

pthread_mutex_t mutexsum;
long long int *number_in_circle;

void *count_pi(void *arg){
    Arg *args = (Arg*)arg;
    const int thread_id = args->thread_id;
    const long long int start = args->start;
    const long long int end = args->end;
    long long int *number_in_circle = args->number_in_circle;

    double x;
    double y;
    double distance_squared;
    long long int local_sum = 0;
    long long int toss = start;
    for (; toss < end; ++toss) {
        x = distr(eng);
        y = distr(eng);
        distance_squared = x * x + y * y;
        if ( distance_squared <= 1)
            ++local_sum;
    }

    pthread_mutex_lock(&mutexsum);
    *number_in_circle += local_sum;
    pthread_mutex_unlock(&mutexsum);

    pthread_exit((void *)0);
}


int main(int argc, char *argv[]){
    const int num_threads = std::stoi(argv[1]);
    const long long int number_of_tosses = std::stoll(argv[2]);
    
    pthread_mutex_init(&mutexsum, NULL);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    number_in_circle = new long long int;
    pthread_t threads[num_threads];
    
    long long int part = number_of_tosses / num_threads;
    Arg arg[num_threads];
    for (int i = 0; i < num_threads; i++)
    {
        arg[i].thread_id = i;
        arg[i].start = part * i;
        arg[i].end = part * (i + 1);
        arg[i].number_in_circle = number_in_circle;

        pthread_create(&threads[i], &attr, count_pi, (void *)&arg[i]);
    }

    pthread_attr_destroy(&attr);
    
    void *status;
    for (int i = 0; i < num_threads; i++){
        pthread_join(threads[i], &status);
    }

    double pi_estimate = 0;  
    pi_estimate = 4 * *number_in_circle /(( double ) number_of_tosses);
    printf("%lf\n", pi_estimate);

    pthread_mutex_destroy(&mutexsum);
    pthread_exit(NULL);
}