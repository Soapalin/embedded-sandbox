#include <stdio.h>

struct bits {
  unsigned int hours:4;
  unsigned int minutes:6;
  unsigned int seconds:6;
};
typedef struct bits bit_t;

bit_t bits;

int main() 
{
	
	int inputSecond;
	
	int hours,minutes,seconds;
	int remainingSeconds;
	
	int secondsInHour = 60 * 60;
	int secondsInMinute = 60;
	
	printf("Enter seconds : ");
	scanf("%d",&inputSecond);
	
	hours = (inputSecond/secondsInHour);
	
	remainingSeconds = inputSecond - (hours * secondsInHour);
	minutes = remainingSeconds/secondsInMinute;
	
	remainingSeconds = remainingSeconds - (minutes*secondsInMinute);
	seconds = remainingSeconds;


	hours = hours << 12;
	minutes = minutes << 6;
	int packed_Time = hours + minutes + seconds;
	return 0;
}