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
	int hours = 10;
	int minutes = 50;
	int seconds = 25;

	bits.hours = hours;
	bits.minutes = minutes;
	bits.seconds = seconds;

	

	printf("%i\n", sizeof(bits.hours));
	printf("%i\n", sizeof(bits.minutes));
	printf("%i\n", sizeof(bits.seconds));

	return 0;
}