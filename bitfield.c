#include <stdio.h>

struct bits {
  unsigned int hours:4;
  unsigned int minutes:6;
  unsigned int seconds:6;
};

int main() 
{
	int hours = 10;
	int minutes = 50;
	int seconds = 25;

	int bits bit;
	bit.hours = hours;
	bit.minutes = minutes;
	bit.seconds = seconds;

	printf("%i\n", bit.hours);
	printf("%i\n", bit.minutes);
	printf("%i\n", bit.seconds);

	return 0;
}