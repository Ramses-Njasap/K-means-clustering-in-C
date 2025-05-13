#include <stdio.h>
#include <stdlib.h>
#include "file_parser.h"

int main() {
    int num_vectors;
    Metadata metadata = {0};
    Vector *vectors = read_vectors("vectors.txt", &num_vectors, &metadata);

    if (!vectors) {
        printf("Failed to read vectors.txt\n");
        return 1;
    }

    // Print metadata
    printf("Total vectors: %d\n", metadata.total_vectors);
    printf("Min dimension: %d\n", metadata.min_dim);
    printf("Max dimension: %d\n", metadata.max_dim);
    printf("Length counts:\n");
    for (int i = metadata.min_dim; i <= metadata.max_dim; i++) {
        if (metadata.length_counts[i] > 0)
            printf("  %dD: %d (%.2f%%)\n", i, metadata.length_counts[i],
                   100.0 * metadata.length_counts[i] / metadata.total_vectors);
    }

    // Decide preprocessing
    char *preprocess_method;
    int target_dim;
    analyze_metadata(&metadata, &preprocess_method, &target_dim);
    printf("Recommended preprocessing: %s\n", preprocess_method);
    printf("Target dimension: %d\n", target_dim);

    // Preprocess vectors
    Vector *processed_vectors = preprocess_vectors(vectors, num_vectors, preprocess_method, target_dim);
    if (!processed_vectors) {
        printf("Preprocessing failed\n");
        free_vectors(vectors, num_vectors);
        free_metadata(&metadata);
        free(preprocess_method);
        return 1;
    }

    // Print sample of preprocessed vectors (first 5)
    printf("\nSample preprocessed vectors (first 5):\n");
    for (int i = 0; i < num_vectors && i < 5; i++) {
        printf("Vector %d: [", i + 1);
        for (int j = 0; j < processed_vectors[i].length; j++) {
            printf("%.2f", processed_vectors[i].values[j]);
            if (j < processed_vectors[i].length - 1) printf(", ");
        }
        printf("]\n");
    }

    // Cleanup
    free_vectors(vectors, num_vectors);
    free_vectors(processed_vectors, num_vectors);
    free_metadata(&metadata);
    free(preprocess_method);

    return 0;
}