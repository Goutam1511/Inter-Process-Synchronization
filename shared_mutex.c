#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MYMUTEX "/mymutex"

int main(int argc, char* argv[])
{
    pthread_cond_t *cond;
    pthread_mutex_t *mutex;
    int cond_id, mutex_id;
    int mode = S_IRWXU | S_IRWXG;
    /* Check if the shared memory object already exists.
     * If it already exists don't create it again through O_CREATE flag,
     * rather simply attach it to memory using the file descriptor through
     * mmap() however if not exists, then create a shared memory object and
     * truncate the file to appropriate size and then attach to memory by
     * mmap().
     */
    mutex_id = shm_open(MYMUTEX, O_CREAT | O_RDWR | O_EXCL, mode);
    if (mutex_id < 0) {
        mutex_id = shm_open(MYMUTEX, O_RDWR, mode);
        mutex = (pthread_mutex_t *) mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, mutex_id, 0);
        if (mutex == MAP_FAILED) {
            printf("mmap failed\n");
            return -1;
        }
    } else {
        if (ftruncate(mutex_id, sizeof(pthread_mutex_t)) == -1) {
            printf("ftruncate failed\n");
            return -1;
        }
        mutex = (pthread_mutex_t *) mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, mutex_id, 0);
        if (mutex == MAP_FAILED) {
            printf("mmap failed\n");
            return -1;
        }
        /* Mutex initialization -  We only do it if the mutex is already not
         * existing due to some other instance of the process already running.
         */
        pthread_mutexattr_t mattr;
        pthread_mutexattr_init(&mattr);
        pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(mutex, &mattr);
        pthread_mutexattr_destroy(&mattr);
    }
    printf("Mutex Id : %d\n", mutex_id);
    printf("Mutex : %p\n", mutex);

    if (pthread_mutex_trylock(mutex)) { //Try acquiring lock
        printf("Cannot acquire Lock. Some instance might be already running\n");
    } else {
        printf("Acquired Lock now sleeping...\n");
        sleep(5);
        pthread_mutex_unlock(mutex);
        pthread_mutex_destroy(mutex);
        shm_unlink(MYMUTEX);
    }
    return 0;
}
