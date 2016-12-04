#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

//#define RAND_MAX 1000
#define MAX_VALUE 10000
#define COUNT 10000		//total number
#define MEAN  5000		//mean value
#define SDEV  1000		//standard device

int main(){
  float mean, sdev, N, sign, sum,sdevsq ;
  int x[COUNT];
  int i,s;	
  float yr,yxr,xr;	
  
  FILE *fp; 
  fp = fopen("dp.log","w");

  mean = MEAN; 
  sdev = SDEV; 
  N = COUNT;  

  srand(time(NULL));
  sdevsq = sdev * sdev;

  for(i = 0; i <= N; i++) {
  	yr = 1;
 	yxr = 0;

  	while(yr > yxr) {
  		xr = mean* sdev * ((float)rand()/RAND_MAX);
  		yr = N * ((float)rand()/RAND_MAX);
  		yxr = N * exp(-(xr) * (xr) / (2 * sdevsq));
		
	}

	s = 1;
	if( (rand() %10) < 5)
	  s = -1;
	if(mean + s * xr <0 || mean + s * xr > MAX_VALUE)
	  i--;
	else{
	  x[i] = mean + s * xr;
	  fprintf(fp,"%d\n",x[i]);
	  printf("%d  \n" , x[i]);
	}
  }
  fclose(fp);
}
