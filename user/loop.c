#include "kernel/types.h"
#include "user.h"

int
main(int argc, char *argv[])
{ 
  //int i = 0;
  while(1){
    //i++;
    //i++;
    //sleep(1);
    
    for(int b = 0; b < 500; b++){
        //selection sort
        int a[1000], n, i, j, position, swap;
        n=100;
        for (i = 0; i < n; i++){
            a[i] = 1000 - i;
        }
        for(i = 0; i < n - 1; i++){
            position=i;
            for(j = i + 1; j < n; j++){
                if(a[position] > a[j])
                    position=j;
                }
                if(position != i){
                    swap=a[i];
                    a[i]=a[position];
                    a[position]=swap;
                }
        }
    }
    sleep(5);
  }
  exit();
}