// Merge Sort (Ordenação por Divisão e Conquista)
// - Divide o vetor em duas metades recursivamente
// - Ordena cada metade
// - Junta as metades ordenadas

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

// Função para mesclar duas metades do vetor
void merge(int vetor[], int inicio, int meio, int fim)
{
    int n1 = meio - inicio + 1;
    int n2 = fim - meio;

    // Vetores temporários para cada metade
    int L[n1], R[n2];

    // Copia os dados para os vetores temporários
    for (int i = 0; i < n1; i++)
        L[i] = vetor[inicio + i];
    for (int j = 0; j < n2; j++)
        R[j] = vetor[meio + 1 + j];

    int i = 0, j = 0, k = inicio;

    // Junta os elementos das duas metades em ordem
    while (i < n1 && j < n2)
    {
        if (L[i] <= R[j])
        {
            vetor[k] = L[i];
            i++;
        }
        else
        {
            vetor[k] = R[j];
            j++;
        }
        k++;
    }

    // Copia os elementos restantes de L, se houver
    while (i < n1)
    {
        vetor[k] = L[i];
        i++;
        k++;
    }

    // Copia os elementos restantes de R, se houver
    while (j < n2)
    {
        vetor[k] = R[j];
        j++;
        k++;
    }
}

// Função principal do Merge Sort (recursiva)
void mergeSort(int vetor[], int inicio, int fim)
{
    if (inicio < fim)
    {
        int meio = inicio + (fim - inicio) / 2;

        // Ordena primeira metade
        mergeSort(vetor, inicio, meio);
        // Ordena segunda metade
        mergeSort(vetor, meio + 1, fim);
        // Mescla as duas metades ordenadas
        merge(vetor, inicio, meio, fim);
    }
}

// Programa principal
int main()
{
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

    mergeSort(array, 0, total - 1);

    printf("\nVetor ordenado:");
    imprimirArray(0, total, array);
    printf("\n");

    return 0;
}