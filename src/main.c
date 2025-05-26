#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "file_parser.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MIN_X -2.5
#define MAX_X 2.5
#define MIN_Y -2.5
#define MAX_Y 2.5
#define POINT_RADIUS 4

// Initialize SDL and TTF
int init_sdl_ttf(SDL_Window** window, SDL_Renderer** renderer, TTF_Font** font) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 0;
    }
    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }
    *window = SDL_CreateWindow("K-Means Clustering Visualization (k = 4)",
                               SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!*window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
    if (!*font) {
        printf("Font loading failed: %s\n", TTF_GetError());
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }
    return 1;
}

// Draw a filled circle
void draw_circle(SDL_Renderer* renderer, int center_x, int center_y, int radius) {
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            if (dx * dx + dy * dy <= radius * radius) {
                SDL_RenderDrawPoint(renderer, center_x + dx, center_y + dy);
            }
        }
    }
}

// Draw text
void draw_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) {
        printf("Text rendering failed: %s\n", TTF_GetError());
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        printf("Texture creation failed: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// Draw scatter plot with axes, legend, and labels
void draw_scatter(SDL_Renderer* renderer, TTF_Font* font, float *x, float *y, int *labels, int n, float *centroid_x, float *centroid_y, int k) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White background
    SDL_RenderClear(renderer);

    // Define plotting area (leave space for legend and labels)
    int plot_left = 80;
    int plot_right = WINDOW_WIDTH - 20;
    int plot_top = 40;
    int plot_bottom = WINDOW_HEIGHT - 40;
    int plot_width = plot_right - plot_left;
    int plot_height = plot_bottom - plot_top;

    // Draw axes
    int zero_x = (int)((0.0 - MIN_X) / (MAX_X - MIN_X) * plot_width + plot_left);
    int zero_y = (int)((MAX_Y - 0.0) / (MAX_Y - MIN_Y) * plot_height + plot_top);
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Gray axes
    SDL_RenderDrawLine(renderer, plot_left, zero_y, plot_right, zero_y); // X-axis
    SDL_RenderDrawLine(renderer, zero_x, plot_top, zero_x, plot_bottom); // Y-axis

    // Draw axis labels
    SDL_Color black = {0, 0, 0, 255};
    draw_text(renderer, font, "Standardized Mean", (plot_left + plot_right) / 2 - 50, plot_bottom + 10, black);
    draw_text(renderer, font, "Standardized Sum", plot_left - 70, (plot_top + plot_bottom) / 2 - 20, black);

    // Draw title
    draw_text(renderer, font, "K-Means Clustering Results (k = 4)", WINDOW_WIDTH / 2 - 100, 10, black);

    // Draw vectors
    int points_drawn = 0;
    for (int i = 0; i < n; i++) {
        if (labels[i] >= 0 && labels[i] < 4) { // Ensure valid label
            int screen_x = (int)((x[i] - MIN_X) / (MAX_X - MIN_X) * plot_width + plot_left);
            int screen_y = (int)((MAX_Y - y[i]) / (MAX_Y - MIN_Y) * plot_height + plot_top);
            // Clamp coordinates to plot area
            screen_x = screen_x < plot_left ? plot_left : (screen_x > plot_right ? plot_right : screen_x);
            screen_y = screen_y < plot_top ? plot_top : (screen_y > plot_bottom ? plot_bottom : screen_y);

            SDL_Color colors[] = {{0, 0, 255}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0}}; // B, R, G, Y
            SDL_SetRenderDrawColor(renderer, colors[labels[i]].r, colors[labels[i]].g, colors[labels[i]].b, 255);
            draw_circle(renderer, screen_x, screen_y, POINT_RADIUS);
            points_drawn++;
        }
    }
    // printf("Debug: Drew %d points out of %d\n", points_drawn, n);

    // Draw centroids
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
    for (int i = 0; i < k; i++) {
        int screen_x = (int)((centroid_x[i] - MIN_X) / (MAX_X - MIN_X) * plot_width + plot_left);
        int screen_y = (int)((MAX_Y - centroid_y[i]) / (MAX_Y - MIN_Y) * plot_height + plot_top);
        screen_x = screen_x < plot_left ? plot_left : (screen_x > plot_right ? plot_right : screen_x);
        screen_y = screen_y < plot_top ? plot_top : (screen_y > plot_bottom ? plot_bottom : screen_y);
        for (int dx = -6; dx <= 6; dx++) {
            for (int dy = -6; dy <= 6; dy++) {
                if (dx == 0 || dy == 0) { // Larger cross shape
                    SDL_RenderDrawPoint(renderer, screen_x + dx, screen_y + dy);
                }
            }
        }
    }

    // Draw legend
    const char* cluster_names[] = {"Cluster 0 (Blue)", "Cluster 1 (Red)", "Cluster 2 (Green)", "Cluster 3 (Yellow)"};
    SDL_Color colors[] = {{0, 0, 255}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0}};
    for (int i = 0; i < k; i++) {
        int legend_y = 60 + i * 20;
        SDL_SetRenderDrawColor(renderer, colors[i].r, colors[i].g, colors[i].b, 255);
        draw_circle(renderer, 20, legend_y, POINT_RADIUS);
        draw_text(renderer, font, cluster_names[i], 30, legend_y - 5, black);
    }

    SDL_RenderPresent(renderer);
}

int main() {
    int num_vectors;
    Metadata metadata = {0};
    Vector *vectors = read_vectors("vectors.txt", &num_vectors, &metadata);

    if (!vectors) {
        printf("Failed to read vectors.txt\n");
        return 1;
    }

    // Print metadata about the input vectors
    printf("Total vectors: %d\n", metadata.total_vectors);
    printf("Min dimension: %d\n", metadata.min_dim);
    printf("Max dimension: %d\n", metadata.max_dim);
    printf("Length counts:\n");
    for (int i = metadata.min_dim; i <= metadata.max_dim; i++) {
        if (metadata.length_counts[i] > 0)
            printf("  %dD: %d (%.2f%%)\n", i, metadata.length_counts[i],
                   100.0 * metadata.length_counts[i] / metadata.total_vectors);
    }

    // Determine the recommended preprocessing method and target dimension
    char *preprocess_method;
    int target_dim;
    analyze_metadata(&metadata, &preprocess_method, &target_dim);
    printf("Recommended preprocessing: %s\n", preprocess_method);
    printf("Target dimension: %d\n", target_dim);

    // Preprocess and normalize the vectors
    Vector *processed_vectors = preprocess_vectors(vectors, num_vectors, preprocess_method, target_dim);
    if (!processed_vectors) {
        printf("Preprocessing failed\n");
        free_vectors(vectors, num_vectors);
        free_metadata(&metadata);
        free(preprocess_method);
        return 1;
    }
    normalize_vectors(processed_vectors, num_vectors, target_dim);

    // Print sample of standardized vectors
    printf("\nSample standardized vectors (first 5):\n");
    for (int i = 0; i < num_vectors && i < 5; i++) {
        printf("Vector %d: [", i + 1);
        for (int j = 0; j < processed_vectors[i].length; j++) {
            printf("%.2f", processed_vectors[i].values[j]);
            if (j < processed_vectors[i].length - 1) printf(", ");
        }
        printf("]\n");
    }

    // Perform k-means clustering for k = 2 to 5 with random restarts
    srand(time(NULL)); // Seed for random initialization
    #define NUM_RESTARTS 5 // Number of random restarts per k
    for (int k = 2; k <= 5; k++) {
        printf("\n--- Running k-means with k = %d ---\n", k);
        float best_wcss = -1;
        int *best_assignments = NULL;
        float **best_centroids = NULL;

        // Perform multiple restarts to find the best clustering
        for (int run = 0; run < NUM_RESTARTS; run++) {
            int *cluster_assignments = malloc(num_vectors * sizeof(int));
            float **centroids = malloc(k * sizeof(float *));
            for (int i = 0; i < k; i++) centroids[i] = malloc(target_dim * sizeof(float));
            if (!cluster_assignments || !centroids) {
                printf("Memory allocation failed for run %d with k = %d\n", run, k);
                free(cluster_assignments);
                for (int i = 0; i < k; i++) free(centroids[i]);
                free(centroids);
                continue;
            }

            // Run k-means and get WCSS
            float wcss;
            perform_kmeans(processed_vectors, num_vectors, target_dim, k, cluster_assignments, centroids, &wcss);

            // Update best result if this WCSS is lower
            if (best_wcss < 0 || wcss < best_wcss) {
                if (best_assignments) free(best_assignments);
                if (best_centroids) {
                    for (int i = 0; i < k; i++) free(best_centroids[i]);
                    free(best_centroids);
                }
                best_wcss = wcss;
                best_assignments = cluster_assignments;
                best_centroids = centroids;
            } else {
                free(cluster_assignments);
                for (int i = 0; i < k; i++) free(centroids[i]);
                free(centroids);
            }
        }

        // Print best results
        printf("Best Within-Cluster Sum of Squares (WCSS): %.2f\n", best_wcss);
        printf("Cluster assignments (first 5 vectors):\n");
        for (int i = 0; i < num_vectors && i < 5; i++) {
            printf("Vector %d: Cluster %d\n", i + 1, best_assignments[i]);
        }
        printf("Final centroids (standardized):\n");
        for (int i = 0; i < k; i++) {
            printf("Centroid %d: [", i);
            for (int j = 0; j < target_dim; j++) {
                printf("%.2f", best_centroids[i][j]);
                if (j < target_dim - 1) printf(", ");
            }
            printf("]\n");
        }
        printf("Cluster sizes:\n");
        int *cluster_counts = calloc(k, sizeof(int));
        for (int i = 0; i < num_vectors; i++) {
            cluster_counts[best_assignments[i]]++;
        }
        for (int i = 0; i < k; i++) {
            printf("Cluster %d: %d vectors\n", i, cluster_counts[i]);
        }
        free(cluster_counts);

        // Visualize for k = 4
        if (k == 4) {
            float *x = malloc(num_vectors * sizeof(float));
            float *y = malloc(num_vectors * sizeof(float));
            int *labels = malloc(num_vectors * sizeof(int));
            float *centroid_x = malloc(k * sizeof(float));
            float *centroid_y = malloc(k * sizeof(float));

            // Populate arrays for plotting
            for (int i = 0; i < num_vectors; i++) {
                x[i] = processed_vectors[i].values[0]; // Standardized mean
                y[i] = processed_vectors[i].values[1]; // Standardized sum
                labels[i] = best_assignments[i];
            }
            for (int i = 0; i < k; i++) {
                centroid_x[i] = best_centroids[i][0];
                centroid_y[i] = best_centroids[i][1];
            }

            // Initialize SDL and TTF
            SDL_Window* window = NULL;
            SDL_Renderer* renderer = NULL;
            TTF_Font* font = NULL;
            if (init_sdl_ttf(&window, &renderer, &font)) {
                int quit = 0;
                SDL_Event e;
                while (!quit) {
                    while (SDL_PollEvent(&e)) {
                        if (e.type == SDL_QUIT) quit = 1;
                    }
                    draw_scatter(renderer, font, x, y, labels, num_vectors, centroid_x, centroid_y, k);
                    SDL_Delay(10);
                }
                TTF_CloseFont(font);
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
            }
            TTF_Quit();
            SDL_Quit();

            // Cleanup
            free(x);
            free(y);
            free(labels);
            free(centroid_x);
            free(centroid_y);
        }

        // Cleanup best results
        free(best_assignments);
        for (int i = 0; i < k; i++) free(best_centroids[i]);
        free(best_centroids);
    }

    // Cleanup
    free_vectors(processed_vectors, num_vectors);
    free_vectors(vectors, num_vectors);
    free_metadata(&metadata);
    free(preprocess_method);

    return 0;
}