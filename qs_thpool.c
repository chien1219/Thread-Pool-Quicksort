#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <string.h>
#define Maxsize 1000000

int arr[Maxsize];
void* thread_function(void* thdata);
void bubble_sort(int left, int right);
int quick_sort(int left, int right);
void assign_job_to_thread();
void swap(int* a, int* b){
	int temp = *a;
	*a = *b;
	*b = temp;
}

typedef struct task_data {
	int id;
	int left;
	int right;
	int lchild_id;
	int rchild_id;
}task_data;

typedef struct thread_data {
	pthread_t thread;
	sem_t start;
	task_data* task_range;
}thread_data;

sem_t finish;

//Thread Queue declare
thread_data* thread_pool[8];
int thread_queue_rear, thread_queue_front;
void thread_pool_init();
void puch_thread_queue(thread_data* pthdata);
thread_data* pop_thread_queue();
sem_t thread_mutex;
sem_t thread_flag;
//JOB Queue declare
task_data* job_pool[8];
int jobrear, jobfront;
void task_pool_init();
void push_job_queue(task_data* pjob_data);
task_data* pop_job_queue();
sem_t job_mutex;
sem_t job_flag;

int main(){

	struct timeval start, end;
	int sec, usec;
	thread_data thdata[8];
	task_data job_data[15];
	FILE *fp;
	char output_filename[15];
	int total;   
	
	thread_pool_init();
	task_pool_init();
	sem_init(&finish,0,0);

for(int number_of_thread = 0; number_of_thread<8; number_of_thread++){
	
    FILE* fp;
    if((fp = fopen("input.txt", "r"))==NULL){
	perror("input file error!\n");
	return 0;
    }
    	fscanf(fp,"%d",&total);
    for(int i =0; i<total ; i++){
	fscanf(fp,"%d",&arr[i]);
    }

		for(int i = 0; i<15 ;i++){
		job_data[i].id = i;
		job_data[i].left = 0;
		job_data[i].right = 0;
		job_data[i].lchild_id = i+1;
		job_data[i].rchild_id = i+2;
		}
		
		//create thread
		sem_init(&thdata[number_of_thread].start,0,0);
		pthread_create(&thdata[number_of_thread].thread, NULL ,thread_function,(void*)&thdata[number_of_thread]);
		//set thread data into thread_pool
		puch_thread_queue(&thdata[number_of_thread]);
		job_data[0].left = 0;
		job_data[0].right = total-1;
		push_job_queue(&job_data[0]);
		gettimeofday(&start, 0);
		
		//dynamically assign job
		for(int i = 0; i<15 ;i++)
		assign_job_to_thread();

		for(int i = 0; i<8 ;i++)
		sem_wait(&finish);

		gettimeofday(&end, 0);
		sec = end.tv_sec - start.tv_sec;
		usec = end.tv_usec - start.tv_usec;
		printf("%d threads: ", number_of_thread+1);
		printf("Elapsed time: %f sec\n", sec+(usec/1000000.0));
		//output to file
		strcpy(output_filename, "output_0.txt");
		output_filename[7] += number_of_thread;
		if((fp = fopen(output_filename, "w")) == NULL){
			perror("open output file");
			exit(1);
		}
		for(int i = 0; i<total; i++){
			fprintf(fp, "%d", arr[i]);
			if(i != total-1)fprintf(fp, " ");
		}
		fclose(fp);      
	}
	return 0;
}

void thread_pool_init(){
	sem_init(&thread_mutex,0,1);
	sem_init(&thread_flag,0,0);
	thread_queue_rear = 0;
	thread_queue_front = 0;
}

void task_pool_init(){
	sem_init(&job_mutex,0,1);
	sem_init(&job_flag,0,0);
	jobrear = 0;
	jobfront = 0;
}

int quick_sort(int left, int right){

    int pivot, i, j, t;
    pivot = arr[left];
    i = left; j = right;

    while(1)
    {
        while( arr[i] < pivot ) i++;
        while( arr[j] > pivot ) j--;    
	   if( i >= j ) break;
        swap(arr+i, arr+j);
	i++;
	j--;
    }
    return j;

}

void bubble_sort(int left, int right){
	
	int i , j;
	for(i = left; i < right;i++){
		for(j = right; j >i ;j--){
			if(arr[j-1] > arr[j])
				swap(arr+j, arr+j-1);
		}
	}
	
}
void puch_thread_queue(thread_data* pthdata){
//set or return thread in rear of thread pool
	sem_wait(&thread_mutex);
	thread_pool[thread_queue_rear] = pthdata;
	thread_queue_rear++;
	if(thread_queue_rear == 8)
		thread_queue_rear = 0;
	sem_post(&thread_mutex);
	sem_post(&thread_flag);
}

void push_job_queue(task_data* pjob_data){
//set job in rear of job pool
	sem_wait(&job_mutex);
	job_pool[jobrear] = pjob_data;
	jobrear++;
	if(jobrear == 8)
		jobrear = 0;
	sem_post(&job_mutex);
	sem_post(&job_flag);
}
thread_data* pop_thread_queue(){
//pop thread from pool
	sem_wait(&thread_flag);
	sem_wait(&thread_mutex);
	int tmp = thread_queue_front;
	thread_queue_front++;
	if(thread_queue_front == 8)
		thread_queue_front = 0;
	sem_post(&thread_mutex);
	return thread_pool[tmp];
}

task_data* pop_job_queue(){
//pop job from job pool
	sem_wait(&job_flag);
	sem_wait(&job_mutex);
	int tmp = jobfront;
	jobfront++;
	if(jobfront == 8)
		jobfront = 0;
	sem_post(&job_mutex);
	return job_pool[tmp];
}

void assign_job_to_thread(){
//assign a job to a thread
	thread_data* newthread = pop_thread_queue();
	task_data* newtask = pop_job_queue();
	
	newthread->task_range = newtask;
	sem_post(&newthread->start);
}

void* thread_function(void* thdata){
	thread_data* tmp = (thread_data*)thdata;
	while(1){		
		sem_wait(&tmp->start);
		if(tmp->task_range->id <7 && tmp->task_range->id >= 0){
			int pivot = quick_sort(tmp->task_range->left,tmp->task_range->right);
			task_data* tmp_job1 = tmp->task_range + tmp->task_range->lchild_id;
			task_data* tmp_job2 = tmp->task_range + tmp->task_range->rchild_id;
			tmp_job1->left = tmp->task_range->left;
			tmp_job1->right = pivot;
			tmp_job2->left = pivot+1;
			tmp_job2->right = tmp->task_range->right;
			push_job_queue(tmp_job1);
			push_job_queue(tmp_job2);
			puch_thread_queue(tmp);
		}
		else if(tmp->task_range->id >= 7){
			bubble_sort(tmp->task_range->left,tmp->task_range->right);
			sem_post(&finish);
			puch_thread_queue(tmp);
		}
	}
}
