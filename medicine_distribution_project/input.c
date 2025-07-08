// input.c
// Implementation of functions for reading user input

#include <stdio.h>
#include "input.h"

// Function to read user input
void read_user_input(SystemInput* input_data) {
    int i;

    printf("Enter the number of provinces: ");
    scanf("%d", &input_data->num_provinces);

    // Check for maximum limit
    if (input_data->num_provinces > 10) {
        printf("Maximum number of provinces is 10. Only the first 10 will be used.\n");
        input_data->num_provinces = 10;
    }

    for (i = 0; i < input_data->num_provinces; i++) {
        printf("Province %d:\n", i + 1);

        printf("Number of pharmacies: ");
        scanf("%d", &input_data->provinces[i].points.pharmacies);

        printf("Number of clinics: ");
        scanf("%d", &input_data->provinces[i].points.clinics);

        printf("Number of hospitals: ");
        scanf("%d", &input_data->provinces[i].points.hospitals);

        printf("Number of distributors: ");
        scanf("%d", &input_data->provinces[i].num_distributors);
    }

    printf("Enter the average time to distribute a single order (in seconds): ");
    scanf("%d", &input_data->average_distribution_time);
}

// Function to print a summary of inputs (for verification)
void print_input_summary(const SystemInput* input_data) {
    int i;
    int total_requests;

    printf("\nInput Summary:\n");
    printf("Number of provinces: %d\n", input_data->num_provinces);
    printf("Average distribution time: %d seconds\n", input_data->average_distribution_time);

    for (i = 0; i < input_data->num_provinces; i++) {
        total_requests = input_data->provinces[i].points.pharmacies
                       + input_data->provinces[i].points.clinics
                       + input_data->provinces[i].points.hospitals;

        printf(" - Province %d: Pharmacies=%d, Clinics=%d, Hospitals=%d, Total Requests=%d, Distributors=%d\n", 
            i + 1,
            input_data->provinces[i].points.pharmacies,
            input_data->provinces[i].points.clinics,
            input_data->provinces[i].points.hospitals,
            total_requests,
            input_data->provinces[i].num_distributors);
    }
}
