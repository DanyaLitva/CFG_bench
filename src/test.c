#include "adapter_CFL_adv.h"
#include "computed_cache.h"
#include "memory.h"
#include "parser.h"
#include "registry.h"
#include "result_manager.h"
#include <GraphBLAS.h>
#include <LAGraph.h>
#include <LAGraphX.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <malloc.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define run_algorithm()                                                                                                \
    LAGraph_CFL_reachability_adv(outputs, adj_matrices, symbols_amount, grammar.rules, grammar.rules_count, msg,       \
                                 optimizations)

#define check_error(error)                                                                                             \
    {                                                                                                                  \
        retval = run_algorithm();                                                                                      \
        TEST_CHECK(retval == error);                                                                                   \
        TEST_MSG("retval = %d (%s)", retval, msg);                                                                     \
    }

#define check_result(result)                                                                                           \
    {                                                                                                                  \
        char *expected = output_to_str(0);                                                                             \
        TEST_CHECK(strcmp(result, expected) == 0);                                                                     \
        TEST_MSG("Wrong result. Actual: %s", expected);                                                                \
    }

#define TRY(GrB_method)                                                                                                \
    {                                                                                                                  \
        GrB_Info LG_GrB_Info = GrB_method;                                                                             \
        if (LG_GrB_Info < GrB_SUCCESS) {                                                                               \
            fprintf(stderr, "LAGraph failure (file %s, line %d): (%d) \n", __FILE__, __LINE__, LG_GrB_Info);           \
            return (LG_GrB_Info);                                                                                      \
        }                                                                                                              \
    }

GrB_Matrix *adj_matrices = NULL;
GrB_Matrix *outputs = NULL;
grammar_t grammar = {0, 0, NULL};
char msg[LAGRAPH_MSG_LEN];
size_t symbols_amount = 0;

void print_rules(Grammar grammar, SymbolList list) {
    for (size_t i = 0; i < grammar.rules_count; i++) {
        Rule rule = grammar.rules[i];
        if (rule.first != -1) {
            printf("%s ->", list.symbols[rule.first].label);
        }
        if (rule.second != -1) {
            printf(" %s", list.symbols[rule.second].label);
        }
        if (rule.third != -1) {
            printf(" %s", list.symbols[rule.third].label);
        }
        printf("\n");
    }
}

void print_list(SymbolList list, size_t *map) {
    if (map == NULL) {
        for (size_t i = 0; i < list.count; i++) {
            Symbol sym = list.symbols[i];
            printf("[%2ld] %s [%s] %s\n", i, sym.label, sym.is_nonterm ? "N" : "T", sym.is_indexed ? "[I]" : "");
        }
    } else {
        for (size_t i = 0; i < list.count; i++) {
            Symbol sym = list.symbols[i];
            printf("[%2ld] %s [%s] %s\n", map[i], sym.label, sym.is_nonterm ? "N" : "T", sym.is_indexed ? "[I]" : "");
        }
    }
}

#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

// Use your custom configuration for the benchmark (default is the xz.g graph
// and vf.cnf grammar)
#define configs_macro configs_java

#define OPT_EMPTY (1 << 0)
#define OPT_FORMAT (1 << 1)
#define OPT_LAZY (1 << 2)
#define OPT_BLOCK (1 << 3)

enum {
    HOT_OPTION = 1000,
    BENCH_PARSE_OPTION = 1001,
    CFL_ALL_PATH_USE_CFPQ = 1002,
    USE_START_NODES_OPTION = 1003,
    COMPUTE_RESULTS_OPTION = 1004,
    TIMEOUT_OPTION = 1005
};

static bool timeout_worker = false;
static size_t timeout_worker_config = 0;
static int timeout_status_fd = -1;
static AdapterRun timed_run_target;
static unsigned int timed_run_seconds;
static double timed_run_start;
static double timed_run_end;

static void timeout_handler(int signal_number) {
    (void)signal_number;
    static const char message[] = "Graph execution exceeded --timeout\n";
    static const char marker = 'T';
    (void)write(STDERR_FILENO, message, sizeof(message) - 1);
    if (timeout_status_fd >= 0) {
        (void)write(timeout_status_fd, &marker, sizeof(marker));
    }
    _exit(124);
}

static GrB_Info run_with_timeout(void) {
    alarm(timed_run_seconds);
    timed_run_start = LAGraph_WallClockTime();
    GrB_Info result = timed_run_target();
    timed_run_end = LAGraph_WallClockTime();
    alarm(0);
    return result;
}

static unsigned int parse_timeout(const char *value) {
    char *end = NULL;
    errno = 0;
    unsigned long timeout = strtoul(value, &end, 10);
    if (value[0] == '-' || errno != 0 || end == value || *end != '\0' || timeout == 0 || timeout > UINT_MAX) {
        fprintf(stderr, "--timeout must be a positive integer number of seconds\n");
        exit(EXIT_FAILURE);
    }
    return (unsigned int)timeout;
}

static void print_usage(const char *program_name) {
    fprintf(stderr,
            "Usage: %s -c <config file> [options]\n"
            "\n"
            "Required:\n"
            "  -c <config file>  Path to benchmark config file\n"
            "\n"
            "Benchmark options:\n"
            "  -r <rounds>       Number of benchmark rounds (default: 10)\n"
            "  --hot             Enable HOT launch (warm-up run before measurements)\n"
            "  --bench-parse     Print grammar and graph parsing times only\n"
            "  --use-start-nodes Use start vertices from the path specified in the config\n"
            "  --timeout <seconds> Skip a graph if an algorithm run exceeds this limit\n"
            "  -a <algorithm>    Algorithm to use (default: " DEFAULT_ALGORITHM "; options: ",
            program_name);
    registry_print_names(stderr);
    fprintf(stderr,
            ")\n"
            "\n"
            "Optimization flags:\n"
            "  -e                Enable empty optimization\n"
            "  -f                Enable format optimization\n"
            "  -l                Enable lazy optimization\n"
            "  -b                Enable block optimization\n"
            "\n"
            "Other:\n"
            "  -t                Enable test mode: run each config once and check the result\n"
            "                    (-r and --hot are ignored)\n"
            "  --compute-results Check the result against CFL_adv -efbl on the same data instead of\n"
            "                    the config value (only with -t); CFL_multsrc and CFL_CFPQ_RSM need it,\n"
            "                    values are cached in .cache when run from the project root\n"
            "  -h                Print this help message\n"
            "  --CFL-all-path-use-CFPQ-Core      Use CFPQ_Core in CFL_all_path algorithm\n"
            "\n"
            "Example:\n"
            "  %s -c configs/configs_my.csv -r 10 --hot\n",
            program_name);
}

// runs CFL_adv with all optimizations on the same data and returns the result "algo" must have
static GrB_Info compute_expected_result(const ParserResult *parser_result, const AlgorithmEntry *algo,
                                        bool use_start_nodes, size_t *expected, unsigned int timeout_seconds) {
    AdapterMethods reference = adapter_CFL_adv_get_methods();
    TRY(reference.prepare(parser_result,
                          &(CFL_adv_PrepareData){.optimizations = OPT_EMPTY | OPT_FORMAT | OPT_LAZY | OPT_BLOCK}));
    TRY(reference.init_outputs());
    if (timeout_seconds != 0) {
        alarm(timeout_seconds);
    }
    GrB_Info run_result = reference.run();
    if (timeout_seconds != 0) {
        alarm(0);
    }
    TRY(run_result);

    if (algo->is_multiple_source) {
        if (!use_start_nodes) {
            TRY(adapter_CFL_adv_count_reachable(NULL, 0, expected));
        } else if (parser_result->start_nodes_count == 0) {
            *expected = 0;
        } else {
            TRY(adapter_CFL_adv_count_reachable(parser_result->start_nodes, parser_result->start_nodes_count,
                                                expected));
        }
    } else {
        *expected = reference.get_result();
    }

    TRY(reference.free_outputs());
    TRY(reference.cleanup());
    return GrB_SUCCESS;
}

int main(int argc, char **argv) {
    GrB_Info retval = GrB_SUCCESS;
    int8_t optimizations = 0;
    int opt;
    bool is_test = false;
    bool has_test_failure = false;
    bool is_hot_enabled = false;
    bool is_bench_parse_enabled = false;
    bool use_start_nodes = false;
    bool compute_results = false;
    bool is_config = false;
    const AlgorithmEntry *algo = NULL;
    char *input_config = NULL;
    size_t rounds_count = 10;
    bool use_cfpq = false;
    unsigned int timeout_seconds = 0;

    AdapterMethods adapter = {0};

    static struct option long_options[] = {
        {"hot", no_argument, 0, HOT_OPTION},
        {"bench-parse", no_argument, 0, BENCH_PARSE_OPTION},
        {"use-start-nodes", no_argument, 0, USE_START_NODES_OPTION},
        {"CFL-all-path-use-CFPQ-Core", no_argument, 0, CFL_ALL_PATH_USE_CFPQ},
        {"compute-results", no_argument, 0, COMPUTE_RESULTS_OPTION},
        {"timeout", required_argument, 0, TIMEOUT_OPTION},
        {0, 0, 0, 0},
    };

    while ((opt = getopt_long(argc, argv, "eflbthr:c:a:", long_options, NULL)) != -1) {
        switch (opt) {
        case 'e':
            optimizations |= OPT_EMPTY;
            break;
        case 'f':
            optimizations |= OPT_FORMAT;
            break;
        case 'l':
            optimizations |= OPT_LAZY;
            break;
        case 'b':
            optimizations |= OPT_BLOCK;
            break;
        case 'h':
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        case HOT_OPTION:
            is_hot_enabled = true;
            break;
        case BENCH_PARSE_OPTION:
            is_bench_parse_enabled = true;
            break;
        case USE_START_NODES_OPTION:
            use_start_nodes = true;
            break;
        case COMPUTE_RESULTS_OPTION:
            compute_results = true;
            break;
        case TIMEOUT_OPTION:
            timeout_seconds = parse_timeout(optarg);
            printf("Choosen timeout: %u seconds\n", timeout_seconds);
            break;
        case 't':
            is_test = true;
            break;
        case 'r':
            rounds_count = strtoul(optarg, NULL, 10);
            if (rounds_count == 0) {
                fprintf(stderr, "Rounds count must be greater than 0\n");
                exit(EXIT_FAILURE);
            }
            printf("Choosen rounds count: %zu\n", rounds_count);
            break;
        case 'c':
            is_config = true;
            input_config = optarg;
            printf("Choosen config: %s\n", input_config);
            break;
        case 'a':
            printf("Choosen algorithm: %s\n", optarg);

            algo = registry_find(optarg);
            if (algo == NULL) {
                fprintf(stderr, "Unknown algorithm: %s\n", optarg);
                exit(EXIT_FAILURE);
            }
            adapter = algo->get_methods();
            break;
        case CFL_ALL_PATH_USE_CFPQ:
            use_cfpq = true;
            break;
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (compute_results && !is_test) {
        fprintf(stderr, "--compute-results works only with -t\n");
        exit(EXIT_FAILURE);
    }

    if (algo == NULL) {
        algo = registry_find(DEFAULT_ALGORITHM);
        adapter = algo->get_methods();
        printf("No algorithm chosen, using " DEFAULT_ALGORITHM " by default\n");
    }

    if (!is_config) {
        fprintf(stderr, "Need to choose config by flag -c [config file]\n");
        exit(EXIT_FAILURE);
    }

    if (timeout_seconds != 0 && !timeout_worker) {
        result_manager_init();
        size_t configs_count = 0;
        char *config_text;
        config_row *configs = get_configs_from_file(input_config, &configs_count, &config_text);
        bool has_timeout = false;
        int exit_status = EXIT_SUCCESS;

        for (size_t i = 0; i < configs_count; i++) {
            fflush(NULL);
            int timeout_pipe[2];
            if (pipe(timeout_pipe) != 0) {
                perror("Failed to create timeout status pipe");
                exit_status = EXIT_FAILURE;
                break;
            }

            pid_t child = fork();
            if (child < 0) {
                perror("Failed to create graph worker");
                close(timeout_pipe[0]);
                close(timeout_pipe[1]);
                exit_status = EXIT_FAILURE;
                break;
            }
            if (child == 0) {
                close(timeout_pipe[0]);
                timeout_status_fd = timeout_pipe[1];
                timeout_worker = true;
                timeout_worker_config = i;
                break;
            }

            close(timeout_pipe[1]);
            int child_status = 0;
            pid_t waited;
            do {
                waited = waitpid(child, &child_status, 0);
            } while (waited < 0 && errno == EINTR);
            if (waited < 0) {
                perror("Failed to wait for graph worker");
                close(timeout_pipe[0]);
                exit_status = EXIT_FAILURE;
                break;
            }

            char timeout_marker;
            ssize_t marker_size;
            do {
                marker_size = read(timeout_pipe[0], &timeout_marker, sizeof(timeout_marker));
            } while (marker_size < 0 && errno == EINTR);
            close(timeout_pipe[0]);
            if (marker_size < 0) {
                perror("Failed to read graph worker status");
                exit_status = EXIT_FAILURE;
                break;
            }

            if (marker_size == 1 && timeout_marker == 'T') {
                fprintf(stderr, "Skipping graph after timeout: %s\n", configs[i].graph);
                has_timeout = true;
            } else if (!WIFEXITED(child_status) || WEXITSTATUS(child_status) != EXIT_SUCCESS) {
                fprintf(stderr, "Graph worker failed: %s\n", configs[i].graph);
                exit_status = EXIT_FAILURE;
                break;
            }
        }

        free(configs);
        free(config_text);
        if (!timeout_worker) {
            return has_timeout && exit_status == EXIT_SUCCESS ? EXIT_FAILURE : exit_status;
        }
    }

    if (timeout_seconds != 0) {
        struct sigaction timeout_action = {0};
        timeout_action.sa_handler = timeout_handler;
        sigemptyset(&timeout_action.sa_mask);
        if (sigaction(SIGALRM, &timeout_action, NULL) != 0) {
            perror("Failed to configure timeout handler");
            exit(EXIT_FAILURE);
        }
    }

    AlgorithmOptions algo_options = {
        .optimizations = optimizations,
        .use_start_nodes = use_start_nodes,
        .use_cfpq = use_cfpq,
    };

    if (is_test && !compute_results && algo->is_multiple_source) {
        fprintf(stderr,
                YELLOW "Warning: %s ignores the expected result from the config, "
                       "use --compute-results to check the result" RESET "\n",
                algo->name);
    }

    TRY(adapter.setup());

    size_t configs_count = 0;
    char *config_text;
    config_row *configs = get_configs_from_file(input_config, &configs_count, &config_text);

    printf("Start bench\n");
    fflush(stdout);

    for (size_t i = 0; i < configs_count; i++) {
        if (timeout_worker && i != timeout_worker_config) {
            continue;
        }
        config_row config = configs[i];
        printf("CONFIG: grammar: %s, graph: %s\n", config.grammar, config.graph);
        fflush(stdout);

        ParserResult parser_result = parser(config, is_bench_parse_enabled);
        if (is_bench_parse_enabled) {
            free_parser_result(&parser_result);
            continue;
        }

        double *start = calloc(rounds_count, sizeof(double));
        double *end = calloc(rounds_count, sizeof(double));
        if (start == NULL || end == NULL) {
            fprintf(stderr, "Failed to allocate memory for benchmark rounds\n");
            free(start);
            free(end);
            exit(EXIT_FAILURE);
        }

        // computed before the tested algorithm: parser_result is freed after prepare and
        // the CFL_adv adapter state can't be shared with a tested CFL_adv run
        size_t expected_result = config.valid_result;
        bool is_cached = false;
        if (compute_results) {
            const char *kind = "pairs";
            const char *start_nodes = NULL;
            if (algo->is_multiple_source) {
                kind = use_start_nodes ? "start_vertices" : "vertices";
                start_nodes = use_start_nodes ? config.start_nodes_path : NULL;
            }

            is_cached = computed_cache_get(kind, config.graph, config.grammar, start_nodes, &expected_result);
            if (!is_cached) {
                TRY(compute_expected_result(&parser_result, algo, use_start_nodes, &expected_result, timeout_seconds));
                computed_cache_put(kind, config.graph, config.grammar, start_nodes, expected_result);
            }
        }

        algo->prepare(&adapter, &parser_result, &algo_options);
        free_parser_result(&parser_result);

        bool is_hot = is_hot_enabled;

#ifndef CI
        if (timeout_seconds != 0) {
            timed_run_target = adapter.run;
            timed_run_seconds = timeout_seconds;
        }
#endif

        size_t result = 0;
        ssize_t max_memory_kb = 0;
        for (size_t j = 0; j < rounds_count; j++) {
            TRY(adapter.init_outputs());

            // in some cases free don't change memory usage, so we need to reset it manually
            malloc_trim(0);
            if (mem_peak_reset() != 0) {
                fprintf(stderr, "Failed to reset memory peak\n");
                exit(EXIT_FAILURE);
            }

#ifndef CI
            if (timeout_seconds == 0) {
                start[j] = LAGraph_WallClockTime();
                retval = adapter.run();
                end[j] = LAGraph_WallClockTime();
            } else {
                retval = run_with_timeout();
                start[j] = timed_run_start;
                end[j] = timed_run_end;
            }
#else
            start[j] = LAGraph_WallClockTime();
#endif
#ifdef CI
            end[j] = LAGraph_WallClockTime();
#endif
            max_memory_kb = mem_get_peak_kb();

            if (is_test) {
                size_t result = 0;
                char status[256];
                if (retval != GrB_SUCCESS) {
                    // outputs are not valid after a failed run, so the result is not checked
                    has_test_failure = true;
                    snprintf(status, sizeof(status), RED "[Failed]" RESET);
                } else {
                    result = adapter.get_result();
                    ResultType result_type = RESULT_UNKNOWN;
                    if (compute_results) {
                        result_type = result == expected_result ? RESULT_OK : RESULT_ERROR;
                    } else {
                        result_type = adapter.is_result_valid(config.valid_result);
                    }
                    switch (result_type) {
                    case RESULT_OK:
                        snprintf(status, sizeof(status), GREEN "[OK]" RESET);
                        break;
                    case RESULT_ERROR:
                        has_test_failure = true;
                        snprintf(status, sizeof(status), RED "[Wrong] (Result must be %zu)" RESET, expected_result);
                        break;
                    case RESULT_UNKNOWN:
                        snprintf(status, sizeof(status), YELLOW "[Unknown]" RESET);
                        break;
                    default:
                        fprintf(stderr, "Unknown result type: %d\n", result_type);
                        abort();
                    }
                }

                printf("\tResult: %ld (Return code: %d) %s", result, retval, status);

                if (compute_results) {
                    printf(" (%s: %zu)", is_cached ? "Cached" : "Computed", expected_result);
                }

                if (retval != 0) {
                    printf("\t(MSG: %s)", msg);
                }
                printf(" (%.4f sec)", end[j] - start[j]);

                TRY(adapter.free_outputs());
                break;
            }

            if (is_hot) {
                is_hot = false;
                j--;
                TRY(adapter.free_outputs());
                continue;
            }

            printf("\t%.3fs", end[j] - start[j]);
            fflush(stdout);

            result = adapter.get_result();
            TRY(adapter.free_outputs());
            save_result(algo->name, config.grammar, config.graph, result, max_memory_kb,
                        (size_t)((end[j] - start[j]) * 1000));
            // in some cases free don't change memory usage, so we need to reset it manually
            malloc_trim(0);
        }
        printf("\n");

        if (is_test) {
            free(start);
            free(end);
            adapter.cleanup();

            fflush(stdout);
            continue;
        }

        double sum = 0;
        for (size_t j = 0; j < rounds_count; j++) {
            sum += end[j] - start[j];
        }
        printf("\tTime elapsed (avg): %.6f seconds. %zd KB max memory. Result: %ld (return code "
               "%d) (%s)\n\n",
               sum / rounds_count, max_memory_kb, result, retval, msg);

        free(start);
        free(end);

        TRY(adapter.cleanup());

        fflush(stdout);
    }

    free(configs);
    free(config_text);
    TRY(adapter.teardown());
    return has_test_failure ? EXIT_FAILURE : EXIT_SUCCESS;
}
