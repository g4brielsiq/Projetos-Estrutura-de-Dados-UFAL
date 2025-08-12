#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// Primeiro ponto extra: Faça um programa que gerencie a agenda de celular da Roberta, de modo que os dados estejam organizados em ordem crescente de nome (ordem alfabética), e que seja permitido para cada contato salvo armazenar 5 informações (nome, sobrenome, telefone residencial, telefone do trabalho e número do celular.

// Você deve implementar de modo que ao deletar um contato do vetor não seja realizada a operação de deslocamento do conteúdo da agenda, mas simplesmente colocado um flag para indicar que a posição não está mais sendo usada.

// Ao inserir um novo contato você deve deslocar contatos para a direita ou esquerda em função da posição vazia mais próxima da posição onde o novo contato deve ser incluído.

// Para ordenar a sua agenda você pode usar qualquer algoritmo de ordenação, para organizar os campos do tipo contato você deve definir uma struct. Seu objetivo é gerar esse algoritmo buscando diminuir a complexidade do algoritmo gerado.

// O envio deve ser realizado somente por uma membro da equipe no dia 13/8/2025 pelo Moodle e um vídeo mostrando o comportamento de seu algoritmo deve ser feito e colocado no grupo de whatsapp da turma.

struct Contato
{
    char nome[20], sobrenome[40];
    int teleResidencial, teleTrabalho, teleCelular;
    bool ativo;
};

void preencherContatos(int x, int num, struct Contato contatos[])
{

    if (x < num)
    {
        printf("\nInforme o nome: ");
        fgets(contatos[x].nome, 20, stdin);
        contatos[x].nome[strcspn(contatos[x].nome, "\n")] = '\0';

        printf("Informe o sobrenome: ");
        fgets(contatos[x].sobrenome, 40, stdin);
        contatos[x].sobrenome[strcspn(contatos[x].sobrenome, "\n")] = '\0';

        printf("Informe o telefone residencial: ");
        scanf("%d", &contatos[x].teleResidencial);
        getchar();

        printf("Informe o telefone de trabalho: ");
        scanf("%d", &contatos[x].teleTrabalho);
        getchar();

        printf("Informe o celular: ");
        scanf("%d", &contatos[x].teleCelular);
        getchar();

        contatos[x].ativo = true;

        return preencherContatos(x + 1, num, contatos);
    }

    return;
}

void imprimirContatos(int x, int num, struct Contato contatos[])
{
    if (x < num)
    {
        if (contatos[x].ativo == true)
        {
            printf("| %s - %s - %d - %d - %d |", contatos[x].nome, contatos[x].sobrenome, contatos[x].teleResidencial, contatos[x].teleTrabalho, contatos[x].teleCelular);
            printf("\n");
        }

        return imprimirContatos((x + 1), num, contatos);
    }

    return;
}

void bubbleSort(int total, struct Contato array[])
{
    for (int i = 0; i < total - 1; i++)
    {
        for (int j = 0; j < (total - 1) - i; j++)
        {
            if (strcmp(array[j].nome, array[j + 1].nome) > 0)
            {
                struct Contato aux = array[j];
                array[j] = array[j + 1];
                array[j + 1] = aux;
            }
        }
    }
}

void adicionarContato(int total, struct Contato array[])
{
    bool buscador = false;

    for (int i = 0; i < total; i++)
    {
        if (array[i].ativo == false)
        {
            buscador = true;
        }
    }

    if (buscador == false)
    {
        printf("\nERRO: NÃO HÁ ESPAÇO PARA UM NOVO CONTATO!");

        return;
    }

    struct Contato novo;
    novo.ativo = true;

    getchar();
    printf("\nInforme o nome: ");
    fgets(novo.nome, 20, stdin);
    novo.nome[strcspn(novo.nome, "\n")] = '\0';

    printf("Informe o sobrenome: ");
    fgets(novo.sobrenome, 40, stdin);
    novo.sobrenome[strcspn(novo.sobrenome, "\n")] = '\0';

    printf("Informe o telefone residencial: ");
    scanf("%d", &novo.teleResidencial);
    getchar();

    printf("Informe o telefone de trabalho: ");
    scanf("%d", &novo.teleTrabalho);
    getchar();

    printf("Informe o celular: ");
    scanf("%d", &novo.teleCelular);
    getchar();

    int posInsercao = 0;

    while (posInsercao < total && array[posInsercao].ativo &&
           strcmp(novo.nome, array[posInsercao].nome) > 0)
    {
        posInsercao++;
    }

    int esq = posInsercao - 1;
    int dir = posInsercao + 1;
    int direcao = 0;

    if (!array[posInsercao].ativo)
    {
        array[posInsercao] = novo;
        printf("Contato adicionado!\n");
        return;
    }

    while (esq >= 0 || dir < total)
    {
        if (esq >= 0 && !array[esq].ativo)
        {
            direcao = -1;
            break;
        }

        if (dir < total && !array[dir].ativo)
        {
            direcao = 1;
            break;
        }

        esq--;
        dir++;
    }

    if (direcao == -1)
    {
        for (int i = esq; i < posInsercao; i++)
        {
            array[i] = array[i + 1];
        }
        posInsercao--;
    }

    else if (direcao == 1)
    {
        for (int i = dir; i > posInsercao; i--)
        {
            array[i] = array[i - 1];
        }
    }

    else
    {
        printf("ERRO: Não há espaço para novo contato!\n");
        return;
    }

    array[posInsercao] = novo;
    printf("Contato adicionado!\n");
}

// void adicionarContato(int total, struct Contato array[])
// {
//     int posicao, menorEsquerdo, menorDireito;
//     printf("\nInforme a posição do contato para adicionar: ");
//     scanf("%d", &posicao);

//     bool buscador = false;

//     for (int i = 0; i < total; i++)
//     {
//         if (array[i].ativo == false)
//         {
//             buscador = true;
//         }
//     }

//     if (buscador == false)
//     {
//         printf("\nERRO: NÃO HÁ ESPAÇO PARA UM NOVO CONTATO!");

//         return;
//     }

//     if (array[posicao].ativo == true)
//     {
//         int auxEsq = posicao;

//         while (array[auxEsq].ativo == true)
//         {
//             auxEsq--;
//         }

//         menorEsquerdo = posicao - auxEsq;

//         int auxDir = posicao;

//         while (array[auxDir].ativo == true)
//         {
//             auxDir++;
//         }

//         menorDireito = auxDir - posicao;

//         if (menorEsquerdo < menorDireito)
//         {
//             for (int i = auxEsq; i < posicao; i++)
//             {
//                 array[i] = array[i + 1];
//             }
//         }

//         else
//         {
//             for (int i = auxDir; i > posicao; i--)
//             {
//                 array[i] = array[i - 1];
//             }
//         }

//         array[posicao].ativo = false;
//     }

//     else if (array[posicao].ativo == false)
//     {
//         fgets(array[posicao].nome, 20, stdin);
//         array[posicao].nome[strcspn(array[posicao].nome, "\n")] = '\0';

//         fgets(array[posicao].sobrenome, 40, stdin);
//         array[posicao].sobrenome[strcspn(array[posicao].sobrenome, "\n")] = '\0';

//         scanf("%d", &array[posicao].teleResidencial);
//         getchar();

//         scanf("%d", &array[posicao].teleTrabalho);
//         getchar();

//         scanf("%d", &array[posicao].teleCelular);
//         getchar();

//         array[posicao].ativo = true;
//     }
// }

void removerContato(int total, struct Contato array[])
{
    char nome[20];
    printf("\nInforme o nome do contato para remover: ");
    scanf("%s", &nome);

    for (int i = 0; i < total; i++)
    {
        if (nome == array[i].nome)
        {
            printf("\nContato removido -> %s\n", array[i].nome);
            array[i].ativo = false;

            return;
        }
    }

    printf("\nERRO: ESSE CONTATO NÃO EXISTE!\n");
}

int main()
{
    int total;
    printf("Informe o total de contatos: ");
    scanf("%d", &total);
    getchar();

    struct Contato contatos[total];

    preencherContatos(0, total, contatos);

    printf("\n");

    imprimirContatos(0, total, contatos);

    printf("\n");

    bubbleSort(total, contatos);

    imprimirContatos(0, total, contatos);

    int escolha = 1;

    while (escolha != 0)
    {
        printf("\n-- Opções --");
        printf("\n\n0 - Encerrrar.");
        printf("\n1 - Adicionar contato.");
        printf("\n2 - Remover contato.");
        printf("\n3 - Listar agenda.");

        printf("\n\nEscolhe alguma opção: ");
        scanf("%d", &escolha);

        if (escolha == 0)
        {
            escolha = 0;
        }

        else if (escolha == 1)
        {
            adicionarContato(total, contatos);
        }

        else if (escolha == 2)
        {
            removerContato(total, contatos);
        }

        else if (escolha == 3)
        {
            printf("\n");
            imprimirContatos(0, total, contatos);
        }

        else
        {
            printf("ERRO: ESCOLHA INVÁLIDA!");
        }
    }

    return 0;
}