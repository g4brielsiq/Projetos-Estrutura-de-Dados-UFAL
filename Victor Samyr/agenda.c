#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct contato{
    char *nome;
    char *sobrenome;
    char *tel_res;
    char *tel_trab;
    char *cell_num;
    int  ind_exclud;
};

void listar_contatos(struct contato arr[], int n){
    for(int i = 0; i < n; i++){
        if(arr[i].ind_exclud == 0){
            printf("Nome: %s\nSobrenome: %s\nTelefone Residencial: %s\nTelefono do Trabalho: %s\nNúmero Celular: %s\n", arr[i].nome, arr[i].sobrenome, arr[i].tel_res, arr[i].tel_trab, arr[i].cell_num);
            printf("--------------------------------------\n");
        }
    }
}

void excluir_contato(struct contato arr[], char nome[], char sobrenome[]){
    int i = 0;
    while(1){
        if(strcmp(arr[i].nome, nome) == 0 && strcmp(arr[i].sobrenome, sobrenome) == 0){
            arr[i].ind_exclud = 1;
            break;
        }
    }
}

int run(){

    int op;

    printf("Digite o Número da Opção Desejada: \n");
    printf("1 - Adicionar Contato\n");
    printf("2 - Modificar Contato\n");
    printf("3 - Exibir Detalhes do Contado\n");
    printf("4 - Excluir Contado\n");

    printf("Opção: ");
    scanf("%d", &op);

}




int main() {

    struct contato agenda[2];
    int agenda_count = 2;

    agenda[0].nome = "Marcelo";
    agenda[0].sobrenome = "Melo";
    agenda[0].tel_res = "7389234";
    agenda[0].tel_trab = "98745334";
    agenda[0].cell_num = "34234234";
    agenda[0].ind_exclud = 0;

    agenda[1].nome = "Marcelo";
    agenda[1].sobrenome = "Melo II";
    agenda[1].tel_res = "27686753";
    agenda[1].tel_trab = "9369294";
    agenda[1].cell_num = "78295012";
    agenda[1].ind_exclud = 0;

    listar_contatos(agenda, agenda_count);
    excluir_contato(agenda, agenda[0].nome, agenda[0].sobrenome);
    listar_contatos(agenda, agenda_count);

    //printf("%ld\n", sizeof(*agenda));

    return 0;
}