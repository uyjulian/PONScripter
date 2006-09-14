// A trivial program to facilitate embedding binary data in C++.

#include <stdio.h>

int main(int argc, char** argv)
{
	int len = 0;
	printf("const unsigned char %s_buffer[] = {", argv[1]);
	if (!argv[2]) {
		len = 1;
		puts("\n\t0");
	}
	else {
		int c;
		FILE* f;
		f = fopen(argv[2], "rb");
		while ((c = getc(f)) != EOF) {
			if (len) putchar(',');
			if (len++ % 16 == 0) printf("\n\t"); else putchar(' ');
			printf("%3d", c);
		}
		fclose(f);
	}
	printf("\n};\nconst long int %s_size = %d;\n", argv[1], len);
	return 0;
}
