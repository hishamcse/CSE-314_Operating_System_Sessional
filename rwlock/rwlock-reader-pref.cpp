#include "rwlock.h"

void InitalizeReadWriteLock(struct read_write_lock * rw)
{
  //	Write the code for initializing your read-write lock.
  rw->readers = 0;
  rw->writers = 0;
  pthread_mutex_init(&rw->mutex, NULL);
  pthread_cond_init(&rw->readLock, NULL);
  pthread_cond_init(&rw->writeLock, NULL);
}

void ReaderLock(struct read_write_lock * rw)
{
  //	Write the code for aquiring read-write lock by the reader.
  pthread_mutex_lock(&rw->mutex);
  rw->readers++;
  while(rw->writers > 0)
    pthread_cond_wait(&rw->readLock, &rw->mutex);
  pthread_mutex_unlock(&rw->mutex);
}

void ReaderUnlock(struct read_write_lock * rw)
{
  //	Write the code for releasing read-write lock by the reader.
  pthread_mutex_lock(&rw->mutex);
  rw->readers--;
  if(rw->readers == 0)
    pthread_cond_broadcast(&rw->writeLock);
  pthread_mutex_unlock(&rw->mutex);
}

void WriterLock(struct read_write_lock * rw)
{
  //	Write the code for aquiring read-write lock by the writer.
  pthread_mutex_lock(&rw->mutex);
  while(rw->readers > 0 || rw->writers > 0)
    pthread_cond_wait(&rw->writeLock, &rw->mutex);
  rw->writers++;
  pthread_mutex_unlock(&rw->mutex);
}

void WriterUnlock(struct read_write_lock * rw)
{
  //	Write the code for releasing read-write lock by the writer.
  pthread_mutex_lock(&rw->mutex);
  rw->writers--;
  pthread_cond_broadcast(&rw->readLock);
  pthread_cond_broadcast(&rw->writeLock);
  pthread_mutex_unlock(&rw->mutex);
}
