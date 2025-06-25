#include <unistd.h>
#include <pthread.h>//working with thread does not work without
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>//for get time 
#include <stdlib.h>
//DISCLAIMER: i used the header that were given here and filtered down as neededhttps://www.man7.org/linux/man-pages/man3/pthread_create.3.html

#define MAX_TRAINS 75 //assignment requirements 
#define USLEEP_INTERVAL 80000 //big guess
#define MAX_RECENT_STARVATION 5


//DEFINING the trains (3.3 output of the assignment)
struct train_struct {
  int ID; //Trains have integer identifying numbers
  char direction; //There are only two possible values for direction: "East" and "West"//first valueß
  int loading_time; //SECOND VALUE!
  int crossing_time;//THIRD VALUE
  bool is_ready;
  bool crossed;
} trains[MAX_TRAINS];
//train specifications
char E[] = "East";
char W[] = "West";
int total_trains; //to get the # of trains
char recent_train_directions[] = {'\0','\0', '\0','\0','\0'};//for starvation train_ready

//I was gonna dinamically allocate but faced a lot of issued so chose arrays with no pointers
pthread_t train_threads[MAX_TRAINS]; //thread for each trains
pthread_cond_t cross[MAX_TRAINS];
pthread_mutex_t crossMutex[MAX_TRAINS];
pthread_mutex_t finishLoadingMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t finishedCrossingMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t finishCrossing = PTHREAD_COND_INITIALIZER;


//WE NEED to keep track of when program started because the output needs it 
/*faced issues+contacted Konrad and 0.1 seconds is not a issue
Hi Kariman,
 
I just wanted to update you on this since you asked earlier.
 
I have changed the assignment specification to allow for 00:00:00.1 variation in the output of any specific train, because I was looking at the tests and something was not quite adding up. This is reflected in the specification now.
 
This may be due to rounding errors or clock inconsistencies depending on implementation, and I do not want to enforce unnecessarily painful requirements where unnecessary.*/

int start_time = 0;

//FIRSt get the cur real time clock 
//DISCLAIMER:https://pubs.opengroup.org/onlinepubs/007908799/xsh/clock_getres.html

int get_Milis() {
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  return now.tv_sec *10+now.tv_nsec /100000000;
}
int timeFromBeginning() {
  return get_Milis()- start_time;
}
//as formatting needed hours, mind,secs,decisenonds, these will be used as helper functions
int get_hours(int ticks) {
  return ticks / 36000;
}
int get_Mins(int ticks) {
  return (ticks % 36000) / 600;
}
int get_Secs(int ticks) {
  return (ticks % 600) / 10;
}
int get_Deci(int ticks) {
  return ticks % 10;
}
void* train_thread_function(void* arg) { //train_thread_function thread
  struct train_struct* train = arg;
  char* direction =E;
  if (train->direction != 'e' && train->direction != 'E') { //filling up the defined arrays
    direction = W;
  }
  pthread_mutex_lock(&crossMutex[train->ID]);

  while (!train->is_ready) {
    if (timeFromBeginning() >= train->loading_time) {
      pthread_mutex_lock(&finishLoadingMutex);
      int mins = get_Mins(train->loading_time);
      int secs = get_Secs(train->loading_time);
      int dsecs = get_Deci(train->loading_time);

      int hours = get_hours(train->loading_time);
      printf("%02d:%02d:%02d.%d Train %2d is ready to go %4s\n", hours, mins, secs, dsecs, train->ID, direction);
      train->is_ready = true;
      pthread_mutex_unlock(&finishLoadingMutex);
    } else {
      usleep(USLEEP_INTERVAL);
    }
  } 

  pthread_cond_wait(&cross[train->ID], &crossMutex[train->ID]);

  int getting_on_time = timeFromBeginning();
  int hours = get_hours(getting_on_time);//BIGGG mistake is to un order these , when i put 
                                        //hour last , the ouput got delayed by 0.1 for the small test case
                                        //!!!
  int mins = get_Mins(getting_on_time);
  int secs = get_Secs(getting_on_time);
  int dsecs = get_Deci(getting_on_time);

  printf("%02d:%02d:%02d.%d Train %2d is ON the main track going %4s\n", hours, mins, secs, dsecs, train->ID, direction); //ouput asked for
  pthread_mutex_unlock(&crossMutex[train->ID]);

  while (!train->crossed) {
    if (timeFromBeginning() >= train->crossing_time + getting_on_time) {
      train->crossed = true;
      pthread_mutex_lock(&finishedCrossingMutex);

      int getting_off_time = train->crossing_time + getting_on_time;
      int hours = get_hours(getting_off_time);
      int mins = get_Mins(getting_off_time);
      int secs = get_Secs(getting_off_time);
      int dsecs = get_Deci(getting_off_time);

//crosses and done prnt , formatting followed
      printf("%02d:%02d:%02d.%d Train %2d is OFF the main track after going %4s\n", hours, mins, secs, dsecs, train->ID, direction);
//so need to signal!
      pthread_cond_signal(&finishCrossing); //4-a)
//and release
      pthread_mutex_unlock(&finishedCrossingMutex);
    } else {
      usleep(USLEEP_INTERVAL);
    }
  }
  pthread_exit(NULL);
  return NULL;
}


bool is_starvation_case(char direction) {
  //no trains on track?op dir?
  int same_dir=0;
  int op_dir=0;
  int pos;
  for (pos=0; pos<MAX_RECENT_STARVATION; pos++) {
    if (recent_train_directions[pos] == toupper(direction)){
      same_dir++;

    } else if (recent_train_directions[pos] == '\0') {
      op_dir++;
    }
  }
   if (same_dir>= 3 && op_dir > 0) {
        // Prevent by allowing a train come from op dir
        return true;
    }
  return false;
}

bool most_imp_signaling() {
  struct train_struct* cur = NULL;
  for (int i = total_trains - 1; i >= 0; i--) {
    struct train_struct* current_train = &trains[i];
    if (!current_train->is_ready || current_train->crossed) {
      continue; // Skip trains not ready or already crossed
    }

    if (cur == NULL) {
      cur = current_train; // Pick first available train
      continue;
    }

    // starvation first
    if (is_starvation_case(current_train->direction) && !is_starvation_case(cur->direction)) {
      cur = current_train; // Prioritize solving starvation Recall: Direction/Load Time/Crossing Time
      continue;
    } else if (!is_starvation_case(current_train->direction) && is_starvation_case(cur->direction)) {
      continue; // ok next+added
    }
    // Pick higher priority  train
    if (isupper(current_train->direction) && islower(cur->direction)) {
      cur = current_train;
      continue;
    } else if (islower(current_train->direction) && isupper(cur->direction)) {
      continue;
    }
    // If same priority, pick based on loading time
    if (toupper(current_train->direction) == toupper(cur->direction)) {
      if (current_train->loading_time <= cur->loading_time) {
        cur = current_train;
      }
      continue;
    }
    // no trains criossed? defauklt is eastbound 
    if (recent_train_directions[0] == '\0') {
      if (toupper(current_train->direction) == 'E') {
        cur = current_train;
      }
    } 
    // Otherwise, pick train opposite to last crossed one
    else if(toupper(current_train->direction) !=recent_train_directions[0]) {
      cur=current_train;
    }
  }
  if (cur == NULL) return false; // No train ready//done out of here

  // update recent train history
  recent_train_directions[2] =recent_train_directions[1];
  recent_train_directions[1] = recent_train_directions[0];
  recent_train_directions[0] = toupper(cur->direction);

  pthread_mutex_lock(&crossMutex[cur->ID]);
  pthread_cond_signal(&cross[cur->ID]);//call unblocks at least one of the threads that are blocked on the specified condition variable 
  pthread_mutex_lock(&finishedCrossingMutex);
  pthread_mutex_unlock(&crossMutex[cur->ID]);
  return true;
}
bool crossing_done() {
  for (int cur=0;cur<total_trains;cur++) {
    if (!trains[cur].crossed) {
      return false; //no trains crossed yet=
    }
  }
  return true; //means all did cross
}


void operator() {
  usleep(USLEEP_INTERVAL);

  while (!crossing_done()) { 
    bool signalled  = most_imp_signaling();
    if (!signalled) {
      usleep(USLEEP_INTERVAL);
      continue;
    }
    pthread_cond_wait(&finishCrossing, &finishedCrossingMutex);// waiting for crossing 
    pthread_mutex_unlock(&finishedCrossingMutex);
  }
}


int main(int argc, char *argv[]) {
  char train_direction;
  int load_time, cross_time;
  total_trains = 0;

  // Initialising
  //DISCLAIMER:Used lines 25-3 from Monitor.c(turoail 3)
  for (int i = 0; i < 100; i++) {
    pthread_mutex_init(&crossMutex[i], NULL);
    pthread_cond_init(&cross[i], NULL);
  }
//first we read
  FILE *file_ptr;
  const char* file_name = argv[1];
  file_ptr = fopen(file_name, "r");
  if (!file_ptr) {
    perror("file can not be opened");
    return 1;
  }

//second we assign
  while (fscanf(file_ptr,"%c %d %d\n",&train_direction, &load_time, &cross_time)!=EOF) {
    struct train_struct* new_train =&trains[total_trains];
    new_train->ID = total_trains++; // Assign train ID and increment total
    new_train->loading_time= load_time;
    new_train->crossing_time= cross_time;
    new_train->direction = train_direction;
    new_train->is_ready =false; // Not ready yet
    new_train->crossed = false; // Hasn't crossed the track yet
  }
  fclose(file_ptr);

  start_time =get_Milis(); // to "mark" the start
  //DISCLAIMER:i used the help of https://hpc-tutorials.llnl.gov/posix/passing_args/ to understanding passing arguments
  //Example from source:rc = pthread_create(&threads[t], NULL, PrintHello, (void *) taskids[t]);
  for (int i = 0; i < total_trains; i++) {
    pthread_create(&train_threads[i], NULL, train_thread_function, (void*)&trains[i]);
  }
  // call helper function 
  operator();

  //as seen in monitor.c(tutorial 4)//lecture we need to wait for threads to complete
  for (int i = 0; i < total_trains; i++) {
    pthread_join(train_threads[i], NULL);
  }

}
