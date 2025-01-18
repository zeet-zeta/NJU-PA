#include <NDL.h>
#include <stdio.h>

// int main() {
//     struct timeval tv;
//     struct timeval start_time, current_time;
//     long elapsed_time;
//     int count = 0;
//     gettimeofday(&start_time, NULL);
//     while (count < 20) {
//         gettimeofday(&current_time, NULL);
//         elapsed_time = (current_time.tv_sec - start_time.tv_sec) * 1000000 
//                        + (current_time.tv_usec - start_time.tv_usec);
//         if (elapsed_time >= 500000) {
//             start_time = current_time;
//             printf("%d \n", count++);
//         }
//     }
//     return 0;
// }

int main() {
    NDL_Init(0);
    uint32_t last_time = NDL_GetTicks();
    int count = 0;
    while (count < 20) {
        uint32_t current_time = NDL_GetTicks();
        if (current_time - last_time >= 500) {
            last_time = current_time;
            printf("%d \n", count++);
        }
    }
}