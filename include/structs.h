#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct {
    float *values; // Array of values
    int length;    // Number of values
} Vector;

typedef struct {
    int total_vectors;     // Total vectors read
    int min_dim;           // Smallest dimension
    int max_dim;           // Largest dimension
    int *length_counts;    // Counts per dimension (dynamic array)
    float *value_mins;     // Min value per dimension
    float *value_maxes;    // Max value per dimension
} Metadata;

#endif