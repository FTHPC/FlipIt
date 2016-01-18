#include "stdio.h"
#include <stdlib.h>
#include "/home/aperson40/research/FlipIt//src/corrupt/corrupt.h"

int work() {
    int a = 3;
    int x = rand()%100;

    return x*x + x + a;
}

void display() {
    printf("Hello, World! %d\n", work());
}

int main(int argc, char** argv) {
    FLIPIT_Init(0, argc, argv, 0);
    display();
    FLIPIT_Finalize(NULL);

	return 0;
}
