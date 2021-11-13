#ifndef _REDISCLIENT_H
#define _REDISCLIENT_H        1


#define UNIX_SOCK "/var/run/redis/redis-server.sock"
#define MAX_THREAD 4

typedef struct st_redis_connection {
  int th_id;
  pthread_t t_conn;
  int t_exit; // bool yes no
}REDIS_CONN;

REDIS_CONN redis_conn[MAX_THREAD];
pthread_t t_producer;
pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;
pthread_attr_t *attr;

#define CHECK_CONN(conn)    \
   if (conn == NULL || conn->err) \
   {                              \
     if (conn)                    \
     {                            \
       printf("Connection error: %s\n", conn->errstr); \
       redisFree(conn);           \
     }                            \
     else                         \
     {                            \
       printf("Connection error: connection NULL\n"); \
     }                            \
     printf("Exit\n"); pthread_exit(NULL);\
   }


int connection_status;

TAILQ_HEAD(tailhead, entry)
head;

typedef struct entry
{
    int num;
    TAILQ_ENTRY(entry)
    entries;
} Q_ENTRY;

#endif
