#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>


#define TOTAL_DISHES 10 //number of dishes served by restaurant

sem_t mutex;
sem_t mutex2;
int isOpen = 1;
int tipBox = 0;
int inputOver = 0;

struct Queue {
    int front, rear, size;
    unsigned capacity;
    int* array;
};

struct Queue* foodQueue;

struct Customer{
    int id;
    int numDishes;
    int tipAmount;
    int* dishes;
};

struct thread_data{
    int table_id;
    int empty;
    struct Customer* customer;
};

struct thread_data *td;

void createQueue(unsigned capacity) {
    foodQueue = (struct Queue*)malloc(sizeof(struct Queue));
    foodQueue->capacity = capacity;
    foodQueue->size = 0;
    foodQueue->rear = capacity - 1;
    foodQueue->array = (int*)malloc(foodQueue->capacity * sizeof(int));
}

int isFull() {
    return (foodQueue->size == foodQueue->capacity);
}

int isEmpty(){
    return (foodQueue->size == 0);
}

void enqueue(int item) {
    if (isFull())
        return;
    foodQueue->rear = (foodQueue->rear + 1) % foodQueue->capacity;
    foodQueue->array[foodQueue->rear] = item;
    foodQueue->size = foodQueue->size + 1;
}

int dequeue(){
    if (isEmpty())
        return INT_MIN;
    int item = foodQueue->array[foodQueue->front];
    foodQueue->front = (foodQueue->front + 1) % foodQueue->capacity;
    foodQueue->size = foodQueue->size - 1;
    return item;
}

int front() {
    if (isEmpty())
        return INT_MIN;
    return foodQueue->array[foodQueue->front];
}

int randomGenerator(int lower, int upper){
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}

void *threadWork(void *threadarg){
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = lrand48() % 5000 + 1;
    while(inputOver==0){

        int index = *((int *)threadarg);

        //waiting state, till main assigns a customer
        while(td[index].empty==1){
            //thread needs to quit as no customer was assigned and no more remaining custmomers to be assigned
            //will forever remain in waiting state if not taken care of
            if(inputOver==1){
                pthread_exit(NULL);
            }
        }

        while(td[index].customer->numDishes > 0){
            sem_wait(&mutex);
            if(isOpen==1 && isEmpty()==0 && td[index].customer->dishes[front()]>0){
                td[index].customer->numDishes--;
                td[index].customer->dishes[front()]--;
                printf("\n%d picked up %d",td[index].table_id,front());
                dequeue();
                nanosleep(&req, NULL);
                sleep(1);
                td[index].customer->tipAmount += randomGenerator(1,10);
            }
            sem_post(&mutex);
        }

        sem_wait(&mutex2);
        tipBox += td[index].customer->tipAmount;
        printf("\n%d leaving %d",td[index].customer->id,td[index].customer->tipAmount);
        sem_post(&mutex2);

        //free table (thread) of this customer
        td[index].empty = 1;
        td[index].customer = NULL;
    }
    return NULL;
}

void assignTable(struct Customer* customer,int numTables){

    int flag = 0;//to check if customer gets a table
    for(int i=0;i<numTables;i++){
        if(td[i].empty==1){
            td[i].empty = 0;
            td[i].customer = customer;
            flag  = 1;
            printf("\n%d assigned to %d",td[i].customer->id,td[i].table_id);
            break;
        }
    }

    if(flag == 0){
        printf("\n%d dropped",customer->id);
    }
}

int firstDishDiscardRequired(int numTables){
    for(int i=0;i<numTables;i++){
        if(td[i].empty==0 && td[i].customer->dishes[front()]>0){
            return 0;
        }
    }
    return 1;
}

void produceDish(int numDishes, int numTables){
    //now queue is empty, produce dish
    for(int i=0;i<numDishes;i++){
        while(isFull()){
            if(firstDishDiscardRequired(numTables)){
                dequeue();
            }
        }
        int dish = randomGenerator(0,9);
        enqueue(dish);
        printf("\nDish %d produced",dish);
    }
}

int main(){
    sem_init(&mutex,0,1);
    sem_init(&mutex2,0,1);
    char *fileName = (char*)calloc(200, sizeof(char));
    FILE *fptr;
    printf("Please enter text file name: ");
    scanf("%s",fileName);
    srand(time(0));
    if ((fptr = fopen(fileName,"r")) == NULL){
       printf("Error! opening file");
       exit(1);
       }
       int numTables,capacityFoodQueue;
    fscanf(fptr,"%d", &numTables);
    fscanf(fptr,"%d",&capacityFoodQueue);

    printf("\nNumber of Tables: %d\nCapacity of food Queue: %d",numTables,capacityFoodQueue);

    //initializing threads/tables
    pthread_t threads[numTables];
    td = (struct thread_data*) malloc(numTables * sizeof(struct thread_data));

    for(int i=0;i<numTables;i++){
        td[i].table_id = i;
        td[i].empty = 1;
        td[i].customer = NULL;
        pthread_create(&threads[i],NULL,threadWork,(void*)&td[i].table_id);
    }

    //initializing foodqueue
    createQueue(capacityFoodQueue);

    while(1){
        char letter;
        fscanf(fptr,"%c",&letter);
        if(letter==' ' || letter=='\n')
            continue;
        
        if(letter == 'C'){

            int custId;
            fscanf(fptr,"%d",&custId);
            printf("\nRead from file: %c, Customer ID: %d",letter,custId);
            int numDishes;
            fscanf(fptr,"%d",&numDishes);
            int dish;

            //making customer
            struct Customer* customer = (struct Customer*)malloc(sizeof(struct Customer));
            customer->id = custId;
            customer->numDishes = numDishes;
            customer->tipAmount = 0;
            customer->dishes = (int*)malloc(TOTAL_DISHES * sizeof(int));
            for(int i=0;i<TOTAL_DISHES;i++)
                customer->dishes[i] = 0;
            for(int i=0;i<numDishes;i++){
                fscanf(fptr,"%d",&dish);
                customer->dishes[dish]++;
            }

            //sit customer on table
            assignTable(customer, numTables);
            produceDish(1, numTables);
        }
        else if(letter == 'P'){
            int numDishes;
            fscanf(fptr,"%d",&numDishes);
            printf("\nRead from file: %c, Number of dishes to be produced: %d",letter,numDishes);
            produceDish(numDishes,numTables);
        }else if(letter == 'O'){
            //toggle isOpen
            isOpen = isOpen==1?0:1;
        }else if(letter == 'Q'){
            break;
        }
    }
    fclose(fptr);
    inputOver = 1;
    //wait for all the threads to finish

    sem_destroy(&mutex);
    sem_destroy(&mutex2);

    for(int i=0;i<numTables;i++)
        pthread_join(threads[i],NULL);

    printf("\nTip Collected %d\n",tipBox);
    return 0;
}
