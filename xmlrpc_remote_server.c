/* A simple standalone XML-RPC server program written in C. */

/* The program takes one argument: the HTTP port number on which the server
   is to accept connections, in decimal. Example:

   $ ./xmlrpc_remote_server 8080

   $ export XMLRPC_TRACE_XML=1
*/

#define WIN32_LEAN_AND_MEAN  /* required by xmlrpc-c/server_abyss.h */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#  include <windows.h>
#  include <winsock2.h>
#else
#  include <unistd.h>
#endif

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>

#include "config.h"  /* information about this build environment */


#ifdef _WIN32
  #define SLEEP(seconds) SleepEx(seconds * 1000, 1);
#else
  #define SLEEP(seconds) sleep(seconds);
#endif

typedef struct writer_values {
  struct writer_values *next;
  struct writer_values *prior;
  unsigned char C;
}wrtr;


pthread_mutex_t lock;



/* implementation of keyword 1 */
int keyword_1() {
    /* just return some fixed integer */
    return 2;
}


char *keyword_2(char *c) {
    return c;
}


char *common_func(int caller, char *msg) {
    pthread_mutex_lock(&lock);
    static int x = 0;
    printf("x = %d", x++);
    switch (caller) {
        case 0: printf("\nReader calling with message %s\n", msg); break;
        case 1: printf("\nWriter calling with message %s\n", msg); break;
        default: printf("DEFAULT"); break;
    }

    pthread_mutex_unlock(&lock);
    return msg;
}


void print_writer_values(wrtr *start) {
    wrtr *p;
    int i;

    p = start;
  
    while (p) {
        printf("%c\n", p->C);
        p = p->next;
    }
    printf("\n\n");
}


void sort_writer_values(wrtr **start, wrtr **last) {
    wrtr *a, *b; 
    int flagi=0, i, j;

    for ( ;!flagi;flagi++) {
        a=*start;
        do {
            b=a->next;

            if (b->C > a->C){
	        flagi=-1;
	
	        a->prior ? (b->prior=a->prior),(a->prior->next=b) : ((b->prior=NULL), (*start = b));
	        b->next  ? (b->next->prior=a), (a->next=b->next) : ((a->next = NULL), (*last = a));
	        b->next = a;
	        a->prior = b;
	
	        a=b->next;
            } else {
	        a=b;
            }

        } while(a != *last);
    }
}


void store_writer_value(wrtr *i, wrtr **start, wrtr **last) {
    *start ? ((*last)->next = i), (i->prior = *last) : ((i->prior = NULL), (*start = i));
    i->next = NULL;
    *last = i;
}


void delete_writer_values(wrtr **start) {
    wrtr *p;
    if (*start) {
        while ((*start)->next) {
	    p = (*start)->next;
	    (*start)->next = p->next;
	    if (p->next)
	        p->next->prior = *start;
	    free(p);
        }
        free(*start);
    } 
}


void *writer(void *t) {
    wrtr *start = NULL, *last = NULL, *p;
    int i;
    long tid;
    tid = (long)t;
    printf("Thread %ld starting...\n",tid);
    for (i=0; i<20; i++) { 
        usleep(100000);
        p = (wrtr *)malloc(sizeof(wrtr));
        p->C = 'A' + i;
        store_writer_value(p, &start, &last);
        common_func(tid, &last->C);
    }
    sort_writer_values(&start,&last);
    print_writer_values(start);
    delete_writer_values(&start);
    printf("Thread %ld done.", tid);
    pthread_exit((void*) t);
}


void keyword_3(char *arg1, char *arg2) {
    pthread_t writer_thread;
    pthread_attr_t attr;
    int rc;
    long t;
    void *status;

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);


    if (pthread_mutex_init(&lock, NULL) != 0)  { 
        printf("\n mutex init has failed\n"); 
        return; 
    } 

    t = 1;
    printf("Creating writer thread %ld\n", t);
    rc = pthread_create(&writer_thread, &attr, writer, (void *)t); 
    if (rc) {
        printf("ERROR; return code from pthread writer create() is %d\n", rc);
        exit(-1);
    }

    /* Free attribute and wait for the other threads */
    pthread_attr_destroy(&attr);

    rc = pthread_join(writer_thread, &status);
    if (rc) {
        printf("ERROR; return code from pthread_join() is %d\n", rc);
        exit(-1);
    }
    printf("Completed join with writer thread having a status of %ld\n",(long)status);
  
    printf("Keyword completed. Exiting.\n");
    pthread_mutex_destroy(&lock);
    //pthread_exit(NULL);
}



/* Implementation of the mandatory get_keyword_names function */
/* Using a simple hard-coded list of keyword function names  */
static xmlrpc_value *get_keyword_names(xmlrpc_env *   const envP,
                                       xmlrpc_value * const paramArrayP,
                                       void *         const serverInfo,
                                       void *         const channelInfo) {

    return xmlrpc_build_value(envP, "(ssss)", 
                                    "keyword 1",
                                    "keyword 2", 
                                    "keyword 3",
                                    "keyword 4");
}


static xmlrpc_value *run_keyword(xmlrpc_env *   const envP,
           xmlrpc_value * const paramArrayP,
           void *         const serverInfo,
           void *         const channelInfo) {

    char *keyword = 0;
    char *dummy = 0;
    char *arg1 = 0;
    char *arg2 = 0;
    char *arg3 = 0;
    int retVal1 = 0;
    char *retVal2;
    xmlrpc_value *val;
    char status[5] = {};

    /* Parse keyword name from argument array */
    xmlrpc_decompose_value(envP, paramArrayP, "(s*)", &keyword);

    if (!strcmp(keyword,"keyword 1")) {
        /* calling keyword with no arguments */
        retVal1 = keyword_1(); 
        strcpy(status, retVal1 > 0 ? "PASS" : "FAIL");
        val = xmlrpc_build_value(envP, "{s:s,s:s,s:i}","status", status,
                                                       "output", "Executed keyword 1",
                                                       "return", retVal1);
    } else if (!strcmp(keyword,"keyword 2")) {
        xmlrpc_decompose_value(envP, paramArrayP, "(s(s))", &dummy, &arg1);
        retVal2 = keyword_2(arg1); 
        strcpy(status, retVal2[0] > 'A' ? "PASS" : "FAIL");
        val = xmlrpc_build_value(envP, "{s:s,s:s,s:s}","status", status,
                                                       "output", "Executed keyword 2",
                                                       "return", retVal2);
    } else if (!strcmp(keyword,"keyword 3")) {
        xmlrpc_decompose_value(envP, paramArrayP, "(s(ss))", &dummy, &arg1, &arg2);
        keyword_3(arg1,arg2);
        val = xmlrpc_build_value(envP, "{s:s,s:s,s:(sss)}","status", "PASS",
                                                           "output", "Executed keyword 3",
                                                           "return", "one","two","three");
    } else if (!strcmp(keyword,"keyword 4")) {
       xmlrpc_decompose_value(envP, paramArrayP, "(s(sss))", &dummy, &arg1, &arg2, &arg3);
    }


    /* Parse our argument array. */
    
    printf("\n VALUE: %s\n", keyword);
    printf("\n VALUE: %s\n", arg1);
    printf("\n VALUE: %s\n", arg2);
    printf("\n VALUE: %s\n", arg3);
    return val;
}



int
main(int           const argc,
     const char ** const argv) {

    struct xmlrpc_method_info3 const getKeywordNames = {
        .methodName     = "get_keyword_names",
        .methodFunction = &get_keyword_names
    };

    struct xmlrpc_method_info3 const runKeyword = {
        .methodName     = "run_keyword",
        .methodFunction = &run_keyword
    };

    xmlrpc_server_abyss_parms serverparm;
    xmlrpc_registry * registryP;
    xmlrpc_env env;

    if (argc-1 != 1) {
        fprintf(stderr, "You must specify 1 argument:  The TCP port "
                "number on which the server will accept connections "
                "for RPCs (8080 is a common choice).  "
                "You specified %d arguments.\n",  argc-1);
        exit(1);
    }

    xmlrpc_env_init(&env);

    registryP = xmlrpc_registry_new(&env);
    if (env.fault_occurred) {
        printf("xmlrpc_registry_new() failed.  %s\n", env.fault_string);
        exit(1);
    }

    xmlrpc_registry_add_method3(&env, registryP, &getKeywordNames);
    xmlrpc_registry_add_method3(&env, registryP, &runKeyword);
    if (env.fault_occurred) {
        printf("xmlrpc_registry_add_method3() failed.  %s\n",
               env.fault_string);
        exit(1);
    }

    serverparm.config_file_name = NULL;   /* Select the modern normal API */
    serverparm.registryP        = registryP;
    serverparm.port_number      = atoi(argv[1]);
    serverparm.log_file_name    = "/tmp/xmlrpc_log";

    printf("Running XML-RPC server...\n");

    xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(log_file_name));
    if (env.fault_occurred) {
        printf("xmlrpc_server_abyss() failed.  %s\n", env.fault_string);
        exit(1);
    }
    /* xmlrpc_server_abyss() never returns unless it fails */

    return 0;
}


