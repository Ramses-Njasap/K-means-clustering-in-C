#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_parser.h"

#define MAX_LINE 1024 // Max line length
#define MAX_DIM 100   // Max vector dimension

Vector *read_vectors(const char *filename, int *num_vectors, Metadata *metadata) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening %s\n", filename);
        return NULL;
    }

    // Initialize metadata
    metadata->total_vectors = 0;
    metadata->min_dim = MAX_DIM;
    metadata->max_dim = 0;
    metadata->length_counts = calloc(MAX_DIM, sizeof(int));
    metadata->value_mins = malloc(MAX_DIM * sizeof(float));
    metadata->value_maxes = malloc(MAX_DIM * sizeof(float));
    for (int i = 0; i < MAX_DIM; i++) {
        metadata->value_mins[i] = 1e9;
        metadata->value_maxes[i] = -1e9;
    }

    // Temporary storage for vectors
    Vector *vectors = NULL;
    int capacity = 100;
    vectors = malloc(capacity * sizeof(Vector));
    char line[MAX_LINE];

    while (fgets(line, MAX_LINE, file)) {
        // Parse vector
        Vector vec = { .values = NULL, .length = 0 };
        char *token = strtok(line, ",");
        while (token) {
            vec.values = realloc(vec.values, (vec.length + 1) * sizeof(float));
            vec.values[vec.length] = atof(token);
            vec.length++;
            token = strtok(NULL, ",");
        }

        if (vec.length == 0) continue; // Skip empty lines

        // Update metadata
        metadata->total_vectors++;
        if (vec.length < metadata->min_dim) metadata->min_dim = vec.length;
        if (vec.length > metadata->max_dim) metadata->max_dim = vec.length;
        metadata->length_counts[vec.length]++;

        for (int i = 0; i < vec.length; i++) {
            if (vec.values[i] < metadata->value_mins[i])
                metadata->value_mins[i] = vec.values[i];
            if (vec.values[i] > metadata->value_maxes[i])
                metadata->value_maxes[i] = vec.values[i];
        }

        // Store vector
        if (metadata->total_vectors > capacity) {
            capacity *= 2;
            vectors = realloc(vectors, capacity * sizeof(Vector));
        }
        vectors[metadata->total_vectors - 1] = vec;
    }

    fclose(file);
    *num_vectors = metadata->total_vectors;
    return vectors;
}

void free_vectors(Vector *vectors, int num_vectors) {
    for (int i = 0; i < num_vectors; i++) {
        free(vectors[i].values);
    }
    free(vectors);
}

void free_metadata(Metadata *metadata) {
    free(metadata->length_counts);
    free(metadata->value_mins);
    free(metadata->value_maxes);
}

void analyze_metadata(Metadata *metadata, char **preprocess_method, int *target_dim) {
    // Compute range and dominant lengths
    int range = metadata->max_dim - metadata->min_dim;
    int dominant_count = 0;
    int dominant_dim = metadata->min_dim;
    for (int i = metadata->min_dim; i <= metadata->max_dim; i++) {
        if (metadata->length_counts[i] > dominant_count) {
            dominant_count = metadata->length_counts[i];
            dominant_dim = i;
        }
    }
    float dominant_pct = (float)dominant_count / metadata->total_vectors;
    float similar_pct = 0;
    for (int i = metadata->min_dim; i <= metadata->max_dim; i++) {
        if (abs(i - dominant_dim) <= 1)
            similar_pct += (float)metadata->length_counts[i] / metadata->total_vectors;
    }

    // Check for sequence-like patterns (basic heuristic)
    int sequence_like = 0;
    for (int i = 0; i < metadata->max_dim - 1; i++) {
        if (metadata->value_mins[i] < metadata->value_mins[i + 1] &&
            metadata->value_maxes[i] < metadata->value_maxes[i + 1]) {
            sequence_like = 1;
            break;
        }
    }

    // Decision rules
    if (range <= 3 && similar_pct > 0.7 && dominant_pct > 0.3 && !sequence_like && metadata->total_vectors > 50) {
        *preprocess_method = strdup("pad_zeros");
        *target_dim = metadata->max_dim; // Pad to max (e.g., 5D)
    } else if (range > 3 || metadata->length_counts[metadata->max_dim] < 0.05 * metadata->total_vectors || sequence_like) {
        *preprocess_method = strdup("reduce_dims");
        *target_dim = 2; // Reduce to [mean, sum]
    } else {
        *preprocess_method = strdup("hybrid");
        *target_dim = dominant_dim; // Pad to dominant (e.g., 3D or 4D)
    }
}