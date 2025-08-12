#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct {
    char nome[20];
    char sobrenome[30];
    char telefone[15];
    char telefone_residencial[15];
    char telefone_celular[15];
    bool Deletado;
} Contato;

void exibirMenu() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif

    printf("---------- AGENDA DE CONTATOS ----------\n");
    printf("1. Mostrar lista de Contatos\n");
    printf("2. Adicionar Contato\n");
    printf("3. Apagar Contato\n");
    printf("0. Sair\n");
    printf("----------------------------------------\n");
}

bool ehnumero(char *str) {
    if (str == NULL || str[0] == '\0') {
        return false;
    }

    bool encontrouDigito = false;

    for (int i = 0; str[i] != '\0'; i++) {
        if (isdigit(str[i])) {
            encontrouDigito = true;
        } else if (!isspace(str[i])) {
            return false;
        }
    }

    return encontrouDigito;
}

int compararContatos(const void *a, const void *b) {
    return strcmp(((Contato *)a)->nome, ((Contato *)b)->nome);
}

void salvarContatosNoArquivo(const Contato contatos[], int numContatos);
void adicionarContato(Contato contatos[], int *numContatos);

void mostrarContatos(Contato *contatos, int numContatos) {
    if (numContatos == 0) {
        printf("Nenhum contato cadastrado.\n");
        return;
    }

    Contato contatosAtivosCopia[100];
    int contagemAtivos = 0;
    for (int i = 0; i < numContatos; i++) {
        if (!contatos[i].Deletado) {
            contatosAtivosCopia[contagemAtivos] = contatos[i];
            contagemAtivos++;
        }
    }

    if (contagemAtivos == 0) {
        printf("Nenhum contato ativo para exibir.\n");
        return;
    }

    qsort(contatosAtivosCopia, contagemAtivos, sizeof(Contato), compararContatos);

    printf("\nLista de Contatos:\n");
    for (int i = 0; i < contagemAtivos; i++) {
        if(i == 0)
        {
        printf("%d. %s %s \n -Telefone: %s\n -Residencial: %s\n -Celular: %s\n",
               i + 1, contatosAtivosCopia[i].nome, contatosAtivosCopia[i].sobrenome,
               contatosAtivosCopia[i].telefone, contatosAtivosCopia[i].telefone_residencial,
               contatosAtivosCopia[i].telefone_celular);
        }
        else
        {
            printf("----------------------------------------\n");
            printf("%d. %s %s \n -Telefone: %s\n -Residencial: %s\n -Celular: %s\n",
            i + 1, contatosAtivosCopia[i].nome, contatosAtivosCopia[i].sobrenome,
            contatosAtivosCopia[i].telefone, contatosAtivosCopia[i].telefone_residencial,
            contatosAtivosCopia[i].telefone_celular);
    }
}
}

void mostrarContatosApagados(Contato *contatos, int numContatos) {
    if (numContatos == 0) {
        printf("Nenhum contato recém apagado1.\n");
        return;
    }

    Contato contatosApagadosCopia[100];
    int contagemApagados = 0;
    for (int i = 0; i < numContatos; i++) {
        if (contatos[i].Deletado) {
            contatosApagadosCopia[contagemApagados] = contatos[i];
            contagemApagados++;
        }
    }

    if (contagemApagados == 0) {
        printf("Nenhum contato recém apagado para exibir.\n");
        return;
    }

    qsort(contatosApagadosCopia, contagemApagados, sizeof(Contato), compararContatos);

    printf("\nLista de Contatos Recém Apagados:\n");
    for (int i = 0; i < contagemApagados; i++) {
        if(i == 0)
        {
        printf("%d. %s %s \n -Telefone: %s\n -Residencial: %s\n -Celular: %s\n",
               i + 1, contatosApagadosCopia[i].nome, contatosApagadosCopia[i].sobrenome,
               contatosApagadosCopia[i].telefone, contatosApagadosCopia[i].telefone_residencial,
               contatosApagadosCopia[i].telefone_celular);
        }
        else
        {
            printf("----------------------------------------\n");
            printf("%d. %s %s \n -Telefone: %s\n -Residencial: %s\n -Celular: %s\n",
            i + 1, contatosApagadosCopia[i].nome, contatosApagadosCopia[i].sobrenome,
            contatosApagadosCopia[i].telefone, contatosApagadosCopia[i].telefone_residencial,
            contatosApagadosCopia[i].telefone_celular);
    }
}
}

void salvarContatosNoArquivo(const Contato contatos[], int numContatos) {
    FILE *arquivo = fopen("AgendaDaRoberta.txt", "w");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo para escrita.\n");
        return;
    }

    for (int i = 0; i < numContatos; i++) {
        if (!contatos[i].Deletado) {
            fprintf(arquivo, "Nome: %s\nSobrenome: %s\nTelefone: %s\nTelefone Residencial: %s\nTelefone Celular: %s\n--------------------\n",
                    contatos[i].nome, contatos[i].sobrenome, contatos[i].telefone,
                    contatos[i].telefone_residencial, contatos[i].telefone_celular);
        }
    }
    fclose(arquivo);
}

void CarregarContatos(Contato contatos[], int *numContatos) {
    FILE *arquivo = fopen("AgendaDaRoberta.txt", "r");
    if (arquivo == NULL) {
        *numContatos = 0;
        return;
    }

    *numContatos = 0;
    while (*numContatos < 100 &&
           fscanf(arquivo, "Nome: %[^\n]\nSobrenome: %[^\n]\nTelefone: %[^\n]\nTelefone Residencial: %[^\n]\nTelefone Celular: %[^\n]\n--------------------\n",
                  contatos[*numContatos].nome, contatos[*numContatos].sobrenome,
                  contatos[*numContatos].telefone, contatos[*numContatos].telefone_residencial,
                  contatos[*numContatos].telefone_celular) == 5) {
        contatos[*numContatos].Deletado = false;
        (*numContatos)++;
    }
    fclose(arquivo);
}

void adicionarContato(Contato contatos[], int *numContatos) {
    if (*numContatos >= 100) {
        printf("A agenda esta cheia! Nao e possivel adicionar mais contatos.\n");
        return;
    }

    printf("\n--- Adicionar Novo Contato ---\n");
    Contato novoContato;
    novoContato.Deletado = false;



    printf("Nome: ");
    fgets(novoContato.nome, sizeof(novoContato.nome), stdin);
    novoContato.nome[strcspn(novoContato.nome, "\n")] = '\0';

    printf("Sobrenome: ");
    fgets(novoContato.sobrenome, sizeof(novoContato.sobrenome), stdin);
    novoContato.sobrenome[strcspn(novoContato.sobrenome, "\n")] = '\0';

    erro_telefone:
    printf("Telefone: ");
    fgets(novoContato.telefone, sizeof(novoContato.telefone), stdin);
    novoContato.telefone[strcspn(novoContato.telefone, "\n")] = '\0';
    if(ehnumero(novoContato.telefone) == false)
    {
        printf("Telefone invalido! Por favor, digite apenas numeros.\n");
        goto erro_telefone;
    }

    erro_telefoneR:
    printf("Telefone Residencial: ");
    fgets(novoContato.telefone_residencial, sizeof(novoContato.telefone_residencial), stdin);
    novoContato.telefone_residencial[strcspn(novoContato.telefone_residencial, "\n")] = '\0';
    if(ehnumero(novoContato.telefone_residencial) == false)
    {
        printf("Telefone Residencial invalido! Por favor, digite apenas numeros.\n");
        goto erro_telefoneR;
    }

    erro_telefoneC:
    printf("Telefone Celular: ");
    fgets(novoContato.telefone_celular, sizeof(novoContato.telefone_celular), stdin);
    novoContato.telefone_celular[strcspn(novoContato.telefone_celular, "\n")] = '\0';
    if(ehnumero(novoContato.telefone_celular) == false)
    {
        printf("Telefone Celular invalido! Por favor, digite apenas numeros.\n");
        goto erro_telefoneC;
    }

    contatos[*numContatos] = novoContato;
    (*numContatos)++;

    salvarContatosNoArquivo(contatos, *numContatos);
    printf("\nContato adicionado com sucesso!\n");
}

void apagarContato(Contato contatos[], int numContatos) {
    if (numContatos == 0) {
        printf("Nenhum contato para apagar.\n");
        return;
    }

    printf("\n--- Apagar Contato ---\n");
    printf("Lista de Contatos Ativos:\n");
    int contatosAtivosIndices[100];
    int count = 0;
    for (int i = 0; i < numContatos; i++) {
        if (!contatos[i].Deletado) {
            printf("%d. %s %s\n", count + 1, contatos[i].nome, contatos[i].sobrenome);
            contatosAtivosIndices[count] = i;
            count++;
        }
    }

    if (count == 0) {
        printf("Nenhum contato ativo para apagar.\n");
        return;
    }

    int escolha;
    printf("\nDigite o numero do contato que deseja apagar (ou 0 para cancelar): ");
    scanf("%d", &escolha);

    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    if (escolha > 0 && escolha <= count) {
        int indiceOriginal = contatosAtivosIndices[escolha - 1];
        contatos[indiceOriginal].Deletado = true;
        salvarContatosNoArquivo(contatos, numContatos);
        printf("Contato apagado com sucesso!\n");
    } else if (escolha == 0) {
        printf("Operacao cancelada.\n");
    } else {
        printf("Opcao invalida.\n");
    }
}

int main() {
    int opcao;
    Contato contatos[100];
    int numContatos = 0;

    CarregarContatos(contatos, &numContatos);

    do {
        exibirMenu();
        printf("\nEscolha uma opcao: ");
        scanf("%d", &opcao);

        int c;
        while ((c = getchar()) != '\n' && c != EOF);

        switch (opcao) {
            case 1:
                mostrarContatos(contatos, numContatos);
                printf("Mostrar contatos recém apagados? \n 1-Sim 2-Nao\n");
                int apagado;
                scanf(" %d",&apagado);
                while ((c = getchar()) != '\n' && c != EOF);
                if(apagado == 1)
                {
                    mostrarContatosApagados(contatos,numContatos);
                }

                break;
            case 2:
                adicionarContato(contatos, &numContatos);
                break;
            case 3:
                apagarContato(contatos, numContatos);
                break;
            case 0:
                printf("Saindo do programa... Ate logo!\n");
                break;
            default:
                printf("Opcao invalida! Tente novamente.\n");
                break;
        }

        if (opcao != 0) {
            printf("\nPressione Enter para continuar...");
            getchar();
        }

    } while (opcao != 0);

    return 0;
}