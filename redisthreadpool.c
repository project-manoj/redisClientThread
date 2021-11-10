#include <stdlib.h>
#include <sys/queue.h>
#include <hiredis.h>
#include <assert.h> 
#include <async.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <redisclient.h>


void do_big_job(void)
{
    volatile unsigned long long longnum;
    for (longnum = 0; longnum < 1000000000ULL; ++longnum);    
}

void add_to_queue(int n)
{
    Q_ENTRY *elem;
    elem = malloc(sizeof(Q_ENTRY));
    if (elem)
    {
        elem->num = n;
    }
    pthread_mutex_lock(&mutexQueue);
    printf("Adding to queue: %d\n", elem->num);
    TAILQ_INSERT_TAIL(&head, elem, entries);
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
}


void *t_redis_command(void *args)
{
    /* local vairables */
    Q_ENTRY *elem = NULL;
    int num = 0;
    int found = 0;
    REDIS_CONN  *conn = args;
    redisContext *conn_t;
    redisReply *reply;
    const char *unixname = UNIX_SOCK;
    struct timeval timeout = {1, 500000}; // 1.5 seconds
    
    
    conn_t = redisConnectUnixWithTimeout(unixname, timeout);
    CHECK_CONN(conn_t);

    // Get task from queue
    // execute task
    while(1)
    {
        pthread_mutex_lock(&mutexQueue);
        while(TAILQ_EMPTY(&head))
        {
          pthread_cond_wait(&condQueue, &mutexQueue);
        }
        printf("Job found on thread(%d)\n", conn->th_id);
        elem = head.tqh_first;
        if (elem)
        {
            found = 1;
            TAILQ_REMOVE(&head, head.tqh_first, entries);
            printf("(%d)Element found: '%d'\n", conn->th_id, elem->num);
            num = elem->num;
            free(elem);
        }

        pthread_mutex_unlock(&mutexQueue);

        // execution will take 2 secs
        if (found)
        {  
           // CHECK_CONN(conn_t);
           /* Try a GET and two INCR */
           reply = redisCommand(conn_t,"GET user%05d",num);
           if (reply == NULL) {
             CHECK_CONN(conn_t);
           }
           printf("GET user%05d: %s\n", num, reply->str);
           freeReplyObject(reply);
           // usleep(400000);
           //usleep(1000);
           found = 0;
        }
    }
}
// Create multiple redis connections
void * t_check_connection(void *args)
{
   // int i = MAX_THREAD;
   const char *unixname = UNIX_SOCK;
   struct timeval timeout = {1, 500000}; // 1.5 seconds
   redisContext *conn_t; 
   (args) = (args);   
   
   conn_t = redisConnectUnixWithTimeout(unixname, timeout);
    
   CHECK_CONN(conn_t);

   while(1)
   {
     printf("Check connection \n");
     sleep(10);
     // break;
#if 0
     for (i = 0; i < 1000; i++)
     {
         add_to_queue(i);
     }
#endif
   }   
   pthread_exit(NULL);
//    return NULL;

}
// Create multiple redis connections
void * t_producer_command(void *args)
{
   int i = MAX_THREAD;
   const char *hostname = "192.168.179.253";
   int port = 6379;
   struct timeval timeout = {1, 500000}; // 1.5 seconds
   (args) = (args);
   for(i=0; i < MAX_THREAD ; i++)
   {
     conn[i] = redisConnectWithTimeout(hostname, port, timeout);
     if (conn[i] == NULL || conn[i]->err)
     {
         CHECK_CONN(conn[i]);
     }
     printf("Redis connection[%d] done. \n",i);   
   }
   return NULL;
}

int main(int argc, char *argv[])
{

    int i = 0;
    int queue_num = 10; // default
    int *th_id[MAX_THREAD];
    int th_attr = PTHREAD_CREATE_DETACHED;

    if(argc >= 2)
      queue_num = atoi(argv[1]); 

    attr = (pthread_attr_t *)malloc(sizeof(pthread_attr_t));
    pthread_attr_setdetachstate(attr, th_attr);

#if 1
    if(pthread_create(&t_producer, attr , t_check_connection, NULL)!= 0)
    {
      perror("Cannot create producer thread");
      exit(0);
    }
    pthread_detach(t_producer);
    // pthread_join(t_producer, NULL);
    // sleep(5);
#endif

    pthread_mutex_init(&mutexQueue, NULL);

    if (pthread_cond_init(&condQueue, NULL) != 0)
    {
        perror("pthread_cond_init() error");
        exit(1);
    }

    printf("Create thread pool of %d threads\n", MAX_THREAD);
    for (i = 0; i < MAX_THREAD; i++)
    {
        th_id[i] = (int*)malloc(sizeof(int));
        redis_conn[i].th_id = i;
        if (pthread_create(&redis_conn[i].t_conn, attr, t_redis_command, &redis_conn[i]) != 0)
        {
            printf("Error creating threads \n");
            exit(0);
        }
        printf("Created thread(%d): %lu\n",*th_id[i], redis_conn[i].t_conn);
        // pthread_detach(redis_conn[i].t_conn);
    }

    TAILQ_INIT(&head);
    for (i = 0; i < queue_num; i++)
    {
        add_to_queue(i);
        usleep(100000);

        // usleep(1000000);
    }
/*
    for (i = 0; i < MAX_THREAD; i++)
    {
        if (pthread_join(redis_conn[i].t_conn, NULL) != 0)
        {
            printf("Error joining thread\n");
            exit(0);
        }
    }
*/
    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);
    pthread_exit(NULL);
    return 0;
}
