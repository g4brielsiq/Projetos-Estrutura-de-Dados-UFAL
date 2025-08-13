#include <stdio.h>
#include <ctype.h>
#include <string.h>

struct aluno {
  char nome[30];
  float nota;
};

int compare(char* c1, char* c2){
    int r = 0, i = 0;

    while(1){
        if(c1[i] == '\0' || c2[i] == '\0'){
            r = (c1[i] == '\0' ? 1 : 2);
            break;
        }
        else if((int) toupper(c1[i]) < (int)toupper(c2[i])){
            r = 1;
            break;
        }else if ((int) toupper(c1[i]) > (int)toupper(c2[i])){
            r = 2;
            break;
        }
        i++;
    }

    return r;
}

void sort(struct aluno arr[], int init, int end){
    printf("%d %d\n", init, end);
    for(int a = init; a <= end; a++){
        for(int b = init; b <= end; b++){
            if(arr[b].nota < arr[b + 1].nota && strcmp(arr[b].nome, arr[b + 1].nome) == 0){
                struct aluno aux = arr[b];
                arr[b] = arr[b + 1];
                arr[b + 1] = aux;
            }
        
        }
    }    
}

void getData(struct aluno arr[], int qtd){
    for(int i = 0; i < qtd; i++){
        printf("Alundo %d de %d: \n", i + 1, qtd);
        printf("Nome Aluno %d: \n", i + 1);
        getchar();
        fgets(arr[i].nome, 30, stdin);

        for(int a = 0; a < 30; a++){
            if(arr[i].nome[a] == '\n'){
                arr[i].nome[a] = '\0';
            }
        }

        printf("Nota Aluno %d: \n", i + 1);
        scanf("%f", &arr[i].nota);
    }

}

int main() {
    
    int qtd = 4;
    struct aluno turma[30];
    printf("Quantos Alunos Vai Cadastrar?\n");
    scanf("%d", &qtd);

    getData(turma, qtd);

    printf("------- Dados Não Organizados -------\n");

    for(int i = 0; i < qtd; i++){
        printf("%d - Nome: %s, nota: %.2f\n", i, turma[i].nome, turma[i].nota);
    }

    for(int a = 0; a < qtd; a++){
        for(int i = 0; i < qtd - 1; i++){
            if(compare(turma[i].nome, turma[i + 1].nome) == 2){
                struct aluno aux = turma[i];
                turma[i] = turma[i + 1];
                turma[i + 1] = aux;
            }        
        }
    }
    
    printf("\n");

    printf("------- Dados Organizados Por Nome -------\n");

    for(int i = 0; i < qtd; i++){
        printf("%d - Nome: %s, nota: %.2f\n", i, turma[i].nome, turma[i].nota);
    }

    int a = 0, b = 0, i = 0;

    while(turma[i].nome == turma[i + 1].nome || (!(turma[i].nome == turma[i + 1].nome) && i < qtd - 1)){
        printf("%s %s - %d\n", turma[i].nome, turma[i + 1].nome, compare(turma[i].nome, turma[i + 1].nome));
        if(strcmp(turma[i].nome, turma[i + 1].nome) == 0){
            b++;   
        }else{
            sort(turma, a, b);
            a = b+1;
            b++;
        }
        i++;
    }

    sort(turma, a, b);

    printf("\n------- Dados Organizados Por Nome e Nota-------\n");

    for(int i = 0; i < qtd; i++){
        printf("%d - Nome: %s, nota: %.2f\n", i, turma[i].nome, turma[i].nota);
    }




    return 0;
}