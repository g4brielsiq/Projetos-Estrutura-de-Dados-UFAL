#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

void clear()
{

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    system("clear");
#endif

#if defined(_WIN32) || defined(_WIN64)
    system("cls");
#endif
}

int main()
{

    // clear();

    int matriz[5][5][5][5];

    for (int w = 0; w < 5; w++)
    {
        for (int x = 0; x < 5; x++)
        {
            for (int y = 0; y < 5; y++)
            {
                for (int z = 0; z < 5; z++)
                {
                    matriz[w][x][y][z] = z;
                }
            }
        }
    }

    for (int w = 0; w < 5; w++)
    {
        for (int x = 0; x < 5; x++)
        {
            for (int y = 0; y < 5; y++)
            {
                for (int z = 0; z < 5; z++)
                {
                    printf("| teste |");
                }

                printf("\n");
            }

            printf("\n");
        }

        printf("\n");
    }

    return 0;
}