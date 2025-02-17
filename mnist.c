#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "include/mnist_file.h"
#include "include/neural_network.h"

#define STEPS 100
// #define STEPS 1000
#define BATCH_SIZE 100
#define LEARNING_RATE 0.5f

// Downloaded from: http://yann.lecun.com/exdb/mnist/
const char * train_images_file = "data/train-images-idx3-ubyte";
const char * train_labels_file = "data/train-labels-idx1-ubyte";
const char * test_images_file = "data/t10k-images-idx3-ubyte";
const char * test_labels_file = "data/t10k-labels-idx1-ubyte";

// Calculates wall-time in seconds
double wtime( void )
{
    double wtime;
    struct timespec tstruct;

    clock_gettime(CLOCK_MONOTONIC, &tstruct);

    wtime = (double) tstruct.tv_nsec/1.0e+9;
    wtime += (double) tstruct.tv_sec;

    return wtime;
}


// Calculate the accuracy of the predictions of a neural network on a dataset.
float calculate_accuracy(mnist_dataset_t * dataset, neural_network_t * network)
{
    float activations[MNIST_LABELS], max_activation;
    int i, j, correct, predict;

#pragma omp parallel for num_threads(NUM_THREADS) collapse(2)
    // Loop through the dataset
    for (i = 0, correct = 0; i < dataset->size; i++) {
        // Calculate the activations for each image using the neural network
        neural_network_hypothesis(&dataset->images[i], network, activations);
        // Set predict to the index of the greatest activation
        for (j = 0, predict = 0, max_activation = activations[0]; j < MNIST_LABELS; j++) {
            if (max_activation < activations[j]) {
                max_activation = activations[j];
                predict = j;
            }
        }

        // Increment the correct count if we predicted the right label
        if (predict == dataset->labels[i]) {
            correct++;
        }
    }

    // Return the percentage we predicted correctly as the accuracy
    return ((float) correct) / ((float) dataset->size);
}


int main(int argc, char *argv[])
{
    mnist_dataset_t* train_dataset;
    mnist_dataset_t* test_dataset;
    mnist_dataset_t batch;
    neural_network_t network;
    float loss, accuracy;
    int i, batches;
    double elapsed, elapsed1, elapsed2, elapsed3;

    // Read the datasets from the files
    elapsed = wtime();
    train_dataset = mnist_get_dataset(train_images_file, train_labels_file);
    test_dataset = mnist_get_dataset(test_images_file, test_labels_file);
    printf("mnist_get_dataset - Time elapsed = %g seconds.\n", wtime() - elapsed);

    // Initialise weights and biases with random values
    elapsed = wtime();
    neural_network_random_weights(&network);
    printf("neural_network_random_weights - Time elapsed = %g seconds.\n", wtime() - elapsed);

    // Calculate how many batches (so we know when to wrap around)
    batches = train_dataset->size / BATCH_SIZE;

    // Train and test model in batches
    for (i = 0; i < STEPS; i++) {
        elapsed = wtime();

        // Initialize a new batch
        elapsed1 = wtime();
        mnist_batch(train_dataset, &batch, BATCH_SIZE, i % batches);
        elapsed1 = wtime() - elapsed1;

        // Run one step of gradient descent and calculate the loss
        elapsed2 = wtime();
        loss = neural_network_training_step(&batch, &network, LEARNING_RATE);
        elapsed2 = wtime() - elapsed2;

        // Calculate the accuracy using the whole test dataset
        elapsed3 = wtime();
        accuracy = calculate_accuracy(test_dataset, &network);
        elapsed3 = wtime() - elapsed3;

        printf("Step %04d\tAverage Loss: %.2f\tAccuracy: %.3f\t\tstep_time: %.3f\tbatch_time: %.3f\ttrain_time: %.3f\ttest_time: %.3f\n", i, loss / batch.size, accuracy, wtime() - elapsed, elapsed1, elapsed2, elapsed3);
    }

    // Cleanup
    elapsed = wtime();
    mnist_free_dataset(train_dataset);
    mnist_free_dataset(test_dataset);
    printf("mnist_free_dataset - Time elapsed = %g seconds.\n", wtime() - elapsed);

    return 0;
}
