#pragma once

#include <stdlib.h>

// Initialize the output filename before forking per-config workers.
void result_manager_init(void);

// Saves the result of the analysis to a csv file
void save_result(const char *algo_name, char *cnf_str, char *graph_str, size_t result, ssize_t max_memory_kb,
                 size_t time_ms);
