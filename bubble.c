#include<stdio.h>
#include<conio.h>

void bubbleSort(int arr[10], int n) {
    int i,j,t;

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (arr[i] > arr[j]) {
                t = arr[i];
                arr[i] = arr[j];
                arr[j] = t;
            }
        }
    }
}

void printVector(int arr[10], int n) {
int i;
    if (n == 0){
    printf("{ }");
    } else {
        printf("{");        
        for (i = 0; i < n - 1; i ++){
            printf(" %d ,", arr[i] );        
        }
        printf(" %d } \n", arr[n-1]);        
    }
}

int main() {
int arr[10];
int i, tamanho;
    printf("forneça a quantidade de números \n");
    scanf("%d", &tamanho);  
    for (i = 0; i < tamanho; i++){
        printf("número %d = \n", i);
        scanf("%d", &arr[i]);
    }
    printf("array fornecido \n");
    printVector(arr, tamanho);
    bubbleSort(arr, tamanho);
    printf("array ordenado \n");
    printVector(arr, tamanho);
    return 0;
}
