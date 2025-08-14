#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

int string_size(char* str){
    int n = 0;
    while(1){
        if(str[n] != '\0'){
            n++;
        }else{
            return n;
        }
    }
}

int is_zero_after(char* str, int n, int index){
    int r = 1;
    for(int i = index; i < n; i++){
        if(str[i] != '0') r = 0;
    }
    return r;
}

int string_to_Num(char* str, int n){
    int result = 0;
    for(int i = 0; i < n; i++){
        result += (pow(10, n - (i + 1)) * (str[i] - 48));
    }
    return result;
}

void pad_String(char num[15]){
    int c = 0;
    char* trimNum = malloc(15);

    for(int i = 0; i < 15; i++){
        if(i >= 15 - string_size(num)){
            trimNum[i] = num[c];
            c++;
        }else{
            trimNum[i] = '0';   
        }
    }

    trimNum[15] = '\0';

    strncpy(num, trimNum, 15);

}

char* get_Ext_Number(char num[15]){
    char unidades[9][13] = {"Um","Dois","Tres","Quatro","Cinco","Seis","Sete","Oito","Nove"};
    char dezenas[9][13] = {"Dez","Vinte","Trinta","Quarenta","Cinquenta","Sessenta", "Setenta","Oitenta","Noventa"};
    char centenas[9][13] = {"Cem", "Duzentos", "Trezentos","Quatrocentos","Quinhentos","Seiscentos","Setecentos","Oitocentos","Novecentos"};
    
    char map[3][9][13];

    memcpy(map[0], centenas, sizeof(centenas));
    memcpy(map[1], dezenas, sizeof(dezenas));
    memcpy(map[2], unidades, sizeof(unidades));

    int numLength, count = 0;
    
    char numExt[10][13];
    char* result = malloc(40);

    numLength = string_size(num);
    int number = string_to_Num(num, numLength);
    
    if(number != 0){
        for(int i = 0; i < numLength; i++){
            if(num[i] != '0'){
                char* word = map[i + (3 - numLength)][(num[i] - 48) - 1];
                if(num[i] == '1' && num[i + 1] != '0' && i == numLength - 2){
                    switch ((num[i] - 48) + (num[i + 1] - 48)){
                    case 2:
                        word = "Onze";
                        break;
                    case 3:
                        word = "Doze";
                        break;
                    case 4:
                        word = "Treze";
                        break;
                    case 5:
                        word = "Quartoze";
                        break;
                    case 6:
                        word = "Quinze";
                        break;
                    case 7:
                        word = "Desseseis";
                        break;
                    case 8:
                        word = "Dessesete";
                        break;
                    case 9:
                        word = "Dezoito";
                        break;
                    case 10:
                        word = "Dezsenove";
                        break;
                    }
                    i++;
                }

                if(((num[i] - 48) - 1) == 0 && (i + (3 - numLength)) == 0 && !is_zero_after(num, numLength, i + 1)){
                    word = "Cento";
                }
                
                memcpy(numExt[count], word, string_size(word) + 1);
                
                count++;

            }
        }

        int i = 0;
        
        for(int a = 0; a < count; a++){
            for(int b = 0; b < string_size(numExt[a]); b++){
                result[b + i] = numExt[a][b];
                if(b == string_size(numExt[a]) - 1){
                    i += b + 1;
                }
            }
            if(a != count - 1){
                result[i] = ' ';
                result[i + 1] = 'e';
                result[i + 2] = ' ';
                i += 3;
            }
        }
        result[i] = '\0';
        return result;
    }else{
        return "Zero";
    }
}


int main() {

    char casasS[5][10] = {"Trilhão", "bilhão", "milhão", "mil", ""};
    char casasP[5][10] = {"Trilhões", "bilhões", "milhões", "mil", ""};
    
    char num[15];

    printf("Digite o Número: ");
    scanf("%s", num);

    if(string_size(num) <= 15){

        int exec = string_size(num) / 3;
        int resto = string_size(num) % 3;

        pad_String(num);
    
        if((exec == 1 && resto == 0) || string_to_Num(num, 15) == 0){
            printf("%s\n", get_Ext_Number(num));
        }
        else{
            for(int a = 0; a < 5; a++){
                char* numPart = malloc(15);
                for(int b = 0; b < 3; b++){
                    numPart[b] = num[b + (3*a)];
                }
                
                char* extNum = malloc(40);
                strncpy(extNum, get_Ext_Number(numPart), 40);

                if(strcmp(extNum, "Zero") != 0){
                    if(strcmp(extNum, "Um") == 0 && strcmp(casasS[a], "mil") == 0){
                        printf("%s ", (string_to_Num(extNum, 3) > 1) ? casasP[a] : casasS[a]);
                    }else{
                        printf("%s %s ", extNum, (string_to_Num(extNum, 3) > 1) ? casasP[a] : casasS[a]);
                    }
                }
            }
            printf("\n");
        }

    }else{
        printf("São Permitidos Apenas Numeros com até 15 casas\n");
    }

    

    return 0;
}