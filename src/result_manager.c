#include "result_manager.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

char file_name[256];

// generate csv file name based on current date and time
static void _init_file_name(void) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(file_name, "results/results_%d-%02d-%02d_%02d-%02d-%02d.csv", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void result_manager_init(void) {
    if (file_name[0] == '\0') {
        _init_file_name();
    }
}

// Saves the result of the analysis to a csv file
void save_result(const char *algo_name, char *cnf_str, char *graph_str, size_t result, ssize_t max_memory_kb,
                 size_t time_ms) {
    result_manager_init();

    FILE *file = fopen(file_name, "a");

    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", file_name);
        exit(EXIT_FAILURE);
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fprintf(stderr, "Error checking file %s\n", file_name);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    if (file_size == 0) {
        fprintf(file, "algo_name,cnf,graph,result,max_memory_kb,time_ms\n");
    }

    fprintf(file, "\"%s\",\"%s\",\"%s\",%zu,%zd,%zu\n", algo_name, cnf_str, graph_str, result, max_memory_kb, time_ms);
    fclose(file);
}
