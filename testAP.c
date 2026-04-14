#include "config.h"
#include <GraphBLAS.h>
#include <LAGraph.h>
#include <LAGraphX.h>
#include <string.h>
#include <parser.h>
#include <time.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#define run_algorithm()                                                        \
  LAGraph_CFL_AllPaths(outputs,&all_paths_t, adj_matrices, grammar.terms_count,\
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
GrB_Type all_paths_t = NULL;
grammar_t grammar = {0, 0, 0, NULL};
char msg[LAGRAPH_MSG_LEN];

void setup() { LAGr_Init(GrB_NONBLOCKING, malloc, NULL, NULL, free, msg); }

void teardown(void) { LAGraph_Finalize(msg); }

void init_outputs() {
    outputs = calloc(grammar.nonterms_count, sizeof(GrB_Matrix));
}

//To test the non-reduction approach in the future
GrB_Info get_nvals_all_paths(GrB_Index *nvals, const GrB_Matrix A){
  GrB_Index accum = 0;
  GxB_Iterator iterator;
  GxB_Iterator_new(&iterator);
  GrB_Info info = GxB_Matrix_Iterator_attach(iterator, A, NULL);
  info = GxB_Matrix_Iterator_seek(iterator, 0);
  AllPathsElem val;
  
  while (info != GxB_EXHAUSTED)
  {
    GxB_Iterator_get_UDT(iterator, (void*) &val);
    accum+=val.n;
    info = GxB_Matrix_Iterator_next(iterator);
  }
  
  GrB_free(&iterator);
  *nvals = accum;
  
  return GrB_SUCCESS;
}


//Cleaning of internal elements before free matrix
void free_AllPaths_matrix(GrB_Matrix* ptr_output){
  GrB_Matrix output = *ptr_output;
  GxB_Iterator iterator;
  GxB_Iterator_new(&iterator);
  GrB_Info info = GxB_Matrix_Iterator_attach(iterator, output, NULL);
  info = GxB_Matrix_Iterator_seek(iterator, 0);
  AllPathsElem val;

  while (info != GxB_EXHAUSTED)
  {
    GxB_Iterator_get_UDT(iterator, (void*) &val);
    if (val.n > 1 && val.data.middle != NULL) {
      free(val.data.middle);
    }
    info = GxB_Matrix_Iterator_next(iterator);
  }

  GrB_free(&iterator);
  GrB_free(ptr_output);
}

void free_outputs() {
    for (size_t i = 0; i < grammar.nonterms_count; i++) {
        if (outputs == NULL)
            break;

        if (outputs[i] == NULL)
            continue;

       
        free_AllPaths_matrix(&outputs[i]);
        // GrB_free(&outputs[i]);
    }
    free(outputs);
    GrB_free(&all_paths_t);
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

void print_n_histogram_allpaths(GrB_Matrix *outputs, size_t nonterms_count, FILE *out)
{
    GrB_Index max_n = 0;
    size_t total = 0;
    //max n
    for (size_t k = 0; k < nonterms_count; k++) {
        if (outputs[k] == NULL) continue;
        GxB_Iterator it;
        GxB_Iterator_new(&it);
        GrB_Info info = GxB_Matrix_Iterator_attach(it, outputs[k], NULL);
        if (info != GrB_SUCCESS) {
            GrB_free(&it);
            continue;
        }
        info = GxB_Matrix_Iterator_seek(it, 0);
        while (info != GxB_EXHAUSTED) {
            AllPathsElem val;
            GxB_Iterator_get_UDT(it, (void *)&val);
            if (val.n > max_n) {
                max_n = val.n;
            }
            total++;
            info = GxB_Matrix_Iterator_next(it);
        }
        GrB_free(&it);
    }
    size_t freq_size = (size_t)max_n + 1;
    uint64_t *freq = calloc(freq_size, sizeof(uint64_t));
    //freq n
    for (size_t k = 0; k < nonterms_count; k++) {
        if (outputs[k] == NULL) continue;
        GxB_Iterator it;
        GxB_Iterator_new(&it);
        GrB_Info info = GxB_Matrix_Iterator_attach(it, outputs[k], NULL);
        if (info != GrB_SUCCESS) {
            GrB_free(&it);
            continue;
        }
        info = GxB_Matrix_Iterator_seek(it, 0);
        while (info != GxB_EXHAUSTED) {
            AllPathsElem val;
            GxB_Iterator_get_UDT(it, (void *)&val);
            if ((size_t)val.n < freq_size) {
                freq[(size_t)val.n]++;
            }
            info = GxB_Matrix_Iterator_next(it);
        }
        GrB_free(&it);
    }
    //print freq n
    for (GrB_Index n = 0; n <= max_n; n++) {
        if (freq[(size_t)n] > 0) {
            fprintf(out, "%" PRIu64 " %" PRIu64 "\n",
                    (uint64_t)n, freq[(size_t)n]);
        }
    }
    free(freq);
}

int main(int argc, char **argv) {
    printf("LAGraph_CFL_AllPaths:\n");
    setup();
    GrB_Info retval;

    FILE *outfile = fopen("results_allpaths.txt", "w+");
    fprintf(outfile, "# graph_name avg_time nnz all_nnz ratio\n");

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

        if (HOT) {
            init_outputs();
            run_algorithm();
            free_outputs();
        }

        GrB_Index nnz = 0;
        size_t count_nnz = 0;
        for (size_t i = 0; i < COUNT; i++) {
            init_outputs();

            start[i] = LAGraph_WallClockTime();
#ifndef CI
            retval = run_algorithm();
#endif
            end[i] = LAGraph_WallClockTime();

            nnz = 0;
            count_nnz = 0;

            for (size_t j = 0; j < grammar.nonterms_count; j++) {
                GrB_Index temp = 0;
                get_nvals_all_paths(&temp, outputs[j]);
                count_nnz += temp;
                GrB_Matrix_nvals(&temp, outputs[j]);
                nnz+=temp;
            }
            
            if(i==0 && PRINT_HIST){
                    char hist_name[256];
                    snprintf(hist_name, sizeof(hist_name), "n_hist_%s.txt", graph_name);

                    FILE *hist = fopen(hist_name, "w");
                    if (hist) {
                        fprintf(hist, "# graph: %s\n", graph_name);
                        fprintf(hist, "# n count\n");
                        print_n_histogram_allpaths(outputs, grammar.nonterms_count, hist);
                        fclose(hist);
                        }
            }

            printf("\t%.3fs", end[i] - start[i]);
            fflush(stdout);
            free_outputs();
        }
        printf("\n");

        printf("retval = %d (%s)\n", retval, msg);
        double sum = 0;
        for (size_t i = 0; i < COUNT; i++) {
            sum += end[i] - start[i];
        }
        // double min_time = end[0] - start[0];
        // for (size_t i = 1; i < COUNT; i++) {
        //     double temp = end[i] - start[i];
        //     if(temp<min_time) min_time = temp;
        // }
        
        double avg_time = sum / COUNT;

        printf("\tTime elapsed (avg): %.6f seconds. Result: %lu, All nnz: %lu, All/nvals = %lf (return code "
               "%d) (%s)\n\n",
               avg_time, nnz, count_nnz, ((double)count_nnz/nnz), retval, msg);
        // GxB_print(outputs[0], 1);

        double ratio = (nnz > 0) ? (double)count_nnz / nnz : 0.0;
        fprintf(outfile, "%s %.6f %lu %lu %lf\n",
                graph_name, avg_time, nnz, count_nnz, ratio);

        free_workspace();
        config = configs[++config_index];
        fflush(stdout);
    }
    fclose(outfile);
    teardown();
}
