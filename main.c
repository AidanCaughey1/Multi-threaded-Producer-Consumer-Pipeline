#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LINE_LENGTH 1000
#define BUFFER_SIZE 50
#define OUTPUT_LINE_LENGTH 80
#define STOP "STOP\n"

//Buffer structure 
typedef struct {
    char lines[BUFFER_SIZE][MAX_LINE_LENGTH];  
    int count;  
    int in;     
    int out;    
} Buffer;

Buffer buffer1, buffer2, buffer3; 
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;  
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;  
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;     
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;     
pthread_cond_t cond3 = PTHREAD_COND_INITIALIZER;     

//Reads lines from standard input and places them into Buffer 1
void *input_thread(void *arg) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, stdin)) {
        pthread_mutex_lock(&mutex1);  
        //Wait if buffer1 is full
        while (buffer1.count == BUFFER_SIZE) {  
            pthread_cond_wait(&cond1, &mutex1);
        }
        //Copy the input line to buffer1
        strcpy(buffer1.lines[buffer1.in], line);  
        buffer1.in = (buffer1.in + 1) % BUFFER_SIZE;  
        //Increment the count of lines in buffer1
        buffer1.count++;  
        pthread_cond_signal(&cond1);  
        pthread_mutex_unlock(&mutex1);  
        //Check for the stop-processing line
        if (strcmp(line, STOP) == 0) {  
            break;
        }
    }
    return NULL;
}

//Replaces newline characters with spaces and moves the data to Buffer 2
void *line_separator_thread(void *arg) {
    char line[MAX_LINE_LENGTH];
    while (1) {
        pthread_mutex_lock(&mutex1);  
        while (buffer1.count == 0) {  
            pthread_cond_wait(&cond1, &mutex1);
        }
        strcpy(line, buffer1.lines[buffer1.out]);  
        buffer1.out = (buffer1.out + 1) % BUFFER_SIZE;  
        buffer1.count--;  
        //Signal that buffer1 is not full
        pthread_cond_signal(&cond1);  
        pthread_mutex_unlock(&mutex1);  
        //Check for the stop-processing line
        if (strcmp(line, STOP) == 0) {  
            pthread_mutex_lock(&mutex2); 
            //Pass the stop-processing line to buffer2
            strcpy(buffer2.lines[buffer2.in], STOP);  
            buffer2.in = (buffer2.in + 1) % BUFFER_SIZE;  
            buffer2.count++;  
            pthread_cond_signal(&cond2);  
            pthread_mutex_unlock(&mutex2);  
            break;
        }

        //Replace newline 
        for (int i = 0; i < strlen(line); i++) {
            if (line[i] == '\n') {
                line[i] = ' ';
            }
        }

        pthread_mutex_lock(&mutex2);  
        while (buffer2.count == BUFFER_SIZE) {  
            pthread_cond_wait(&cond2, &mutex2);
        }
        //Copy the modified line to buffer2
        strcpy(buffer2.lines[buffer2.in], line);  
        buffer2.in = (buffer2.in + 1) % BUFFER_SIZE;  
        buffer2.count++;  
        pthread_cond_signal(&cond2);  
        pthread_mutex_unlock(&mutex2);  
    }
    return NULL;
}

//Replaces ++ with ^ and places the processed data into Buffer 3
void *plus_sign_thread(void *arg) {
    char line[MAX_LINE_LENGTH];
    while (1) {
        pthread_mutex_lock(&mutex2);  
        while (buffer2.count == 0) {  
            pthread_cond_wait(&cond2, &mutex2);
        }
        //Copy a line from buffer2
        strcpy(line, buffer2.lines[buffer2.out]);  
        buffer2.out = (buffer2.out + 1) % BUFFER_SIZE;  
        buffer2.count--;  
        pthread_cond_signal(&cond2);  
        pthread_mutex_unlock(&mutex2);  
        //Check for the stop-processing line
        if (strcmp(line, STOP) == 0) {  
            pthread_mutex_lock(&mutex3);  
            strcpy(buffer3.lines[buffer3.in], STOP);  
            buffer3.in = (buffer3.in + 1) % BUFFER_SIZE;  
            buffer3.count++;  
            pthread_cond_signal(&cond3);  
            pthread_mutex_unlock(&mutex3); 
            break;
        }

        //Replace ++ with ^
        for (int i = 0; i < strlen(line) - 1; i++) {
            if (line[i] == '+' && line[i + 1] == '+') {
                line[i] = '^';
                //Shift the characters left
                memmove(&line[i + 1], &line[i + 2], strlen(line) - i - 1);  
                //And null-terminate the string
                line[strlen(line) - 1] = '\0';  
            }
        }

        pthread_mutex_lock(&mutex3); 
        while (buffer3.count == BUFFER_SIZE) {  
            pthread_cond_wait(&cond3, &mutex3);
        }
        strcpy(buffer3.lines[buffer3.in], line);  
        buffer3.in = (buffer3.in + 1) % BUFFER_SIZE; 
        buffer3.count++;  
        pthread_cond_signal(&cond3);  
        pthread_mutex_unlock(&mutex3);  
    }
    return NULL;
}

//Takes data from Buffer 3 and writes lines of exactly 80 characters to standard output
void *output_thread(void *arg) {
    char line[MAX_LINE_LENGTH];
    char output_line[OUTPUT_LINE_LENGTH + 1];
    int output_index = 0;

    while (1) {
        pthread_mutex_lock(&mutex3);  
        while (buffer3.count == 0) {  
            pthread_cond_wait(&cond3, &mutex3);
        }
        strcpy(line, buffer3.lines[buffer3.out]);  
        buffer3.out = (buffer3.out + 1) % BUFFER_SIZE;  
        buffer3.count--;  
        pthread_cond_signal(&cond3);  
        pthread_mutex_unlock(&mutex3);  
        //Check for the stop-processing line
        if (strcmp(line, STOP) == 0) {  
            break;
        }

        //Collect characters to form lines of exactly 80 characters
        for (int i = 0; i < strlen(line); i++) {
            output_line[output_index++] = line[i];
            if (output_index == OUTPUT_LINE_LENGTH) {  
                output_line[OUTPUT_LINE_LENGTH] = '\0';
                //Print the output line
                printf("%s\n", output_line);  
                output_index = 0;
            }
        }
    }

    //Print any remaining characters as the final line
    if (output_index > 0) {
        output_line[output_index] = '\0';
        printf("%s\n", output_line);
    }

    return NULL;
}

//Main
int main() {
    pthread_t input_tid, line_separator_tid, plus_sign_tid, output_tid;

    //Initialize buffer
    buffer1.count = buffer1.in = buffer1.out = 0;
    buffer2.count = buffer2.in = buffer2.out = 0;
    buffer3.count = buffer3.in = buffer3.out = 0;

    //Create threads
    pthread_create(&input_tid, NULL, input_thread, NULL);
    pthread_create(&line_separator_tid, NULL, line_separator_thread, NULL);
    pthread_create(&plus_sign_tid, NULL, plus_sign_thread, NULL);
    pthread_create(&output_tid, NULL, output_thread, NULL);

    //Join threads
    pthread_join(input_tid, NULL);
    pthread_join(line_separator_tid, NULL);
    pthread_join(plus_sign_tid, NULL);
    pthread_join(output_tid, NULL);

    return 0;
}
