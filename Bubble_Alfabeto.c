#include <stdio.h>
#include <string.h>
typedef struct { 
    char nome[20]; 
    float nota;
} aluno;

void bubbleSortNome(aluno turma[10], int n) {
    int i,j;
    aluno temp;

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) 
        {
            if (strcmp(turma[i].nome, turma[j].nome)>0) 
                {
                    temp = turma[i];
                    turma[i] = turma[j];
                    turma[j] = temp;
                }
         }
    }
}   

void BubbleArray(aluno turma[10], int n)
{
    int i, j;
    aluno temp;

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) 
        {
            if (strcmp(turma[i].nome, turma[j].nome) == 0) 
            {
                if(turma[i].nota < turma[j].nota)
                {
                    temp = turma[i];
                    turma[i] = turma[j];
                    turma[j] = temp;
                }
            }
         }
    }

}


int main ()
{
    aluno turma[10];
    
    
    for(int i = 0; i < 10; i++)
    {
        printf("Nome do aluno %d: ", i + 1);
        fgets(turma[i].nome, sizeof(turma[i].nome), stdin);
        turma[i].nome[strcspn(turma[i].nome, "\n")] = '\0'; 
        
        printf("Nota do aluno %d: ", i + 1);
        scanf("%f", &turma[i].nota); 
        
       
        while(getchar() != '\n');
    }

    
    bubbleSortNome(turma, 10); 
    BubbleArray(turma, 10);  

    
    printf("\n Alunos Ordenados: \n");
    for(int i = 0; i < 10; i++)
    {
        printf("Nome: %-20s  Nota: %.2f\n", turma[i].nome, turma[i].nota);
    }
    
    return 0;
}