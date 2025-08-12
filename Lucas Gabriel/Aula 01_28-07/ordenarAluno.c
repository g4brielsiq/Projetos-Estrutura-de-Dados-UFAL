#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

struct Aluno
{
    char nome[20];
    double nota;
};

void preencherAlunos(int x, int num, struct Aluno alunos[])
{

    if (x < num)
    {
        fgets(alunos[x].nome, 20, stdin);

        // Remover o '\n' final do nome (se houver)
        alunos[x].nome[strcspn(alunos[x].nome, "\n")] = '\0';

        scanf("%lf", &alunos[x].nota);
        getchar(); // consumir o '\n' deixado no buffer

        return preencherAlunos(x + 1, num, alunos);
    }

    return;
}

void imprimirAlunos(int x, int num, struct Aluno alunos[])
{
    if (x < num)
    {
        printf("| %s - %.2lf |", alunos[x].nome, alunos[x].nota);
        printf("\n");

        return imprimirAlunos((x + 1), num, alunos);
    }

    return;
}

int main()
{
    int total;
    scanf("%d", &total);
    getchar();

    struct Aluno alunos[total];

    preencherAlunos(0, total, alunos);

    imprimirAlunos(0, total, alunos);
    printf("\n");

    for (int i = 0; i < (total - 1); i++)
    {
        for (int j = 0; j < (total - 1) - i; j++)
        {
            if (strcmp(alunos[j].nome, alunos[j + 1].nome) > 0)
            {
                struct Aluno aux = alunos[j];
                alunos[j] = alunos[j + 1];
                alunos[j + 1] = aux;
            }
        }
    }

    imprimirAlunos(0, total, alunos);
    printf("\n");

    for (int i = 0; i < (total - 1); i++)
    {
        for (int j = 0; j < (total - 1) - i; j++)
        {
            if (alunos[j].nome[0] == alunos[j + 1].nome[0])
            {
                if (alunos[j].nota < alunos[j + 1].nota)
                {
                    struct Aluno aux = alunos[j];
                    alunos[j] = alunos[j + 1];
                    alunos[j + 1] = aux;
                }
            }
        }
    }

    imprimirAlunos(0, total, alunos);

    return 0;
}