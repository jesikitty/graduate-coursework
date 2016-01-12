//Jesi Merrick 
//CS553-01, Fall 2014
//Assignment 1

using namespace std;
#include <ctgmath>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

typedef std::chrono::duration<long, std::ratio<1,1000>> millisecs;
template <typename T>
long  time_since(std::chrono::time_point<T> time)
{
  auto diff = std::chrono::system_clock::now() - time;
  return std::chrono::duration_cast<millisecs>( diff ).count();
}

void compute_task(ulong iter, size_t size, void* src, void* dest)
{
  for (ulong i=0; i<iter; i++)
  {
    memcpy(dest, src, size);
  }
}

//create blocks for memcopy
void createBlocks(void* srcs[], void* dests[], size_t size, int num)
{
  //cout<<"Creating " << num << "blocks of size " << size << endl;
  for(int i = 0; i < num; i++){
    srcs[i] = malloc(size);
    dests[i] = malloc(size);
  }
}

//free all blocks
void freeBlocks(void* srcs[], void* dests[], int num)
{
  //cout<<"Freeing " << num << "blocks" << endl;
  for(int i = 0; i < num; i++){
    free(srcs[i]);
    free(dests[i]);   
  }
}

int main(void)
{
  ulong iter=pow(2, 31);
  cout<<"Memory Benchmarking"<<endl;
  for (int i=0; i<2; i++)
  {
    int num_threads=pow(2,i);
    cout<<"\nTesting "<<num_threads<<" threads."<<endl;
    thread threads[num_threads];

    for (int j=0; j<3; j++)
    {
      ulong bsize = pow(1000,j);
      cout << "Block size " << bsize << "B" << endl;

      //RANDOM read+write
      cout << "Random read and write" << endl;

      //create blocks
      void* srcs[num_threads]; void* dests[num_threads];
      createBlocks(srcs, dests, bsize, num_threads);

      //starting timer
      auto before = std::chrono::system_clock::now();
      for(int k=0; k<num_threads; k++) {
        threads[k] = thread(compute_task, iter/bsize, bsize, srcs[k], dests[k]);
      }
      for(int k=0; k<num_threads; k++) {
        threads[k].join();
      }
      auto elapsed = time_since(before);
      //end of timer

      //free the blocks
      freeBlocks(srcs, dests, num_threads);

      cout << elapsed << "ms" << endl;
      ulong speed = (iter/elapsed) * 1000;
      cout << speed/1048576 << " MB/s" << endl;

      //SEQUENTIAL read+write
      cout << "Sequential read and write" << endl;

      //create larger block to copy from
      //size of all the blocks used: bsize*num_threads
      void* main_src = malloc(bsize*num_threads); 
      void* main_dest = malloc(bsize*num_threads); 
      void* src; void* dest;

      //starting timer
      before = std::chrono::system_clock::now();
      for(int k=0; k<num_threads; k++) {
        //Need to move pointer by bsize before passing to a thread to handle
        if(k > 0) {
          src = (void*)((char*)main_src + (k*bsize)-1);
          dest = (void*)((char*)main_src + (k*bsize)-1);
        }
        else {
          src = main_src;
          dest = main_dest;
        }
        threads[k] = thread(compute_task, iter/bsize, bsize, src, dest);
      }
      for(int k=0; k<num_threads; k++) {
        threads[k].join();
      }
      elapsed = time_since(before);
      //end of timer

      //free the blocks
      free(main_src);
      free(main_dest);

      cout << elapsed << "ms" << endl;
      speed = (iter/elapsed) * 1000;
      cout << speed/1048576 << " MB/s" << endl;
    }
  }
}

