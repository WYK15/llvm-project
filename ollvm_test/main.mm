#include <stdio.h>
#import <Foundation/Foundation.h>

static char *str = "lalalallalaldiididlsisjk";
static NSString *globalOcStr = @"this is globalOcStr";

int add(int a , int b) {
	if (&a > (int *)100)
	{
		return a * a + b* b;
	}else {
		return a * a * b * b;
		printf("this is add function\n");
	}

	return 65;
	
}

int sub(int a, int b) {
	return a*a - b*b;
}


int main() {

	printf("hello world\n");

	int d = 10;
	scanf("%d",&d);

	if (d > 100)
	{
		/* code */printf("a + c  = %d\n",add(5,8) );
	}else {
		printf("b+ c = %d\n",sub(5,9));
	}


	printf("str : %s", str);

	NSLog(@"this is OC Str!!!");
	NSLog(globalOcStr);
	
	return 1 + 2;
}