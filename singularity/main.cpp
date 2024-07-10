#include <cblas.h>

#include <iostream>

int main() {
    std::cout << "Hello from singularity!" << std::endl;

    int i = 0;
    double A[4] = {1.0, 2.0, 1.0, -3.0};
    double B[4] = {2.0, 0.0, 0.0, 0.5};
    double C[4] = {.5, .5, .5, .5};
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 2, 2, 2, 1, A, 2, B,
                2, 0, C, 2);

    for (i = 0; i < 4; i++) printf("%lf ", C[i]);
    printf("\n");
}
