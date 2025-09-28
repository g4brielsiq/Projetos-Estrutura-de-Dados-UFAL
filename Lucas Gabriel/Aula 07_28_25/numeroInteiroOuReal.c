#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

int main()
{
    int total;
    scanf("%d", &total);

    char num[total];
    fgets(num, total, stdin);

    if (num[0] == '+' ||
        num[0] == '0' ||
        num[0] == '1' ||
        num[0] == '2' ||
        num[0] == '3' ||
        num[0] == '4' ||
        num[0] == '5' ||
        num[0] == '6' ||
        num[0] == '7' ||
        num[0] == '8' ||
        num[0] == '9')
    {
        if (num[0] == '-')
        {
            printf("REAL");

            return 0;
        }

        for (int i = 1; i < total; i++)
        {
            if(num[i] == ',');
        }
    }

    else
    {
        printf("ERRO");
    }

    return 0;
}