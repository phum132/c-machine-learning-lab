CC = cc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow
BUILD_DIR := build
CLI := $(BUILD_DIR)/cml-lab.exe
FOUNDATION_TESTS := $(BUILD_DIR)/test-foundation.exe
COMMON_SOURCES := src/ml_error.c src/app_cli.c src/app_commands.c
MATRIX_SOURCE := src/matrix.c
ML_SOURCES := src/linear_regression.c
DATASET_SOURCE := src/dataset.c
CLASSIFICATION_SOURCES := src/logistic_regression.c src/classification_metrics.c src/regression_metrics.c src/cross_validation.c
SCALING_SOURCE := src/feature_scaling.c
SERIALIZATION_SOURCE := src/model_serialization.c

.PHONY: all test

ifeq ($(OS),Windows_NT)

all:
	cmd /c build.bat

test:
	cmd /c build.bat test

else

all: $(CLI)

$(BUILD_DIR):
	python -c "from pathlib import Path; Path('$(BUILD_DIR)').mkdir(parents=True, exist_ok=True)"

$(CLI): main.c $(COMMON_SOURCES) $(MATRIX_SOURCE) $(ML_SOURCES) $(DATASET_SOURCE) $(CLASSIFICATION_SOURCES) $(SERIALIZATION_SOURCE) $(SCALING_SOURCE) include/ml_numeric.h include/feature_scaling.h include/regression_metrics.h include/cross_validation.h include/app_cli.h include/app_commands.h include/ml_error.h include/matrix.h include/linear_regression.h include/dataset.h include/logistic_regression.h include/classification_metrics.h include/model_serialization.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) main.c $(COMMON_SOURCES) $(MATRIX_SOURCE) $(ML_SOURCES) $(DATASET_SOURCE) $(CLASSIFICATION_SOURCES) $(SERIALIZATION_SOURCE) $(SCALING_SOURCE) $(LDFLAGS) -lm -o $@

$(FOUNDATION_TESTS): tests/test_main.c tests/test_ml_error.c tests/test_app_cli.c tests/test_matrix.c tests/test_linear_regression.c tests/test_dataset.c tests/test_logistic_regression.c tests/test_classification_metrics.c tests/test_model_serialization.c tests/test_feature_scaling.c tests/test_regression_metrics.c tests/test_cross_validation.c tests/test_regularization.c $(COMMON_SOURCES) $(MATRIX_SOURCE) $(ML_SOURCES) $(DATASET_SOURCE) $(CLASSIFICATION_SOURCES) $(SERIALIZATION_SOURCE) $(SCALING_SOURCE) include/ml_numeric.h include/feature_scaling.h include/regression_metrics.h include/cross_validation.h include/app_cli.h include/app_commands.h include/ml_error.h include/matrix.h include/linear_regression.h include/dataset.h include/logistic_regression.h include/classification_metrics.h include/model_serialization.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_main.c tests/test_ml_error.c tests/test_app_cli.c tests/test_matrix.c tests/test_linear_regression.c tests/test_dataset.c tests/test_logistic_regression.c tests/test_classification_metrics.c tests/test_model_serialization.c tests/test_feature_scaling.c tests/test_regression_metrics.c tests/test_cross_validation.c tests/test_regularization.c $(COMMON_SOURCES) $(MATRIX_SOURCE) $(ML_SOURCES) $(DATASET_SOURCE) $(CLASSIFICATION_SOURCES) $(SERIALIZATION_SOURCE) $(SCALING_SOURCE) $(LDFLAGS) -lm -o $@

test: $(CLI) $(FOUNDATION_TESTS)
	$(FOUNDATION_TESTS)
	python scripts/test_cli.py $(CLI)
	python scripts/validate_infrastructure.py

.PHONY: test-analyzer test-sanitize benchmark

test-analyzer:
	$(MAKE) BUILD_DIR=$(BUILD_DIR)/analyzer CFLAGS="$(CFLAGS) -fanalyzer" test

test-sanitize:
	$(MAKE) BUILD_DIR=$(BUILD_DIR)/sanitize CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer" LDFLAGS="-fsanitize=address,undefined" test

benchmark: $(CLI)
	python scripts/benchmark_cli.py --exe $(CLI)

endif
