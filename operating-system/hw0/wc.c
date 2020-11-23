#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

void wordcount(FILE* input, char* input_name);

static int total_line = 0, total_word = 0, total_char = 0;

int main(int argc, char* argv[]) {
    int i = 1;
    FILE* fp;
    if(argv[1] == NULL)
        wordcount(stdin, " ");
    while(argv[i] != 0){
        fp = fopen(argv[i], "r");
        if(fp == NULL){
            fprintf(stderr, "This file could not be opened or does not exist.\n");
            return 1;
        }
        wordcount(fp, argv[i]);
        fclose(fp);
        i++;
    }
    if(argc > 2) {
        printf("%5d %5d %5d  total\n", total_line, total_word, total_char);
    }
    return 0;
}

void wordcount(FILE* input, char* input_name){
    int line_num = 0, char_num = 0, word_num = 0;
    char ch = 0;
    int newWord = 1;
    while((ch = (char)fgetc(input)) != EOF){

        char_num++;

        if(ch == '\n' || ch == '\r'){
            line_num++;
            newWord = 1;
        }
        else if(ch == ' ' || ch == '\t'){
            newWord = 1;
        }

        else{
            if(newWord){
                word_num++;
                newWord = 0;
            }
        }
    }
    total_char += char_num;
    total_line += line_num;
    total_word += word_num;
    printf("%5d %5d %5d  %s\n", line_num, word_num, char_num, input_name);
}
