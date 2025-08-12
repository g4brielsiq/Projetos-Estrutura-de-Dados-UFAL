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

// Merge Sort
// - Compara índeces em pares (maior e menor)
// - Garante que a cada varredura o maior valor esteja no final

void mergeSort(int total, int array[])
{
    for (int i = 0; i < total - 1; i++)
    {
        for (int j = 0; j < (total - 1) - i; j++)
        {
            if (array[j] > array[j + 1])
            {
                int aux = array[j];
                array[j] = array[j + 1];
                array[j + 1] = aux;
            }
        }
    }
}

int main()
{
    int total;
    printf("\nInforme o tamanho do vetor: ");
    scanf("%d", &total);

    int array[total];

    printf("\nPreencha o vetor: ");
    preencherArray(0, total, array);

    printf("\nVetor desordenado:");
    imprimirArray(0, total, array);
    printf("\n");

    mergeSort(total, array);

    printf("\nVetor ordenado:");
    imprimirArray(0, total, array);

    return 0;
}