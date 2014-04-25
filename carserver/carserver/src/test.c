#include <stdio.h>
void main(char** argv, int argc){
 int i=0;
 while(1){
 	int j = scanf("Hallo, das ist %d Test\n",&i);
	printf("Gelesene Zahl: %d, j: %d",i,j);
 }
}
