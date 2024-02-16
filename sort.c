#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quicksort.h"


#define MAX_STRLEN     66 // Not including '\0'
#define MAX_ELEMENTS 1024

// Usage Message
void display_usage() {
	printf("Usage: ./sort [i|d] [filename]\n");
	printf("    -i: Specifies the input contains ints.\n");
	printf("    -d: Specifies the input contains doubles.\n");
	printf("    filename: The file to sort. If no file is supplied, input is read from\n");
	printf("              stdin.\n");
	printf("    No flags defaults to sorting strings.\n");
}
int main(int argc, char **argv) {
	// getopt setup vars
	int iflag = 0;
	int dflag = 0;
	char *input = NULL;
	int c;
	opterr = 0;
	
	// getopt loop
	while ((c = getopt(argc, argv, "id")) != -1) {
		switch (c) {
			case 'i':
				iflag = 1;
				break;
			case 'd':
				dflag = 1;
				break;
			case '?':
				fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
				display_usage();
				return EXIT_FAILURE;
			default:
				return EXIT_FAILURE;

		}
	}
	
	// Handling too many flags
	if ((iflag + dflag) > 1) {
		fprintf(stderr, "Error: Too many flags specified.\n");
	}
	
	// Intaking filename and handling too many files
	if ((iflag + dflag) <= 1) {
		int index = optind;
		input = argv[index];
		if (index + 1 < argc) {
			fprintf(stderr, "Error: Too many files specified.\n");
			return EXIT_FAILURE;
		}	
	}
	
	// File operations and error handling
	char buf[MAX_STRLEN+1];
	FILE *file; 
	if (input != NULL) {
		file = fopen(input, "r");
		if (file == NULL) {
			fprintf(stderr, "Error: Cannot open '%s'. %s.\n", input, strerror(errno));
			return EXIT_FAILURE;	
		}

	} else {
		file = stdin;
	}

	int int_values[MAX_ELEMENTS];
	double dbl_values[MAX_ELEMENTS];
	char* str_values[MAX_ELEMENTS];
	int* int_val = int_values;
	double* dbl_val = dbl_values;
	char** str_val = str_values;

	
	// Processing file/stdin into an array


	int i = 0;	
	while (fgets(buf, MAX_STRLEN, file)) {
		char *line_end = strchr(buf, '\n');
		if (line_end != NULL) {
			*line_end = '\0';
		}
		
		if (iflag == 1) {
			int_val[i] = atoi(buf);
		} else if (dflag == 1) {
			dbl_val[i] = atof(buf);
		} else {
			str_val[i] = strdup(buf);
		}
		i++;
	}

	
	// Quicksorting and printing the array
	if(iflag == 1) {
		quicksort(int_val, (size_t)i, sizeof(int), *int_cmp);
		for(int j = 0; j < i; j++) {
			printf("%d\n", int_val[j]);
		}
		
	} else if (dflag == 1) {
		quicksort(dbl_val, (size_t)i, sizeof(double), *dbl_cmp);
		for(int j = 0; j < i; j++) {
                        printf("%f\n", dbl_val[j]);
                }
	} else {
		quicksort(str_val, (size_t)i, sizeof(char *), *str_cmp);
		for(int j = 0; j < i; j++) {
                        printf("%s\n", str_val[j]);
                }
	}
	
	fclose(file);
	
	return EXIT_SUCCESS;


}



