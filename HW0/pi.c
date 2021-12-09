#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main(){
    long long int number_in_circle = 0;
    long long int number_of_tosses = 1000*1000;
    double pi_estimate = 0;
    long long int toss = 0;
    srand ( time ( NULL));
    for (toss = 0; toss < number_of_tosses; ++toss) {
        double x = (double)rand()/RAND_MAX;
        double y = (double)rand()/RAND_MAX;
        double distance_squared = x * x + y * y;
        if ( distance_squared <= 1)
            number_in_circle++;
    }
    pi_estimate = 4 * number_in_circle /(( double ) number_of_tosses);
    printf("%lf\n", pi_estimate);

    return 0;
}