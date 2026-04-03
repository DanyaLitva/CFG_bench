#include "config.h"
#include <GraphBLAS.h>
#include <LAGraph.h>
#include <LAGraphX.h>
#include <string.h>
#include <parser.h>
#include <time.h>
#include <stdio.h>

#define run_algorithm()                                                        \
  LAGraph_CFL_single_path(outputs, &single_path_t, adj_matrices, grammar.terms_count,\
                             grammar.nonterms_count, grammar.rules,            \
                             grammar.rules_count, msg)

#define check_error(error)                                                     \
    {                                                                          \
        retval = run_algorithm();                                              \
        TEST_CHECK(retval == error);                                           \
        TEST_MSG("retval = %d (%s)", retval, msg);                             \
    }

#define check_result(result)                                                   \
    {                                                                          \
        char *expected = output_to_str(0);                                     \
        TEST_CHECK(strcmp(result, expected) == 0);                             \
        TEST_MSG("Wrong result. Actual: %s", expected);                        \
    }

GrB_Matrix *adj_matrices = NULL;
GrB_Matrix *outputs = NULL;
GrB_Type single_path_t = NULL;
grammar_t grammar = {0, 0, 0, NULL};
char msg[LAGRAPH_MSG_LEN];

void setup() { LAGr_Init(GrB_NONBLOCKING, malloc, NULL, NULL, free, msg); }

void teardown(void) { LAGraph_Finalize(msg); }

void init_outputs() {
    outputs = calloc(grammar.nonterms_count, sizeof(GrB_Matrix));
}

void free_outputs() {
    for (size_t i = 0; i < grammar.nonterms_count; i++) {
        if (outputs == NULL)
            break;

        if (outputs[i] == NULL)
            continue;

       
        // free_AllPaths_matrix(&outputs[i]);
        GrB_free(&outputs[i]);
    }
    free(outputs);
    GrB_free(&single_path_t);
    outputs = NULL;
}

void free_workspace() {
    for (size_t i = 0; i < grammar.terms_count; i++) {
        if (adj_matrices == NULL)
            break;

        if (adj_matrices[i] == NULL)
            continue;

        GrB_free(&adj_matrices[i]);
    }
    free(adj_matrices);
    adj_matrices = NULL;

    free_outputs();
    free(grammar.rules);
    grammar = (grammar_t){0, 0, 0, NULL};
}

int main(int argc, char **argv) {
    printf("LAGraph_CFL_single_path:\n");
    setup();
    GrB_Info retval;

    FILE *outfile = fopen("results_single_path.txt", "w+");
    fprintf(outfile, "# graph_name min_time nnz all_nnz ratio\n");

    // char *config = argv[1];
    printf("Start bench\n");
    fflush(stdout);
    int config_index = 0;
    char *config = configs[config_index];
    while (config) {
        printf("CONFIG: %s\n", config);
        fflush(stdout);
        parser(config, &grammar, &adj_matrices);

        double start[COUNT];
        double end[COUNT];

        if (HOT) {
            init_outputs();
            run_algorithm();
            free_outputs();
        }

        GrB_Index nnz = 0;
        for (size_t i = 0; i < COUNT; i++) {
            init_outputs();

            start[i] = LAGraph_WallClockTime();
#ifndef CI
            retval = run_algorithm();
#endif
            end[i] = LAGraph_WallClockTime();

            nnz = 0;            
            for(size_t j = 0; j < grammar.nonterms_count; j++){
                GrB_Index temp = 0;
                GrB_Matrix_nvals(&temp, outputs[j]);
                nnz+=temp;
            }

            printf("\t%.3fs", end[i] - start[i]);
            fflush(stdout);
            free_outputs();
        }
        printf("\n");

        // printf("retval = %d (%s)\n", retval, msg);
        // double sum = 0;
        // for (size_t i = 0; i < COUNT; i++) {
        //     sum += end[i] - start[i];
        // }
        double min_time = end[0] - start[0];
        for (size_t i = 0; i < COUNT; i++) {
            double temp = end[i] - start[i];
            if(temp<min_time) min_time = temp;
        }

        printf("\tTime elapsed (min): %.6f seconds. Result: %lu (return code "
               "%d) (%s)\n\n",
               min_time, nnz, retval, msg);
        // GxB_print(outputs[0], 1);

        char graph_name[256] = "";
        char config_copy[1024];
        strcpy(config_copy, config);
        char *graph_path = strtok(config_copy, ",");
        if (graph_path) {
            char *last_slash = strrchr(graph_path, '/');
            char *base = last_slash ? last_slash + 1 : graph_path;
            char *dot = strchr(base, '.');
            if (dot) *dot = '\0';
            strcpy(graph_name, base);
        }

        // double avg_time = sum / COUNT;
        fprintf(outfile, "%s %.6f %lu %lu 1.0\n", graph_name, min_time, nnz, nnz);

        free_workspace();
        config = configs[++config_index];
        fflush(stdout);
    }

    fclose(outfile);
    teardown();
}
