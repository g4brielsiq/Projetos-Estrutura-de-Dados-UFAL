#include <stdio.h>
#include <string.h>

int QuantasCasas(int numero)
{
    int casas = 1;
    while((numero = numero/10) >= 1)
    {
        casas+=1;
    }
    printf("\nO número digitado tem %d casas",casas);
    return casas;
}


void Impressora(int numero, int casas)
{
    printf("\nCheguei");
    return;
}

// a partir dessa função fazer a impressora

int main()
{
    int numero;
    printf("Insira aqui o número:");
    scanf("%d",&numero);
    printf("\n");
    Impressora(numero,QuantasCasas(numero));

}