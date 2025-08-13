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

// Função auxiliar para mesclar duas metades ordenadas
void merge(int array[], int esquerda, int meio, int direita)
{
    int n1 = meio - esquerda + 1;
    int n2 = direita - meio;

    int L[n1], R[n2];

    for (int i = 0; i < n1; i++)
        L[i] = array[esquerda + i];
    for (int j = 0; j < n2; j++)
        R[j] = array[meio + 1 + j];

    int i = 0, j = 0, k = esquerda;

    while (i < n1 && j < n2)
    {
        if (L[i] <= R[j])
            array[k++] = L[i++];
        else
            array[k++] = R[j++];
    }

    while (i < n1)
        array[k++] = L[i++];

    while (j < n2)
        array[k++] = R[j++];
}

// Merge Sort recursivo
void mergeSort(int array[], int esquerda, int direita)
{
    if (esquerda < direita)
    {
        int meio = esquerda + (direita - esquerda) / 2;

        mergeSort(array, esquerda, meio);
        mergeSort(array, meio + 1, direita);

        merge(array, esquerda, meio, direita);
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

    mergeSort(array, 0, total - 1);

    printf("\nVetor ordenado:");
    imprimirArray(0, total, array);

    return 0;
}
