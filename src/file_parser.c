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
        *target_dim = metadata->max_dim; // Pad to max (e.g., 7D)
    } else if (range > 3 || metadata->length_counts[metadata->max_dim] < 0.05 * metadata->total_vectors || sequence_like) {
        *preprocess_method = strdup("reduce_dims");
        *target_dim = (dominant_dim >= 2) ? dominant_dim : 2; // Dynamic: dominant_dim, min 2D
    } else {
        *preprocess_method = strdup("hybrid");
        *target_dim = dominant_dim; // Hybrid: dominant_dim (e.g., 2D or 3D)
    }
}

Vector *preprocess_vectors(Vector *vectors, int num_vectors, const char *method, int target_dim) {
    Vector *new_vectors = malloc(num_vectors * sizeof(Vector));
    if (!new_vectors) {
        printf("Memory allocation failed\n");
        return NULL;
    }

    for (int i = 0; i < num_vectors; i++) {
        new_vectors[i].length = target_dim;
        new_vectors[i].values = malloc(target_dim * sizeof(float));
        if (!new_vectors[i].values) {
            printf("Memory allocation failed\n");
            for (int j = 0; j < i; j++) free(new_vectors[j].values);
            free(new_vectors);
            return NULL;
        }

        if (strcmp(method, "reduce_dims") == 0) {
            // Compute statistics for reduction
            float sum = 0.0, sum_sq = 0.0, max_val = -1e9;
            for (int j = 0; j < vectors[i].length; j++) {
                sum += vectors[i].values[j];
                sum_sq += vectors[i].values[j] * vectors[i].values[j];
                if (vectors[i].values[j] > max_val) max_val = vectors[i].values[j];
            }
            float mean = sum / vectors[i].length;
            float variance = (sum_sq / vectors[i].length) - (mean * mean);

            // Assign features based on target_dim
            if (target_dim >= 1) new_vectors[i].values[0] = mean;
            if (target_dim >= 2) new_vectors[i].values[1] = sum;
            if (target_dim >= 3) new_vectors[i].values[2] = variance;
            if (target_dim >= 4) new_vectors[i].values[3] = max_val;
            for (int j = 4; j < target_dim; j++) new_vectors[i].values[j] = 0.0; // Pad rest
        } else if (strcmp(method, "pad_zeros") == 0) {
            // Pad with zeros to target_dim
            for (int j = 0; j < target_dim; j++) {
                new_vectors[i].values[j] = (j < vectors[i].length) ? vectors[i].values[j] : 0.0;
            }
        } else if (strcmp(method, "hybrid") == 0) {
            // Hybrid: Reduce if longer, pad if shorter
            if (vectors[i].length >= target_dim) {
                // Reduce: Take first target_dim values
                for (int j = 0; j < target_dim; j++) {
                    new_vectors[i].values[j] = vectors[i].values[j];
                }
            } else {
                // Pad: Copy values, then pad with zeros
                for (int j = 0; j < target_dim; j++) {
                    new_vectors[i].values[j] = (j < vectors[i].length) ? vectors[i].values[j] : 0.0;
                }
            }
        }
    }

    return new_vectors;
}