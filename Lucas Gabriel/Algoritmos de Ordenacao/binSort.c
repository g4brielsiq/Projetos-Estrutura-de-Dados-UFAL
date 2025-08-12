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

void insertionSort(int total, int array[])
{
    for (int i = 1; i < total; i++)
    {
        int chave = array[i];
        int j = i - 1;

        while (j >= 0 && array[j] > chave)
        {
            array[j + 1] = array[j];
            j--;
        }

        array[j + 1] = chave;
    }
}

// Bin Sort ou Bucket Sort
// - 

void binSort(int total, int array[])
{
    int i, j, k;
    int buckets[10][10];
    int count[10];

    for (i = 0; i < 10; i++)
    {
        count[i] = 0;
    }

    for (i = 0; i < total; i++)
    {
        int idx = array[i] / 10;
        buckets[idx][count[idx]++] = array[i];
    }

    for (i = 0; i < 10; i++)
    {
        insertionSort(buckets[i], count[i]);
    }

    k = 0;
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < count[i]; j++)
        {
            array[k++] = buckets[i][j];
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

    binSort(total, array);

    printf("\nVetor ordenado:");
    imprimirArray(0, total, array);

    return 0;
}