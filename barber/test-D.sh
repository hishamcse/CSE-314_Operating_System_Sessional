echo "Testing D: Mutex and CV"
g++ cond-solve.c -o cond-solve -lpthread
./cond-solve $1 $2
echo "Testing D: Semaphore"
g++ semaphore-solve.c zemaphore.c -o semaphore-solve -lpthread
./semaphore-solve $1 $2