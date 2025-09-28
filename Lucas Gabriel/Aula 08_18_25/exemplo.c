#include <stdio.h>

void copia(int x) {
    x++;
    printf("Dentro da copia: %d\n", x);
}

void referencia(int *x) {
    (*x)++;
    printf("Dentro da referencia: %d\n", (*x));
}

int main() {
    
    int valor = 10;

    copia(valor);
    printf("Depois da copia: %d\n", valor);

    referencia(&valor);
    printf("Depois da referencia: %d\n", valor);

    struct Aluno{

        int idade;
        int altura;
    }

    return 0;
}