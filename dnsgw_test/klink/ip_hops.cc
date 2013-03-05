/*#include <stdio.h>
 #include <string.h>

 int main()
 {
 FILE* fp = popen("ftp localhost;","w");
 char str[100];
 strcpy(str, "append ftp_test.cc logs/ftp_test.cc ;");
 fputs(str, fp);
 fflush(fp);
 while(fgets(str, sizeof(str), fp) != NULL) puts(str);
 pclose(fp);
 return 0;
 }

 */
#include <time.h>
#include <stdio.h>

/*int main()
 {
 time_t start, end;
 start = clock();
 for(int i = 0; i < 10000000; i++)
 for(int j = 0; j < 1; j++);
 
 end = clock();
 double t = (double) difftime( end , start ) / CLOCKS_PER_SEC;
 printf("%.9lf\n", t);
 }*/

#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>

using namespace std;

void getNIPHops(string destination)
{
	string command = "ping -c 1 ";
	command += destination;
	FILE* pipe = popen(command.c_str(), "r");
	int line = 0;
	char buffer[300];
	while (line < 2)
	{
		fgets(buffer, sizeof(buffer), pipe);
		line++;
	}
	char* p = strtok(buffer, " ");
	int token = 0;
	while (token < 6)
	{
		p = strtok(NULL, " ");
		token++;
	}

	char* q = strtok(p, "=");
	q = strtok(NULL, "=");
	int hops = atoi(q);
	if (hops < 64)
		hops = 64 - hops;
	else
		hops = 128 - hops;
	printf("%d\n", hops);
}

int main(int argc, char* argv[])
{
	string dest = argv[1];
	getNIPHops(dest);
	return 0;
}
