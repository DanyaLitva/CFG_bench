#include <GraphBLAS.h>
#include <LAGraph.h>
#include <LAGraphX.h>
#include <string.h>
#include <ctype.h>
#include <parser.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    ALGO_REACHABILITY = 0,
    ALGO_ALL_PATHS = 1,
    ALGO_SINGLE_PATH = 2
} bench_algo_t;

//default values for env vars
const bench_algo_t default_algo = ALGO_REACHABILITY;
int default_count = 1;
bool default_hot = false;
const char* default_config = "data/configs/vf.txt";

typedef struct {
    char **items;
    size_t count;
} config_list_t;

GrB_Matrix *adj_matrices = NULL;
GrB_Matrix *outputs = NULL;
GrB_Type algo_elem_type = NULL;
grammar_t grammar = {0, 0, 0, NULL};
char msg[LAGRAPH_MSG_LEN];

void setup() { LAGr_Init(GrB_NONBLOCKING, malloc, NULL, NULL, free, msg); }

void teardown(void) { LAGraph_Finalize(msg); }
static bench_algo_t g_algo = default_algo;

void init_outputs() {
    outputs = calloc(grammar.nonterms_count, sizeof(GrB_Matrix));
}

static int env_int(const char *name, int def) {
    const char *s = getenv(name);
    if (s == NULL || *s == '\0') {
        return def;
    }
    return atoi(s);
}

static bool env_bool(const char *name, bool def) {
    const char *s = getenv(name);
    if (s == NULL || *s == '\0') {
        return def;
    }
    return strcmp(s, "1") == 0 || strcmp(s, "true") == 0 ||
           strcmp(s, "yes") == 0 || strcmp(s, "on") == 0;
}

static bench_algo_t parse_algo(const char *s) {
    if (s == NULL || *s == '\0') {
        return default_algo;
    }

    if (strcmp(s, "reachability") == 0 || strcmp(s, "r") == 0) {
        return ALGO_REACHABILITY;
    }

    if (strcmp(s, "allpaths") == 0 || strcmp(s, "ap") == 0) {
        return ALGO_ALL_PATHS;
    }

    if (strcmp(s, "singlepath") == 0 || strcmp(s, "sp") == 0) {
        return ALGO_SINGLE_PATH;
    }

    return default_algo;
}

static const char *algo_name(bench_algo_t algo) {
    switch (algo) {
    case ALGO_REACHABILITY:
        return "reachability";
    case ALGO_ALL_PATHS:
        return "all paths";
    case ALGO_SINGLE_PATH:
        return "single path";
    default:
        return "unknown";
    }
}

static const char *default_outfile(bench_algo_t algo) {
    switch (algo) {
    case ALGO_REACHABILITY:
        return "results_reachability.txt";
    case ALGO_ALL_PATHS:
        return "results_all_paths.txt";
    case ALGO_SINGLE_PATH:
        return "results_single_path.txt";
    default:
        return "results.txt";
    }
}

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) {
        ++s;
    }

    if (*s == '\0') {
        return s;
    }

    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end-- = '\0';
    }

    return s;
}

static void config_list_push(config_list_t *list, const char *line) {
    list->items = realloc(list->items, sizeof(char *) * (list->count + 1));
    list->items[list->count] = strdup(line);
    list->count++;
}

static config_list_t load_config_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "[ERROR] Cannot open config file: %s\n",
                path);
        exit(EXIT_FAILURE);
    }

    config_list_t list = {0};

    char buf[4096];
    while (fgets(buf, sizeof(buf), f) != NULL) {
        char *line = trim(buf);

        if (*line == '\0' || *line == '#') {
            continue;
        }

        char *comment = strchr(line, '#');
        if (comment != NULL) {
            *comment = '\0';
            line = trim(line);
            if (*line == '\0') {
                continue;
            }
        }

        config_list_push(&list, line);
    }

    fclose(f);

    if (list.count == 0) {
        fprintf(stderr, "[ERROR] Empty config file: %s\n", path);
        exit(EXIT_FAILURE);
    }

    return list;
}

static void free_config_list(config_list_t *list) {
    for (size_t i = 0; i < list->count; ++i) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

static GrB_Info get_nvals_all_paths(GrB_Index *nvals, const GrB_Matrix A) {
    GrB_Index accum = 0;
    GxB_Iterator iterator;
    GxB_Iterator_new(&iterator);

    GrB_Info info = GxB_Matrix_Iterator_attach(iterator, A, NULL);
    if (info != GrB_SUCCESS) {
        GrB_free(&iterator);
        *nvals = 0;
        return info;
    }

    info = GxB_Matrix_Iterator_seek(iterator, 0);

    AllPathsElem val;
    while (info != GxB_EXHAUSTED) {
        GxB_Iterator_get_UDT(iterator, (void *) &val);
        accum += val.n;
        info = GxB_Matrix_Iterator_next(iterator);
    }

    GrB_free(&iterator);
    *nvals = accum;
    return GrB_SUCCESS;
}

static void free_allpaths_matrix(GrB_Matrix *ptr_output) {
    if (ptr_output == NULL || *ptr_output == NULL) {
        return;
    }

    GrB_Matrix output = *ptr_output;

    GxB_Iterator iterator;
    GxB_Iterator_new(&iterator);

    GrB_Info info = GxB_Matrix_Iterator_attach(iterator, output, NULL);
    if (info == GrB_SUCCESS) {
        info = GxB_Matrix_Iterator_seek(iterator, 0);

        AllPathsElem val;
        while (info != GxB_EXHAUSTED) {
            GxB_Iterator_get_UDT(iterator, (void *) &val);
            if (val.middle != NULL) {
                free(val.middle);
            }
            info = GxB_Matrix_Iterator_next(iterator);
        }
    }

    GrB_free(&iterator);
    GrB_free(ptr_output);
}

static void free_outputs(void) {
    if (outputs == NULL) {
        return;
    }

    for (size_t i = 0; i < grammar.nonterms_count; ++i) {
        if (outputs[i] == NULL) {
            continue;
        }

        if (g_algo == ALGO_ALL_PATHS) {
            free_allpaths_matrix(&outputs[i]);
        } else {
            GrB_free(&outputs[i]);
        }
    }

    free(outputs);
    outputs = NULL;
}

static void free_algo_state(void) {
    if (g_algo == ALGO_ALL_PATHS || g_algo == ALGO_SINGLE_PATH) {
        GrB_free(&algo_elem_type);
    }
}

static void free_workspace(void) {
    for (size_t i = 0; i < grammar.terms_count; ++i) {
        if (adj_matrices == NULL) {
            break;
        }
        if (adj_matrices[i] == NULL) {
            continue;
        }
        GrB_free(&adj_matrices[i]);
    }

    free(adj_matrices);
    adj_matrices = NULL;

    free_outputs();
    free_algo_state();

    free(grammar.rules);
    grammar = (grammar_t){0, 0, 0, NULL};
}

static GrB_Info run_algorithm_impl(void) {
    switch (g_algo) {
    case ALGO_REACHABILITY:
        return LAGraph_CFL_reachability(outputs, adj_matrices,
                                        grammar.terms_count,
                                        grammar.nonterms_count,
                                        grammar.rules,
                                        grammar.rules_count, msg);

    case ALGO_ALL_PATHS:
        return LAGraph_CFL_AllPaths(outputs, adj_matrices, &algo_elem_type,
                                    grammar.terms_count,
                                    grammar.nonterms_count,
                                    grammar.rules,
                                    grammar.rules_count, msg);

    case ALGO_SINGLE_PATH:
        return LAGraph_CFL_single_path(outputs, &algo_elem_type, adj_matrices,
                                       grammar.terms_count,
                                       grammar.nonterms_count,
                                       grammar.rules,
                                       grammar.rules_count, msg);

    default:
        return GrB_INVALID_VALUE;
    }
}

static void collect_metrics(GrB_Index *nnz, size_t *count_nnz) {
    *nnz = 0;
    *count_nnz = 0;

    for (size_t j = 0; j < grammar.nonterms_count; ++j) {
        GrB_Index temp = 0;
        GrB_Matrix_nvals(&temp, outputs[j]);
        *nnz += temp;

        if (g_algo == ALGO_ALL_PATHS) {
            GrB_Index all_temp = 0;
            get_nvals_all_paths(&all_temp, outputs[j]);
            *count_nnz += all_temp;
        }
    }

    if (g_algo != ALGO_ALL_PATHS) {
        *count_nnz = (size_t) *nnz;
    }
}

static void write_summary(FILE *outfile, const char *graph_name,
                          double algo_time, GrB_Index nnz, size_t count_nnz,
                          GrB_Info retval) {
    if (g_algo == ALGO_ALL_PATHS) {
        double ratio = (nnz > 0) ? (double) count_nnz / (double) nnz : 0.0;
        fprintf(outfile, "%s %.6f %lu %lu %lf\n", graph_name, algo_time,
                (unsigned long) nnz, (unsigned long) count_nnz, ratio);
        printf("\tTime: %.6f seconds. Result: %lu, All nnz: %lu, "
               "All/nvals = %lf (return code %d) (%s)\n\n",
               algo_time, (unsigned long) nnz, (unsigned long) count_nnz,
               ratio, retval, msg);
        return;
    }

    fprintf(outfile, "%s %.6f %lu %lu 1.0\n", graph_name, algo_time,
            (unsigned long) nnz, (unsigned long) count_nnz);

    printf("\tTime: %.6f seconds. Result: %lu (return code %d) "
           "(%s)\n\n",
           algo_time, (unsigned long) nnz, retval, msg);
}

static void graph_name_from_config(const char *config, char *out,
                                   size_t out_size) {
    char copy[1024];
    snprintf(copy, sizeof(copy), "%s", config);

    char *graph_path = strtok(copy, ",");
    if (graph_path == NULL) {
        snprintf(out, out_size, "unknown");
        return;
    }

    char *last_slash = strrchr(graph_path, '/');
    char *base = last_slash ? last_slash + 1 : graph_path;
    char *dot = strchr(base, '.');
    if (dot != NULL) {
        *dot = '\0';
    }

    snprintf(out, out_size, "%s", base);
}

int main(void) {
    setup();

    g_algo = parse_algo(getenv("BENCH_ALGO"));
    int count = env_int("BENCH_COUNT", default_count);
    bool hot = env_bool("BENCH_HOT", default_hot);

    const char *config_file = getenv("BENCH_CONFIG_FILE");
    if (config_file == NULL || *config_file == '\0') {
        config_file = default_config;
    }

    const char *outfile_name_env = getenv("BENCH_OUTFILE");
    const char *outfile_name =
        (outfile_name_env != NULL && *outfile_name_env != '\0')
            ? outfile_name_env
            : default_outfile(g_algo);

    config_list_t configs = load_config_file(config_file);

    FILE *outfile = fopen(outfile_name, "w+");

    fprintf(outfile, "# graph_name time nvals nnz ratio\n");

    printf("Algo %s:\n", algo_name(g_algo));
    printf("Start bench\n");
    fflush(stdout);

    GrB_Info retval = GrB_SUCCESS;

    for (size_t config_index = 0; config_index < configs.count;
         ++config_index) {
        char *config = configs.items[config_index];

        printf("CONFIG: %s\n", config);
        fflush(stdout);

        parser(config, &grammar, &adj_matrices);

        double *start = calloc((size_t) count, sizeof(double));
        double *end = calloc((size_t) count, sizeof(double));

        if (hot) {
            init_outputs();
            run_algorithm_impl();
            free_outputs();
            free_algo_state();
        }

        GrB_Index nnz = 0;
        size_t count_nnz = 0;

        for (int i = 0; i < count; ++i) {
            init_outputs();

            start[i] = LAGraph_WallClockTime();
#ifndef CI
            retval = run_algorithm_impl();
#endif
            end[i] = LAGraph_WallClockTime();

            collect_metrics(&nnz, &count_nnz);

            printf("\t%.3fs", end[i] - start[i]);
            fflush(stdout);
            free_outputs();
            free_algo_state();
        }
        printf("\n");

        double min_time = end[0] - start[0];
        for (int i = 1; i < count; ++i) {
            double i_time = end[i] - start[i];
            if (i_time < min_time) {
                min_time = i_time;
            }
        }
        // double sum = 0.0;
        // for (int i = 0; i < count; ++i) {
        //     sum += end[i] - start[i];
        // }

        char graph_name[256] = {0};
        graph_name_from_config(config, graph_name, sizeof(graph_name));

        write_summary(outfile, graph_name, min_time, nnz, count_nnz,
                      retval);

        free(start);
        free(end);
        free_workspace();
    }

    fclose(outfile);
    free_config_list(&configs);
    teardown();
}
