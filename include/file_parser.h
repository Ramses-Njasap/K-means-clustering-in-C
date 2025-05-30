#ifndef FILE_PARSER_H
#define FILE_PARSER_H

#include "structs.h"

Vector *read_vectors(const char *filename, int *num_vectors, Metadata *metadata);
void free_vectors(Vector *vectors, int num_vectors);
void free_metadata(Metadata *metadata);
void analyze_metadata(Metadata *metadata, char **preprocess_method, int *target_dim);
Vector *preprocess_vectors(Vector *vectors, int num_vectors, const char *method, int target_dim);
void normalize_vectors(Vector *vectors, int num_vectors, int target_dim);
void perform_kmeans(Vector *vectors, int num_vectors, int target_dim, int k, int *cluster_assignments, float **centroids, float *wcss);

#endif