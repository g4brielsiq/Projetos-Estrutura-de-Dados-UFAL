// Selection Sort (Ordenação por Seleção)
// - Seleciona o menor valor e o coloca na primeira posição
// - Seleciona o segundo menor valor e o coloca na segunda posição

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

// Função de ordenação Selection Sort
void selectionSort(int vetor[], int total) {

    int minIndex;

    // Percorre todo o vetor
    for (int i = 0; i < total - 1; i++) {
        // Assume que o menor elemento está na posição i
        minIndex = i;

        // Busca se há algum menor no resto do vetor
        for (int j = i + 1; j < total; j++) {
            if (vetor[j] < vetor[minIndex]) {
                minIndex = j; // atualiza índice do menor
            }
        }

        // Troca o menor encontrado com o da posição i
        if (minIndex != i) {

            int aux = vetor[i];
            vetor[i] = vetor[minIndex];
            vetor[minIndex] = aux;
        }
    }
}

// Programa principal
int main() {

    int total;
    printf("\nInforme o tamanho do vetor: ");
    scanf("%d", &total);

    int array[total];

    printf("\nPreencha o vetor: ");
    preencherArray(0, total, array);

    printf("\nVetor original:");
    imprimirArray(0, total, array);
    printf("\n");

    selectionSort(array, total);

    printf("\nVetor ordenado:");
    imprimirArray(0, total, array);

    return 0;
}