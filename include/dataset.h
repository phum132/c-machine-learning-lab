#ifndef DATASET_H
#define DATASET_H

#include <stddef.h>

#include "matrix.h"

typedef struct MLDataset MLDataset;

typedef struct {
    /* Nonzero when the first record contains column names. */
    int has_header;
    /* Target index in the original CSV columns, before feature projection. */
    size_t target_column;
} MLDatasetCsvOptions;

/* Load a bounded numeric CSV. On success, *out owns the dataset. On every
 * failure, *out is NULL. Dataset getters return borrowed, read-only data. */
MLStatus ml_dataset_load_csv(const char *path, const MLDatasetCsvOptions *options,
                             MLDataset **out);
void ml_dataset_destroy(MLDataset *dataset);

size_t ml_dataset_sample_count(const MLDataset *dataset);
size_t ml_dataset_feature_count(const MLDataset *dataset);
size_t ml_dataset_column_count(const MLDataset *dataset);
size_t ml_dataset_target_column(const MLDataset *dataset);
const MLMatrix *ml_dataset_features(const MLDataset *dataset);
const double *ml_dataset_targets(const MLDataset *dataset);
/* Column index refers to original CSV order; NULL means no header/invalid index. */
const char *ml_dataset_column_name(const MLDataset *dataset, size_t column_index);

#endif
