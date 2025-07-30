#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

int QuantasCasas(long long int numero)
{
    int casas = 1;
    while((numero = numero/10) >= 1)
    {
        casas+=1;
    }
    if(casas == 1)
    {
        printf("\nO número digitado tem 1 casa");
    }
    else
    {
        printf("\nO número digitado tem %d casas",casas);
    }
   
    return casas;
}


void Impressora(long long int numero, int casas)
{
    

    printf("\nCheguei");
    return;
}

// a partir dessa função fazer a impressora

int main()
{
    long long int numero;
    bool negativo;
    printf("Insira aqui o número:");
    scanf("%lld",&numero);
    if(numero < 0)
    {
        negativo = true;
        numero = llabs(numero);  // Menos é Mais 

    }
    printf("\n");
    int casas = QuantasCasas(numero);
    Impressora(numero,casas);

}
