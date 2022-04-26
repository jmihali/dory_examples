#ifndef UTILS_H
#define UTILS_H

% if verbose_level == 'Check_all+Perf_final':
#ifdef VERBOSE
// check for input/output acitvation checksum
static void check_layer(char *output, int check_sum_true, int dim) {
 int checksum = 0;
 char *ptr = (char *) output;
 for(int j=0; j<dim; j++) {
   checksum += ptr[j];
 }

 if(check_sum_true == checksum)
   printf("Checksum in/out Layer :\tOk\n");
 else
   printf("Checksum in/out Layer :\tFailed [%u vs. %u]\n", checksum, check_sum_true);
}

static void check_layer_last(int *output, int check_sum_true, int dim) {
 int checksum = 0;
 int *ptr = (int *) output;
 for(int j=0; j<(int)(dim/4); j++) {
   checksum += ptr[j];
 }

 if(check_sum_true == checksum)
   printf("Checksum final :\tOk\n");
 else
   printf("Checksum final :\tFailed [%d vs. %d]\n", checksum, check_sum_true);
}

// check for weight checksum
static void check_layer_weight(char *weight, int check_sum_true, int dim) {
 int checksum = 0;
 char *ptr = (char *) weight;
 for(int j=0; j<dim; j++) {
   checksum += ptr[j];
 }

 if(check_sum_true == checksum)
   printf("Checksum weight/bias Layer :\tOk\n");
 else
   printf("Checksum weight/bias Layer :\tFailed [%u vs. %u]\n", checksum, check_sum_true);
}
#endif
% endif

% if 'Last' in verbose_level:
static void check_layer_last(int *output, int check_sum_true, int dim) {
 int checksum = 0;
 int *ptr = (int *) output;
 for(int j=0; j<(int)(dim/4); j++) {
   checksum += ptr[j];
 }

 if(check_sum_true == checksum)
   printf("Checksum final :\tOk\n");
 else
   printf("Checksum final :\tFailed [%d vs. %d]\n", checksum, check_sum_true);
}
% endif

#endif