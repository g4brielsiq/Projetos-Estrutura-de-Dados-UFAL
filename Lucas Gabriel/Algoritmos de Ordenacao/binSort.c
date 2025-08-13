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

void binSort(int total, int array[])
{
    for (int i = 1; i < total; i++)
    {
        int key = array[i];
        int j = i - 1;

        while (j >= 0 && array[j] > key)
        {
            array[j + 1] = array[j];
            j--;
        }

        array[j + 1] = key;
    }
}

int main()
{
    int total;
    printf("Informe o tamanho do vetor: ");
    scanf("%d", &total);

    int array[total];

    printf("\nPreencha o vetor: ");
    preencherArray(0, total, array);

    printf("\nVetor desordenado:");
    imprimirArray(0, total, array);
    printf("\n");

    binSort(total, array);

    printf("\nVetor ordenado:");
    imprimirArray(0, total, array);

    return 0;
}