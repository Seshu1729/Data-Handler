#include <stdio.h>
#include <stdlib.h>

int get_file_size(char *file_address)
{
	int file_size_in_bytes,file_size;

	FILE *file_pointer = fopen(file_address, "r"); 
	fseek(file_pointer, 0, SEEK_END);
	file_size_in_bytes = ftell(file_pointer);
	fclose(file_pointer);

	file_size = file_size_in_bytes / 1024;
	if (file_size_in_bytes % 1024)
		file_size++;
	return file_size;
}

int main()
{
	printf("%d", get_file_size("documents/sample video.mp4"));
	return 0;
}