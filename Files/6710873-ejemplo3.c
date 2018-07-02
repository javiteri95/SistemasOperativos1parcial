#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int generate_random_id(){
	srand(time(NULL));   // should only be called once
	int r = rand();      // returns a pseudo-random integer between 0 and RAND_MAX
	return r;
}


int main(){
    int variable = generate_random_id();
    printf("variable: %d\n", variable);
    return 0;
}
