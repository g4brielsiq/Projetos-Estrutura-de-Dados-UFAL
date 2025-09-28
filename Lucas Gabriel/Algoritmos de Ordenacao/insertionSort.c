// Insertion Sort (Ordenação por Inserção)
// - Percorre o vetor elemento por elemento da esquerda para a direita
// - Insere cada elemento na posição correta dentro da parte já ordenada


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

void preencherArray(int x, int total, int array[])
{
    if (x < total)
    {
        scanf("%d", &array[x]);

        return preencherArray(x + 1, total, array);
    }

    return;
}

void imprimirArray(int x, int total, int array[])
{
    if (x < total)
    {
        printf("| %d |", array[x]);

        return imprimirArray(x + 1, total, array);
    }

    return;
}

void insertionSort(int vetor[], int total) {

    for (int i = 1; i < total; i++) {

        int chave = vetor[i]; // elemento atual a ser inserido
        int j = i - 1;

        // Desloca os elementos maiores para frente
        while (j >= 0 && vetor[j] > chave) {

            vetor[j + 1] = vetor[j];
            j--;
        }

        // Insere a chave na posição correta
        vetor[j + 1] = chave;
    }
}

// Programa principal
int main() {

    system("cls");

    int total;
    printf("Informe o tamanho do vetor: ");
    scanf("%d", &total);

    int array[total];

    printf("\nPreencha o vetor: ");
    preencherArray(0, total, array);

    printf("\nVetor original:");
    imprimirArray(0, total, array);
    printf("\n");

    insertionSort(array, total);

    printf("\nVetor ordenado:");
    imprimirArray(0, total, array);

    return 0;
}
