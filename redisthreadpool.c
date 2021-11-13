#include <stdlib.h>
#include <sys/queue.h>
#include <hiredis.h>
#include <assert.h> 
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <redisclient.h>
extern int connection_status; 

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
    pthread_cond_signal(&condQueue);
    pthread_mutex_unlock(&mutexQueue);
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
        if(TAILQ_EMPTY(&head))
        {
          //printf("waiting thread\n");
          pthread_cond_wait(&condQueue, &mutexQueue);
          // printf("job arrived thread\n");
        }
        printf("Job found on thread(%d) conn state: %d\n", conn->th_id, connection_status);
        // assert(0);
        if(connection_status == 0)
        {
          printf("Redis disconnected\n");
          pthread_mutex_unlock(&mutexQueue);
          goto EXIT_LABEL;
        }
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
           redisAppendCommand(conn_t, "GET user%05d",num);
           if (redisGetReply(conn_t, (void **)&reply) == REDIS_OK) {
             printf("GET user%05d: %s\n", num, reply->str);
             freeReplyObject(reply);
           } else {
            CHECK_CONN(conn_t);
            printf("Exit thread\n");
            break; 
           }
           // usleep(100000);
           found = 0;
        }
    }
EXIT_LABEL:
    pthread_exit(NULL);
}

// Create multiple redis connections
void * t_check_connection(void *args)
{
   const char *unixname = UNIX_SOCK;
   struct timeval timeout = {1, 500000}; // 1.5 seconds
   redisContext *conn_t; 
   redisReply *reply;
   (args) = (args);   
  
   while(1)
   { 
     conn_t = redisConnectUnixWithTimeout(unixname, timeout);

     while(1)
     {
       printf("Check connection\n");
       /* Send command to the redis pipeline */
       redisAppendCommand(conn_t, "PING");
       if (redisGetReply(conn_t, (void **)&reply) == REDIS_OK) {
          
         freeReplyObject(reply);
         printf("connected..\n");
         connection_status = 1;
       } else {
         if(conn_t)
         {
           redisFree(conn_t);
         }
         connection_status = 0;
         pthread_mutex_lock(&mutexQueue);
         pthread_cond_broadcast(&condQueue);
         pthread_mutex_unlock(&mutexQueue);
         break;
       }
       sleep(5);
     }
     sleep(5);
     printf("Reconnect connection\n");
     conn_t = NULL;
   }  
   printf("Exit thread\n"); 
   pthread_exit(NULL);
   return NULL;
}


int main(int argc, char *argv[])
{
  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

    int i = 0;
    int queue_num = 10; // default

    if(argc >= 2)
      queue_num = atoi(argv[1]); 

    // Set connection_status 1
    connection_status = 1;
    attr = (pthread_attr_t *)malloc(sizeof(pthread_attr_t));
    pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED);

#if 1
    if(pthread_create(&t_producer, NULL , t_check_connection, NULL)!= 0)
    {
      perror("Cannot create check connection thread");
      exit(0);
    }
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
        redis_conn[i].th_id = i;
        if (pthread_create(&redis_conn[i].t_conn, attr, t_redis_command, &redis_conn[i]) != 0)
        {
            printf("Error creating threads \n");
            exit(0);
        }
        printf("Created thread(%d): %lu\n",i, redis_conn[i].t_conn);
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
    printf("Add job\n");
    TAILQ_INIT(&head);
    for (i = 0; i < queue_num; i++)
    {
        add_to_queue(i);
        usleep(2000000);
    }

    pthread_join(t_producer, NULL);
    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);
    pthread_detach(pthread_self());
    printf("program exits here \n");
    pthread_exit(NULL);
    return 0;
}
