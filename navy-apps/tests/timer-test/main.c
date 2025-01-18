//通过gettimeofday()获取当前时间, 并每过0.5秒输出一句话
#include <sys/time.h>
#include <stdio.h>

int main() {
    struct timeval tv;
    struct timeval start_time, current_time;
    long elapsed_time;
    int count = 0;
    gettimeofday(&start_time, NULL);
    while (count < 20) {
        gettimeofday(&current_time, NULL);
        elapsed_time = (current_time.tv_sec - start_time.tv_sec) * 1000000 
                       + (current_time.tv_usec - start_time.tv_usec);
        if (elapsed_time >= 500000) {
            start_time = current_time;
            printf("%d \n", count++);
        }
    }
    return 0;
}