#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define MAX 10

void listar(char fila[], int store, int retrieve){

    for(int i = store; i < retrieve; i++){
        
        if(fila[i] != '\0'){

            printf("Posicao %d: %c\n", i, fila[i]);
        }
    }
}

void designar(char fila[], int *retrieve){

    if((*retrieve) < MAX){

        char element;
        printf("Informe: ");
        scanf(" %c", &element);

        fila[(*retrieve)] = element;

        (*retrieve)++;
    }

    else {

        printf("SEM ESPAÇO");
    }

    return;
}

void atender(char fila[], int *store, int retrieve){

    if(*store < retrieve){

        fila[*store] = '\0';

        (*store)++;
    }

    else {

        printf("\nNÃO HÁ NINGUÉM PARA ATENDER!");
    }
}

int main(){

    char fila[MAX];

    for(int i = 0; i < MAX; i++) fila[i] = '\0';

    int store = 0;
    int retrieve = 0;

    char escolha;

    while(escolha != '0'){

        printf("\n\n--- MENU ---");

        printf("\n\nA) Atender a fila");
        printf("\nD) Designar a fila");
        printf("\nL) Listar a fila");
        printf("\nE) ENCERRAR\n\n");

        printf("Escolha: ");
        scanf(" %c", &escolha);
        printf("\n");

        if(escolha == 'D'){

            designar(fila, &retrieve);
        }

        else if(escolha == 'A'){

            atender(fila, &store, retrieve);
        }

        else if(escolha == 'L'){

            listar(fila, store, retrieve);
        }

        else if(escolha == 'E'){

            escolha = '0';
        }
    }

    return 0;
}