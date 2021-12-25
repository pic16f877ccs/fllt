/* Fllt - Fill text with the specified character (standard is a space)
   up to the maximum line length of the text.
   Copyright (C) 2021 by pic16f877ccs

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <getopt.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "utf8.h"

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "fllt"
#endif

#ifndef AUTHORS
#define AUTHORS "Karl Wulfert"
#endif

#ifndef VERSION
#define VERSION 0.1
#endif

#define LIMIT_FILL 100000000
#define SIZE 10048576

#define NEW_LINE 1

/* 
    Options from GNU's coreutils/src/system.h */

enum {
    GETOPT_HELP_CHAR = (CHAR_MIN - 2),
    GETOPT_VERSION_CHAR = (CHAR_MIN - 3)
};
#define GETOPT_HELP_OPTION_DECL \
    "help", no_argument, NULL, GETOPT_HELP_CHAR
#define GETOPT_VERSION_OPTION_DECL \
    "version", no_argument, NULL, GETOPT_VERSION_CHAR
#define case_GETOPT_HELP_CHAR                   \
    case GETOPT_HELP_CHAR:                        \
        usage(EXIT_SUCCESS);                        \
    break;
#define case_GETOPT_VERSION_CHAR(Program_name, Version, Authors)        \
    case GETOPT_VERSION_CHAR:                                             \
        fprintf(stdout, "%s version %0.3f\nCopyright (c) 2021 by pic16f877ccs\n"   \
                "%s is free software and comes with ABSOLUTELY NO WARRANTY.\n" \
                "Distributed under the CPLv3 License.\n\nWritten by %s\n",      \
                Program_name, (double) Version, Program_name, Authors);       \
        exit(EXIT_SUCCESS);                                                   \
    break;
/* end code from system.h */

void usage(int status) {
    fputs("\nUsage: fllt [OPTION]... [FILE]\n\
Print each line of text with placeholder character,\n\
up to the maximum length of a line of text.\n", stdout);
    fputs("\nOptions:\n\
  -f,   --fill=CHARACTER      use single CHARACTER to filling (default: \\s)\n\
  -n,   --number=INTEGER      use positive INTEGER to expand max lines (default: 0)\n\
  -e,   --end=STRING          use STRING to print end of line\n\
        --help                display this help and exit\n\
        --version             output version information and exit\n\n", stdout);

    exit(status);
}

int main(int argc, char *argv[]) {
    FILE *stream = NULL;

    int number_chr_fill = 0, chr = 0, chr_counter = 0 , max_line_size = 0 , option = 0;
    int realloc_size = SIZE , current_chr_counter = 0;
    
    char str_fill[10] = {" "}; 
    char *start_str = NULL;
    char *end_str = NULL;
    char *text_buff = NULL;
    
    bool start_flag = false;
    bool end_flag = false;

    static struct option const long_options[] = {
        /* Options with an argument */
        {"fill", required_argument, 0, 'f'},
        {"number", required_argument, 0, 'n'},
        {"end", required_argument, 0, 'e'},
        {"start", required_argument, 0, 's'},
        {GETOPT_HELP_OPTION_DECL},
        {GETOPT_VERSION_OPTION_DECL},
        {NULL, 0, NULL, 0}
    };

    while((option = getopt_long(argc, argv, "f:n:s:e:", long_options, NULL)) != -1) {
        switch(option) {
            case '?':
                exit(EXIT_FAILURE);
                break;

            case 'f':
                if(utf8len(optarg) > 1) {
                    fprintf(stderr, "fllt: argument to -- 'f' should be a one character\n\
                            Try 'fllt --help' for more information.\n");
                    exit(EXIT_FAILURE);
                }

                utf8cpy(str_fill, optarg);
                break;

            case 'n': 
               for(int i = 0; i < (int)(strlen(optarg));) {
                   if(isdigit(optarg[i]) == 0) {
                       fprintf(stderr, "fllt: argument to -- 'n' should be a non-negative integer\n\
                               Try 'fllt --help' for more information.\n");
                       exit(EXIT_FAILURE);
                   }

                   i++;
               }

                if((number_chr_fill = atoi(optarg)) > LIMIT_FILL) {
                    fprintf(stderr, "fllt: should then be a non down six digits\n\
                            Try 'fllt --help' for more information.\n");
                    exit(EXIT_FAILURE);
                }
                break;

            case 's' :
                start_flag = true;
                start_str = optarg;
                break;

            case 'e':
                end_flag = true;
                end_str = optarg;
                break;

            case ':':
                exit(EXIT_FAILURE);
                break;

            case_GETOPT_HELP_CHAR;

            case_GETOPT_VERSION_CHAR(PROGRAM_NAME, VERSION, AUTHORS);

            default:
                usage(EXIT_FAILURE);
        }
    }

     if(argc == optind) {
        stream = stdin;

    } else if((argc - optind) > 1) {
        printf("Try 'fllt --help' for more information.\n");
        exit(EXIT_FAILURE);
        
    } else if((argc - optind) == 1) {
        stream = fopen(argv[optind], "r");

        if(NULL == stream) {
            fprintf(stderr, "%s\n", strerror(errno));
            usage(EXIT_FAILURE);
        }
    }

    text_buff = (char*)malloc(sizeof(char) * SIZE);

    if(text_buff == NULL){
        exit(EXIT_FAILURE);
    }

    while((chr = getc(stream)) != EOF) {

        if(chr_counter >= realloc_size){
            realloc_size = realloc_size + SIZE;
            text_buff = realloc(text_buff, realloc_size);

            if(text_buff == NULL) {
                exit(EXIT_FAILURE);
            }
        } 

        if(chr != 10) {
            if( (chr & 0xc0) != 0x80) {
                current_chr_counter++;
            }
        } else {
            if(max_line_size < current_chr_counter) {
                max_line_size = current_chr_counter;
            }
            current_chr_counter = 0;
        }

        text_buff[chr_counter++] = chr;
    }

    int i = 0;
    if(current_chr_counter != 0) {
        /*
         if there is one line in the text, then print */
        max_line_size = current_chr_counter;

        if(start_flag)
            fputs(start_str, stdout);

        for(; i < max_line_size;) {
            fputc(text_buff[i], stdout);
            i++;
        }

        for(;number_chr_fill--;)
            fputs(str_fill, stdout);

        /* print the end string of the text line */
        if(end_flag)
            fputs(end_str, stdout);

        putchar('\n');
    } else {
        /*
        if there are many lines in the text, then output */

        for(; i < chr_counter;) {
            
            /* print the first string of the text line */
            if(start_flag)
                if(current_chr_counter == 0) {
                    fputs(start_str, stdout);
            }

            if(text_buff[i] != '\n') {
                fputc(text_buff[i], stdout);
                if( (text_buff[i] & 0xc0) != 0x80)
                    current_chr_counter++;

            } else {

                for(int j = 0; j < ((max_line_size - current_chr_counter) + number_chr_fill); j++)
                    fputs(str_fill, stdout);

                current_chr_counter = 0;

                /* print the end string of the text line */
                if(end_flag)
                    fputs(end_str, stdout);
                
                putchar('\n');
            }

            i++;
        }
    }


    free(text_buff);
    fclose(stream);
    exit(EXIT_SUCCESS);
}

