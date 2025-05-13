#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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

    // Normalize vectors (standardize mean and sum)
    normalize_vectors(processed_vectors, num_vectors, target_dim);

    // Print sample of standardized vectors (first 5)
    printf("\nSample standardized vectors (first 5):\n");
    for (int i = 0; i < num_vectors && i < 5; i++) {
        printf("Vector %d: [", i + 1);
        for (int j = 0; j < processed_vectors[i].length; j++) {
            printf("%.2f", processed_vectors[i].values[j]);
            if (j < processed_vectors[i].length - 1) printf(", ");
        }
        printf("]\n");
    }

    // Perform k-means clustering for k = 2 to 5
    srand(time(NULL)); // Seed for random initialization
    for (int k = 2; k <= 5; k++) {
        printf("\n--- Running k-means with k = %d ---\n", k);

        int *cluster_assignments = malloc(num_vectors * sizeof(int));
        float **centroids = malloc(k * sizeof(float *));
        for (int i = 0; i < k; i++) centroids[i] = malloc(target_dim * sizeof(float));
        if (!cluster_assignments || !centroids) {
            printf("Memory allocation failed for clustering with k = %d\n", k);
            free(cluster_assignments);
            for (int i = 0; i < k; i++) free(centroids[i]);
            free(centroids);
            free_vectors(processed_vectors, num_vectors);
            free_vectors(vectors, num_vectors);
            free_metadata(&metadata);
            free(preprocess_method);
            return 1;
        }

        // Run k-means and get WCSS
        float wcss;
        perform_kmeans(processed_vectors, num_vectors, target_dim, k, cluster_assignments, centroids, &wcss);

        // Print WCSS
        printf("Within-Cluster Sum of Squares (WCSS): %.2f\n", wcss);

        // Print cluster assignments (first 5)
        printf("Cluster assignments (first 5 vectors):\n");
        for (int i = 0; i < num_vectors && i < 5; i++) {
            printf("Vector %d: Cluster %d\n", i + 1, cluster_assignments[i]);
        }

        // Print centroids
        printf("Final centroids (standardized):\n");
        for (int i = 0; i < k; i++) {
            printf("Centroid %d: [", i);
            for (int j = 0; j < target_dim; j++) {
                printf("%.2f", centroids[i][j]);
                if (j < target_dim - 1) printf(", ");
            }
            printf("]\n");
        }

        // Print cluster sizes
        int *cluster_counts = calloc(k, sizeof(int));
        for (int i = 0; i < num_vectors; i++) {
            cluster_counts[cluster_assignments[i]]++;
        }
        printf("Cluster sizes:\n");
        for (int i = 0; i < k; i++) {
            printf("Cluster %d: %d vectors\n", i, cluster_counts[i]);
        }
        free(cluster_counts);

        // Cleanup for this k
        free(cluster_assignments);
        for (int i = 0; i < k; i++) free(centroids[i]);
        free(centroids);
    }

    // Cleanup
    free_vectors(processed_vectors, num_vectors);
    free_vectors(vectors, num_vectors);
    free_metadata(&metadata);
    free(preprocess_method);

    return 0;
}