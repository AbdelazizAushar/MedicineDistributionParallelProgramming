#ifndef INPUT_H
#define INPUT_H


typedef enum {
    POINT_PHARMACY = 0,
    POINT_CLINIC = 1,
    POINT_HOSPITAL = 2
} DistributionPointType;


typedef struct {
    int pharmacies;  
    int clinics;       
    int hospitals;     
} ProvinceDistributionPoints;


typedef struct {
    ProvinceDistributionPoints points; 
    int num_distributors;              
} ProvinceInfo;

typedef struct {
    int num_provinces;              
    int average_distribution_time;   
    ProvinceInfo provinces[10];    
} SystemInput;


void read_user_input(SystemInput* input_data);


void print_input_summary(const SystemInput* input_data);

#endif
