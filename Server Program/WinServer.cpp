#include "stdafx.h"
#include <winsock2.h>
#include <windows.h>
#include <Windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#define buffer_size 1024
#define data_size 32
#define initial_date "01-01-2016"

#define blob_database "Asserts/blob_database.bin"
#define message_database "Asserts/message_database.bin"
#define calender_database "Asserts/calender_database.bin"

//////////////////////////////////////////////
//											//
//		      ***BLOB MACROS***	     		//
//											//
//////////////////////////////////////////////

#define blob_meta_data_size (24 * 1024 * 1024)
#define blob_file_blocks (1024 * 1000)

#define blob_user_data_size (8 * 1024 * 1024)
#define blob_user_block_size sizeof(struct blob_user_node)
#define blob_max_user_count ((blob_user_data_size/sizeof(struct blob_user_node)) - 1)
#define blob_user_file_count 16

#define blob_existing_files_data_size (12 * 1024 * 1024)
#define blob_existing_files_block_size sizeof(struct blob_existing_file_node)
#define blob_max_existing_files_count (blob_existing_files_data_size/sizeof(struct blob_existing_file_node))

#define blob_deleted_files_data_size (4 * 1024 * 1024)
#define blob_deleted_files_block_size sizeof(struct blob_deleted_file_node)
#define blob_max_deleted_file_count (blob_deleted_files_data_size/sizeof(struct blob_deleted_file_node))


//////////////////////////////////////////////
//											//
//		    ***MASSAGE MACROS***	     	//
//											//
//////////////////////////////////////////////

#define message_meta_data_size (1024 * 1024)

#define message_bit_vector_size (60 * 1024)

#define message_user_data_size message_meta_data_size - message_bit_vector_size
#define message_user_block_size sizeof(struct message_user_node)
#define message_max_user_count ((message_user_data_size/message_user_block_size)-1)

#define message_message_block_size 256
#define message_bit_vector_blocks (15 * 1024)


//////////////////////////////////////////////
//											//
//		    ***CALENDER MACROS***	     	//
//											//
//////////////////////////////////////////////

#define calender_category_data_size (10 * 1024 * 1024)

#define calender_user_block_size sizeof(struct calender_service_man_details)
#define calender_max_user_count (calender_category_data_size)/(sizeof(struct calender_service_man_details)-1)

//////////////////////////////////////////////
//											//
//		    ***STRUCTURE TYPES***			//
//											//
//////////////////////////////////////////////

struct blob_user_node
{
	char user_name[data_size];
	char password[data_size];
	int file_offsets[blob_user_file_count];
};

struct blob_existing_file_node
{
	char file_name[data_size];
	int start_offset;
	int file_size;
};

struct blob_deleted_file_node
{
	int start_offset;
	int file_size;
};

struct message_user_node
{
	char user_name[data_size];
	char password[data_size];
};

struct calender_service_man_details
{
	char name[data_size];
	char number[data_size];
	char address[2 * data_size];
	int calender[180];
};

struct calender_date
{
	int day;
	int month;
	int year;
};

//////////////////////////////////////////////
//											//
//	     ***FUNCTION DEFINATIONS***			//
//											//
//////////////////////////////////////////////

DWORD WINAPI SocketHandler(void*);

//////////////////////////////////////////////
//											//
//	      ***GLOBAL VARIABLES***			//
//											//
//////////////////////////////////////////////

int zero = 0, one = 1;

//////////////////////////////////////////////
//											//
//		    ***SERVER FUNCTIONS***			//
//											//
//////////////////////////////////////////////

void socket_server()
{
	int host_port = 1101;
	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0 || (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2))
	{
		fprintf(stderr, "No sock dll %d\n", WSAGetLastError());
		goto FINISH;
	}

	int hsock;
	int *p_int;
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if (hsock == -1)
	{
		printf("Error initializing socket %d\n", WSAGetLastError());
		goto FINISH;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if ((setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1) || (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1))
	{
		printf("Error setting options %d\n", WSAGetLastError());
		free(p_int);
		goto FINISH;
	}
	free(p_int);

	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1)
	{
		fprintf(stderr, "Error binding to socket %d\n", WSAGetLastError());
		goto FINISH;
	}

	if (listen(hsock, 10) == -1)
	{
		fprintf(stderr, "Error listening %d\n", WSAGetLastError());
		goto FINISH;
	}

	int* csock;
	sockaddr_in sadr;
	int	addr_size = sizeof(SOCKADDR);

	int request_count = 0;

	while (true)
	{
		csock = (int*)malloc(sizeof(int));

		if ((*csock = accept(hsock, (SOCKADDR*)&sadr, &addr_size)) != INVALID_SOCKET)
		{
			request_count++;
			printf("REQUEST RECEIVED :: %d\n",request_count);
			CreateThread(0, 0, &SocketHandler, (void*)csock, 0, 0);
		}
		else
		{
			fprintf(stderr, "Error accepting %d\n", WSAGetLastError());
		}
	}
	FINISH:
		/*DO NOTHING*/;
}

char *receive_message(int *csock)
{
	char buffer[1024];
	int bytecount;
	memset(buffer, 0, buffer_size);
	if ((bytecount = recv(*csock, buffer, buffer_size, 0)) == SOCKET_ERROR)
	{
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return 0;
	}
	return buffer;
}

void send_message(int *csock, char *buffer)
{
	int bytecount;
	if ((bytecount = send(*csock, buffer, strlen(buffer), 0)) == SOCKET_ERROR)
	{
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
	}
}

//////////////////////////////////////////////
//											//
//	   	  ***HELPER FUNCTIONS***		 	//
//											//
//////////////////////////////////////////////

int get_size_of_file(char *file_name)
{
	int size;
	FILE *file_pointer = fopen(file_name, "r");
	fseek(file_pointer, 0, SEEK_END);
	size = ftell(file_pointer);
	fclose(file_pointer);
	return size;
}

void save_message_data(int offset, char *message)
{
	FILE *file_pointer = fopen(message_database, "rb+");
	fseek(file_pointer, offset, SEEK_SET);
	fwrite(message, message_message_block_size, 1, file_pointer);
	fclose(file_pointer);
}

void save_number_data(int offset, int data)
{
	FILE *file_pointer = fopen(message_database, "rb+");
	fseek(file_pointer, offset, SEEK_SET);
	fwrite(&data, sizeof(int), 1, file_pointer);
	fclose(file_pointer);
}

char *get_message_data(int offset, int block_size)
{
	char buffer[buffer_size];
	FILE *file_pointer = fopen(message_database, "rb+");
	fseek(file_pointer, offset, SEEK_SET);
	fread(buffer, block_size, 1, file_pointer);
	fclose(file_pointer);
	return buffer;
}

char *allocate_string_memory(int string_size)
{
	char *string;

	string = (char *)malloc(string_size);
	memset(string, '\0', string_size);

	return string;
}

int count_leap_years(calender_date d)
{
	int years = d.year;

	if (d.month <= 2)
		years--;

	return years / 4 - years / 100 + years / 400;
}

int get_difference_between_date(calender_date d1, calender_date d2)
{
	const int month_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	
	long int n1 = d1.year * 365 + d1.day;
	for (int i = 0; i<d1.month - 1; i++)
		n1 += month_days[i];
	n1 += count_leap_years(d1);

	long int n2 = d2.year * 365 + d2.day;
	for (int i = 0; i<d2.month - 1; i++)
		n2 += month_days[i];
	n2 += count_leap_years(d2);

	return (n2 - n1);
}

int is_leap(int year)
{
	int leap = 1;
	if (year % 4 != 0)
		leap = 0;
	else if (year % 400 == 0)
		leap = 1;
	else if (year % 100 == 0)
		leap = 0;
	return leap;
}

int is_valid_date(calender_date date)
{
	int month_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if (is_leap(date.year))
		month_days[1]++;
	if (month_days[date.month - 1]<date.day || date.day<1)
		return 0;
	if (date.month>12 || date.month<0)
		return 0;
	return 1;
}

//////////////////////////////////////////////
//											//
//		***BLOB HELPER FUNCTIONS***			//
//											//
//////////////////////////////////////////////

int get_blob_user_count()
{
	int user_count;

	FILE *file_pointer = fopen(blob_database,"rb+");
	fread(&user_count, sizeof(int), 1, file_pointer);
	fclose(file_pointer);

	return user_count;
}

void set_blob_user_count(int user_count)
{
	FILE *file_pointer = fopen(blob_database, "rb+");
	fwrite(&user_count, sizeof(int), 1, file_pointer);
	fclose(file_pointer);
}

int get_blob_file_filled_block_count()
{
	int offset;

	FILE *file_pointer = fopen(blob_database, "rb+");
	fseek(file_pointer, sizeof(int), SEEK_SET);
	fread(&offset, sizeof(int), 1, file_pointer);
	fclose(file_pointer);

	return offset;
}

void set_blob_file_filled_block_count(int offset)
{
	FILE *file_pointer = fopen(blob_database, "rb+");
	fseek(file_pointer, sizeof(int), SEEK_SET);
	fwrite(&offset, sizeof(int), 1, file_pointer);
	fclose(file_pointer);
}

int get_blob_user_offset(char *user_name)
{
	char *user_name_from_database = allocate_string_memory(data_size);
	FILE *file_pointer = fopen(blob_database, "rb+");
	int user_offset, user_count = get_blob_user_count();

	for (int index = 1; index <= user_count; index++)
	{
		user_offset = blob_user_block_size * index;
		fseek(file_pointer, user_offset, SEEK_SET);
		fread(user_name_from_database, data_size, 1, file_pointer);
		if (!strcmp(user_name, user_name_from_database))
		{
			fclose(file_pointer);
			return user_offset;
		}
	}

	fclose(file_pointer);
	return -1;
}

int get_blob_file_slot(char *user_name)
{
	int index, file_offset, offset_value, user_offset = get_blob_user_offset(user_name);
	FILE *file_pointer = fopen(blob_database, "rb+");

	fseek(file_pointer, user_offset + 2 * data_size, SEEK_SET);

	for (index = 0; index < blob_user_file_count;index++)
	{
		file_offset = ftell(file_pointer);
		fread(&offset_value, sizeof(int), 1, file_pointer);
		if (offset_value == 0)
		{
			fclose(file_pointer);
			return file_offset;
		}
	}

	fclose(file_pointer);
	return -1;
}

int get_blob_strorage_offset()
{
	int storage_offset, index, data;
	FILE *file_pointer = fopen(blob_database, "rb+");
	fseek(file_pointer, blob_user_data_size, SEEK_SET);

	for (index = 0; index < blob_max_existing_files_count; index++)
	{
		storage_offset = ftell(file_pointer);
		fread(&data, sizeof(int), 1, file_pointer);
		if (data == 0)
		{
			fclose(file_pointer);
			return storage_offset;
		}
		fseek(file_pointer, sizeof(int), SEEK_CUR);
	}

	fclose(file_pointer);
	return -1;
}

//////////////////////////////////////////////
//											//
//		 ***BLOB CORE FUNCTIONS***			//
//											//
//////////////////////////////////////////////

char *blob_login()
{
	int user_offset, user_count = get_blob_user_count();
	char user_name[data_size], password[data_size];
	char password_from_database[data_size];

	strcpy(user_name, strtok(NULL, "/"));
	strcpy(password, strtok(NULL, "/"));

	user_offset = get_blob_user_offset(user_name);
	if (user_offset != -1)
	{
		FILE *file_pointer = fopen(blob_database, "rb+");
		fseek(file_pointer, user_offset + data_size, SEEK_SET);
		fread(&password_from_database, data_size, 1, file_pointer);
		fclose(file_pointer);
		if (!strcmp(password_from_database, password))
			return "VALID USER";
	}
	return "USERNAME AND PASSWORD ARE NOT MATCHED.\nPLEASE TRY AGAIN.\n";
}

char *blob_signup()
{
	struct blob_user_node new_user_data;
	int user_count = get_blob_user_count(), user_offset;

	memset(&new_user_data, 0, sizeof(blob_user_node));
	strcpy(new_user_data.user_name, strtok(NULL, "/"));
	strcpy(new_user_data.password, strtok(NULL, "/"));

	if (user_count == blob_max_user_count)
		return "MAXIMUM USER COUNT REACHED\n.UNABLE TO CREATE NEW USERS.\n";
	else if (get_blob_user_offset(new_user_data.user_name) != -1)
		return "USERNAME ALREADY USED. PLEASE TRY WITH DIFERENT USERNAME.\n";
	else
	{
		user_count += 1;
		user_offset = user_count * blob_user_block_size;

		FILE *file_pointer = fopen(blob_database, "rb+");
		fseek(file_pointer, user_offset, SEEK_SET);
		fwrite(&new_user_data, sizeof(blob_user_node), 1, file_pointer);
		fclose(file_pointer);

		set_blob_user_count(user_count);
		return "NEW USER CREATED SUCCUSSFULLY!!";
	}
}

char *blob_viewfiles(char *user_name)
{
	char *result = allocate_string_memory(buffer_size);
	char *file_name, *buffer;
	int file_count = 0, user_offset = get_blob_user_offset(user_name);
	int file_data_offset, file_offset;
	FILE *file_pointer = fopen(blob_database, "rb+");

	for (int index = 0; index < blob_user_file_count; index++)
	{
		buffer = allocate_string_memory(buffer_size);

		file_data_offset = user_offset + 2 * data_size + index * sizeof(int);
		fseek(file_pointer, file_data_offset, SEEK_SET);
		fread(&file_offset, sizeof(int), 1, file_pointer);
		if (file_offset)
		{
			file_name = allocate_string_memory(buffer_size);
			fseek(file_pointer, file_offset, SEEK_SET);
			fread(file_name, data_size, 1, file_pointer);
			sprintf(buffer, "%c %s\n", 4, file_name);
			strcat(result, buffer);
			file_count += 1;
		}
	}
	fclose(file_pointer);
	if (file_count != 0)
		return result;
	else
		return "NO FILES CREATED YET!!.";
}

char *blob_allocate_file_memory(char *user_name)
{
	char *file_name = allocate_string_memory(data_size);
	char *buffer = allocate_string_memory(buffer_size);
	int file_size, file_slot, file_storage_offset, filled_block_count;

	strcpy(file_name, strtok(NULL, "/"));
	file_size = atoi(strtok(NULL, "/"));
	filled_block_count = get_blob_file_filled_block_count();

	if (file_size  > blob_file_blocks - filled_block_count)
		return "DATABASE MEMORY OUT OF RANGE.\nTRY WITH SMALLER SIZE FILES.\n";
	else
	{
		file_slot = get_blob_file_slot(user_name);

		if (file_slot == -1)
			return "EACH USER CAN SAVE 64 FILES ONLY.\nPLEASE DELETE OLDER FILES AND TRY AGAIN.\n";

		file_storage_offset = get_blob_strorage_offset();

		FILE *file_pointer = fopen(blob_database, "rb+");
		struct blob_existing_file_node new_file;
		strcpy(new_file.file_name, file_name);
		new_file.start_offset = filled_block_count;
		new_file.file_size = file_size;

		fseek(file_pointer, file_slot, SEEK_SET);
		fwrite(&file_storage_offset, sizeof(int), 1, file_pointer);

		fseek(file_pointer, file_storage_offset, SEEK_SET);
		fwrite(&new_file, sizeof(struct blob_existing_file_node), 1, file_pointer);
		fclose(file_pointer);

		set_blob_file_filled_block_count(filled_block_count + file_size);

		sprintf(buffer, "%d", filled_block_count);
		return buffer;
	}
}

char *blob_add_file_data(char *user_name)
{
	int file_offset, index;
	char *buffer = allocate_string_memory(buffer_size + 128);

	file_offset = atoi(strtok(NULL, "/"));
	index = atoi(strtok(NULL, "/"));
	strcpy(buffer, strtok(NULL, "/"));

	FILE *file_pointer = fopen(blob_database, "rb+");
	fseek(file_pointer, blob_meta_data_size + (file_offset + index) * buffer_size, SEEK_SET);
	fwrite(buffer, buffer_size, 1, file_pointer);
	fclose(file_pointer);

	return "DATA RECEIVED";
}

char *blob_addfile(char *user_name)
{
	char parser[data_size];
	strcpy(parser,strtok(NULL, "/"));

	if (!strcmp(parser, "FILEINFO"))
		return blob_allocate_file_memory(user_name);
	else if (!strcmp(parser, "FILEDATA"))
		return blob_add_file_data(user_name);
	else
		return "URL ERROR : PLEASE REPORT ABOUT ERROR";
}

char *blob_deletefile(char *user_name)
{
	FILE *file_pointer = fopen(blob_database, "rb+");
	int file_size, file_offset, storage_offset;

	file_offset = atoi(strtok(NULL, "/"));
	fseek(file_pointer, file_offset, SEEK_SET);
	fwrite(&zero, sizeof(int), 1, file_pointer);
	fclose(file_pointer);

	return "FILE DELETED SUCCUSSFULLY!!";
}

char *blob_get_file_memory(char *user_name)
{
	FILE *file_pointer = fopen(blob_database, "rb+");
	int file_size, file_offset, storage_offset;
	char *buffer = allocate_string_memory(buffer_size);

	file_offset = atoi(strtok(NULL, "/"));
	fseek(file_pointer, file_offset, SEEK_SET);
	fread(&storage_offset, sizeof(int), 1, file_pointer);

	fseek(file_pointer, storage_offset + data_size + sizeof(int), SEEK_SET);
	fread(&file_size, sizeof(int), 1, file_pointer);

	sprintf(buffer, "%d", file_size);
	return buffer;
}

char *blob_get_file_data(char *user_name)
{
	FILE *file_pointer = fopen(blob_database, "rb+");
	int data_count, file_offset, storage_offset,file_data_offset;
	char *buffer = allocate_string_memory(buffer_size);

	file_offset = atoi(strtok(NULL, "/"));
	data_count = atoi(strtok(NULL, "/"));

	fseek(file_pointer, file_offset, SEEK_SET);
	fread(&storage_offset, sizeof(int), 1, file_pointer);

	fseek(file_pointer, storage_offset + data_size, SEEK_SET);
	fread(&file_data_offset, sizeof(int), 1, file_pointer);

	fseek(file_pointer, blob_meta_data_size + (file_data_offset + data_count) * buffer_size, SEEK_SET);
	fread(buffer, buffer_size, 1, file_pointer);

	fclose(file_pointer);
	return buffer;
}

char *blob_downloadfile(char *user_name)
{
	char parser[data_size];
	strcpy(parser, strtok(NULL, "/"));

	if (!strcmp(parser, "FILEINFO"))
		return blob_get_file_memory(user_name);
	else if (!strcmp(parser, "FILEDATA"))
		return blob_get_file_data(user_name);
	else
		return "URL ERROR : PLEASE REPORT ABOUT ERROR";
}

char *blob_filedetails(char *user_name)
{
	char *search_name = allocate_string_memory(buffer_size);
	char *file_name, *buffer;
	int user_offset = get_blob_user_offset(user_name);
	int file_data_offset, file_offset;
	FILE *file_pointer = fopen(blob_database, "rb+");

	strcpy(search_name, strtok(NULL, "/"));

	for (int index = 0; index < blob_user_file_count; index++)
	{
		file_data_offset = user_offset + 2 * data_size + index * sizeof(int);
		fseek(file_pointer, file_data_offset, SEEK_SET);
		fread(&file_offset, sizeof(int), 1, file_pointer);
		if (file_offset)
		{
			file_name = allocate_string_memory(buffer_size);
			fseek(file_pointer, file_offset, SEEK_SET);
			fread(file_name, data_size, 1, file_pointer);
			if (!strcmp(file_name, search_name))
			{
				buffer = allocate_string_memory(buffer_size);
				sprintf(buffer, "%d", file_data_offset);
				fclose(file_pointer);
				return buffer;
			}
		}
	}

	fclose(file_pointer);
	return "NO SUCH FILE EXIST!!";
}

//////////////////////////////////////////////
//											//
//		***MESSAGE HELPER FUNCTIONS***		//
//											//
//////////////////////////////////////////////

int get_message_user_count()
{
	int user_count;
	FILE *file_pointer = fopen(message_database, "rb+");
	fread(&user_count, sizeof(int), 1, file_pointer);
	fclose(file_pointer);
	return user_count;
}

void set_message_user_count(int user_count)
{
	FILE *file_pointer = fopen(message_database, "rb+");
	fwrite(&user_count, sizeof(int), 1, file_pointer);
	fclose(file_pointer);
}

int get_message_bit_vector_position()
{
	int user_count;
	FILE *file_pointer = fopen(message_database, "rb+");
	fseek(file_pointer, sizeof(int), SEEK_SET);
	fread(&user_count, sizeof(int), 1, file_pointer);
	fclose(file_pointer);
	return user_count;
}

void set_message_bit_vector_position(int bit_vector_count)
{
	FILE *file_pointer = fopen(message_database, "rb+");
	fseek(file_pointer, sizeof(int), SEEK_SET);
	fwrite(&bit_vector_count, sizeof(int), 1, file_pointer);
	fclose(file_pointer);
}

int get_message_direction()
{
	int user_count;
	FILE *file_pointer = fopen(message_database, "rb+");
	fseek(file_pointer, 2 * sizeof(int), SEEK_SET);
	fread(&user_count, sizeof(int), 1, file_pointer);
	fclose(file_pointer);
	return user_count;
}

void set_message_direction(int direction)
{
	FILE *file_pointer = fopen(message_database, "rb+");
	fseek(file_pointer, 2 * sizeof(int), SEEK_SET);
	fwrite(&direction, sizeof(int), 1, file_pointer);
	fclose(file_pointer);
}

int message_offset_to_data_block(int offset)
{
	return message_meta_data_size + offset * message_message_block_size;
}

int message_offset_to_offset_block(int offset)
{
	return message_user_data_size + offset * sizeof(int);
}

int get_message_next_valid_offset_in_bit_vector(int data)
{
	int next_bit_vector_position, bit_vector_position = get_message_bit_vector_position();

	if (get_message_direction() == 0)
	{
		next_bit_vector_position = bit_vector_position + 1;
		if (next_bit_vector_position == message_bit_vector_blocks)
			set_message_direction(1);
	}
	else
	{
		next_bit_vector_position = bit_vector_position - 1;
		if (next_bit_vector_position == 0)
			set_message_direction(0);
	}

	set_message_bit_vector_position(next_bit_vector_position);

	FILE *file_pointer = fopen(message_database, "rb+");
	fseek(file_pointer, message_user_data_size + bit_vector_position * sizeof(int), SEEK_SET);
	fwrite(&data, sizeof(int), 1, file_pointer);
	fclose(file_pointer);

	return bit_vector_position;
}

int get_message_next_valid_offset_in_inode(int data, int category_offset)
{
	FILE *read_pointer = fopen(message_database, "rb+");
	FILE *write_pointer = fopen(message_database, "rb+");
	int index, value, offset, category_position = message_offset_to_data_block(category_offset);

	for (index = 1; index <= 60; index++)
	{
		offset = category_position + index * sizeof(int);
		fseek(read_pointer, offset, SEEK_SET);
		fread(&value, sizeof(int), 1, read_pointer);

		if (value == 0)
		{
			fseek(write_pointer, offset, SEEK_SET);
			fwrite(&data, sizeof(int), 1, write_pointer);
			fclose(read_pointer); fclose(write_pointer);
			return offset;
		}
	}

	fclose(read_pointer); fclose(write_pointer);
	return -1;
}

int get_message_user_offset(char *user_name)
{
	char user_name_from_database[data_size];
	FILE *file_pointer = fopen(message_database, "rb+");
	int user_offset, user_count = get_message_user_count();
	for (int index = 1; index <= user_count; index++)
	{
		user_offset = message_user_block_size * index;
		fseek(file_pointer, user_offset, SEEK_SET);
		fread(&user_name_from_database, data_size, 1, file_pointer);
		if (!strcmp(user_name, user_name_from_database))
		{
			fclose(file_pointer);
			return user_offset;
		}
	}
	fclose(file_pointer);
	return -1;
}

int message_select_category(char *category_name)
{
	int index, bit_vector_position, bit_vector_data, category_position;
	char *catagory;
	FILE *file_pointer = fopen(message_database, "rb+");
	for (index = 0; index < message_bit_vector_blocks; index++)
	{
		bit_vector_position = message_offset_to_offset_block(index);
		fseek(file_pointer, bit_vector_position, SEEK_SET);
		fread(&bit_vector_data, sizeof(int), 1, file_pointer);
		if (bit_vector_data == 1)
		{
			catagory = allocate_string_memory(message_message_block_size);

			fseek(file_pointer, message_offset_to_data_block(index), SEEK_SET);
			fread(&category_position, sizeof(int), 1, file_pointer);
			fseek(file_pointer, message_offset_to_data_block(category_position), SEEK_SET);
			fread(catagory, message_message_block_size, 1, file_pointer);
			if (!strcmp(catagory, category_name))
				return index;
		}
	}
	return -1;
}

int message_select_message(char *message_name,int category_offset)
{
	int index, bit_vector_position, bit_vector_data, category_position, data_position, message_offset;
	int category_block = message_offset_to_data_block(category_offset);
	char *item_data, *buffer;

	FILE *file_pointer = fopen(message_database, "rb+");

	for (index = 1; index <= 60; index++)
	{
		message_offset = category_block + index * sizeof(int);
		fseek(file_pointer, message_offset, SEEK_SET);
		fread(&bit_vector_position, sizeof(int), 1, file_pointer);

		if (bit_vector_position != 0)
		{
			item_data = allocate_string_memory(message_message_block_size);
			buffer = allocate_string_memory(buffer_size);

			data_position = message_offset_to_data_block(bit_vector_position);

			fseek(file_pointer, data_position, SEEK_SET);
			fread(&bit_vector_position, sizeof(int), 1, file_pointer);
			data_position = message_offset_to_data_block(bit_vector_position);

			fseek(file_pointer, data_position, SEEK_SET);
			fread(item_data, message_message_block_size, 1, file_pointer);

			if (!strcmp(item_data, message_name))
			{
				fclose(file_pointer);
				return index;
			}
		}
	}

	fclose(file_pointer);

	return -1;
}

//////////////////////////////////////////////
//											//
//		***MESSAGE CORE FUNCTIONS***		//
//											//
//////////////////////////////////////////////

char *message_login()
{
	int user_offset, user_count = get_message_user_count();
	char user_name[data_size], password[data_size];
	char password_from_database[data_size];

	strcpy(user_name, strtok(NULL, "/"));
	strcpy(password, strtok(NULL, "/"));

	user_offset = get_message_user_offset(user_name);
	if (user_offset != -1)
	{
		FILE *file_pointer = fopen(message_database, "rb+");
		fseek(file_pointer, user_offset + data_size, SEEK_SET);
		fread(&password_from_database, data_size, 1, file_pointer);
		fclose(file_pointer);
		if (!strcmp(password_from_database, password))
			return "VALID USER";
	}
	return "USERNAME AND PASSWORD ARE NOT MATCHED.\nPLEASE TRY AGAIN.\n";
}

char *message_signup()
{
	struct message_user_node new_user_data;
	int user_count = get_message_user_count(), user_offset;

	memset(&new_user_data, 0, sizeof(message_user_node));
	strcpy(new_user_data.user_name, strtok(NULL, "/"));
	strcpy(new_user_data.password, strtok(NULL, "/"));

	if (user_count == message_max_user_count)
		return "MAXIMUM USER COUNT REACHED\n.UNABLE TO CREATE NEW USERS.\n";
	else if (get_message_user_offset(new_user_data.user_name) != -1)
		return "USERNAME ALREADY USED. TRY WITH DIFERENT NAME.\n";
	else
	{
		FILE *file_pointer = fopen(message_database, "rb+");
		user_offset = (user_count + 1) * message_user_block_size;
		fseek(file_pointer, user_offset, SEEK_SET);
		fwrite(&new_user_data, sizeof(message_user_node), 1, file_pointer);
		fclose(file_pointer);
		set_message_user_count(++user_count);

		return "NEW USER CREATED SUCCUSSFULLY.\n";
	}
}
 
char *message_create(char *user_name)
{
	int inode_offset,data_offset;
	char *message_type = allocate_string_memory(buffer_size);
	char *message = allocate_string_memory(message_message_block_size);

	strcpy(message_type, strtok(NULL, "/"));

	if (!strcmp(message_type, "CATAGORY"))
	{
		strcpy(message, strtok(NULL, "/"));

		if (message_select_category(message) == -1)
		{
			inode_offset = get_message_next_valid_offset_in_bit_vector(one);
			data_offset = get_message_next_valid_offset_in_bit_vector(inode_offset);

			FILE *file_pointer = fopen(message_database, "rb+");

			fseek(file_pointer, message_offset_to_data_block(inode_offset), SEEK_SET);
			fwrite(&data_offset, sizeof(int), 1, file_pointer);

			fseek(file_pointer, message_offset_to_data_block(data_offset), SEEK_SET);
			fwrite(message, message_message_block_size, 1, file_pointer);

			fclose(file_pointer);

			return "NEW CATAGORY CREATED SUCCESSFULLY!!";
		}
		else
			return "THIS CATAGORY ALREADY CREATED!!";
	}
	else if (!strcmp(message_type, "MESSAGE"))
	{
		int catagory_offset, message_offset;
		strcpy(message, strtok(NULL, "/"));
		catagory_offset = atoi(strtok(NULL, "/"));

		if (message_select_message(message, catagory_offset) == -1)
		{
			inode_offset = get_message_next_valid_offset_in_bit_vector(catagory_offset);
			data_offset = get_message_next_valid_offset_in_bit_vector(catagory_offset);
			message_offset = get_message_next_valid_offset_in_inode(inode_offset, catagory_offset);

			FILE *file_pointer = fopen(message_database, "rb+");

			fseek(file_pointer, message_offset_to_data_block(inode_offset), SEEK_SET);
			fwrite(&data_offset, sizeof(int), 1, file_pointer);

			fseek(file_pointer, message_offset_to_data_block(data_offset), SEEK_SET);
			fwrite(message, message_message_block_size, 1, file_pointer);

			fclose(file_pointer);

			return "NEW MESSAGE CREATED SUCCESSFULLY!!";
		}
		else
			return "THIS MESSAGE IS ALREADY CREATED";

	}
	else if (!strcmp(message_type, "COMMENT"))
	{
		int message_offset, comment_offset;
		strcpy(message, strtok(NULL, "/"));
		message_offset = atoi(strtok(NULL, "/"));

		if (message_select_message(message, message_offset) == -1)
		{
			inode_offset = get_message_next_valid_offset_in_bit_vector(message_offset);
			data_offset = get_message_next_valid_offset_in_bit_vector(message_offset);
			comment_offset = get_message_next_valid_offset_in_inode(inode_offset, message_offset);

			FILE *file_pointer = fopen(message_database, "rb+");

			fseek(file_pointer, message_offset_to_data_block(inode_offset), SEEK_SET);
			fwrite(&data_offset, sizeof(int), 1, file_pointer);

			fseek(file_pointer, message_offset_to_data_block(data_offset), SEEK_SET);
			fwrite(message, message_message_block_size, 1, file_pointer);

			fclose(file_pointer);

			return "NEW COMMENT CREATED SUCCESSFULLY!!";
		}
		else
			return "THIS COMMENT IS ALREADY CREATED";
	}
	else
		return "404 - NOT - FOUND\n";
		
}

char *message_select(char *user_name)
{
	char *message_type = allocate_string_memory(buffer_size);
	char *message = allocate_string_memory(message_message_block_size);
	char *buffer = allocate_string_memory(buffer_size);

	strcpy(message_type, strtok(NULL, "/"));
	strcpy(message, strtok(NULL,"/"));

	if (!strcmp(message_type, "CATAGORY"))
	{
		int catagory_inode_offset = message_select_category(message);
		if (catagory_inode_offset != -1)
		{
			sprintf(buffer, "%d", catagory_inode_offset);
			return buffer;
		}
		else
			return "NO SUCH CATAGORY AVALIABLE!!";
	}
	else if (!strcmp(message_type, "MESSAGE"))
	{
		int category_block, message_category_position, inode_offset;
		int category_offset = atoi(strtok(NULL, "/"));
		int message_category_offset = message_select_message(message,category_offset);
		
		if (message_category_offset != -1)
		{
			FILE *file_pointer = fopen(message_database, "rb+");

			category_block = message_offset_to_data_block(category_offset);
			message_category_position = category_block + message_category_offset * sizeof(int);

			fseek(file_pointer, message_category_position, SEEK_SET);
			fread(&inode_offset, sizeof(int), 1, file_pointer);

			sprintf(buffer, "%d", inode_offset);
			return buffer;
		}
		else
			return "NO SUCH MESSAGE AVALIABLE!!";
	}
}

char *message_retrive(char *user_name)
{
	int index, bit_vector_position, bit_vector_data;
	int position, count = 0;
	char *result = allocate_string_memory(buffer_size);
	char *parser = allocate_string_memory(buffer_size);
	char *item_data, *buffer;


	strcpy(parser, strtok(NULL, "/"));

	if (!strcmp(parser, "CATAGORY"))
	{
		FILE *file_pointer = fopen(message_database, "rb+");

		for (index = 0; index < message_bit_vector_blocks; index++)
		{
			bit_vector_position = message_offset_to_offset_block(index);

			fseek(file_pointer, bit_vector_position, SEEK_SET);
			fread(&bit_vector_data, sizeof(int), 1, file_pointer);
			if (bit_vector_data == 1)
			{
				item_data = allocate_string_memory(message_message_block_size);
				buffer = allocate_string_memory(buffer_size);

				fseek(file_pointer, message_offset_to_data_block(index), SEEK_SET);
				fread(&position, sizeof(int), 1, file_pointer);
		
				fseek(file_pointer, message_offset_to_data_block(position), SEEK_SET);
				fread(item_data, message_message_block_size, 1, file_pointer);

				sprintf(buffer, "%c %s\n", 4, item_data);
				strcat(result, buffer);
				count += 1;
			}
		}

		fclose(file_pointer);

		if (count == 0)
			return "NO CATEGORIES CREATED YET!!\n";
		else
			return result;
	}
	else if (!strcmp(parser, "MESSAGE"))
	{
		int category_offset = atoi(strtok(NULL, "/"));
		int category_block = message_offset_to_data_block(category_offset);
		int data_position;

		FILE *file_pointer = fopen(message_database, "rb+");

		for (index = 1; index <= 60; index++)
		{
			fseek(file_pointer, category_block + index *sizeof(int), SEEK_SET);
			fread(&bit_vector_position, sizeof(int), 1, file_pointer);

			if (bit_vector_position != 0)
			{
				item_data = allocate_string_memory(message_message_block_size);
				buffer = allocate_string_memory(buffer_size);

				data_position = message_offset_to_data_block(bit_vector_position);

				fseek(file_pointer, data_position, SEEK_SET);
				fread(&bit_vector_position, sizeof(int), 1, file_pointer);
				data_position = message_offset_to_data_block(bit_vector_position);

				fseek(file_pointer, data_position, SEEK_SET);
				fread(item_data, message_message_block_size, 1, file_pointer);

				sprintf(buffer, "%c %s\n", 4, item_data);
				strcat(result, buffer);
				count += 1;
			}
		}

		fclose(file_pointer);

		if (count == 0)
			return "NO MESSAGES CREATED YET!!\n";
		else
			return result;
	}
	else if (!strcmp(parser, "COMMENT"))
	{
		int message_offset = atoi(strtok(NULL, "/"));
		int message_block = message_offset_to_data_block(message_offset);
		int data_position;

		FILE *file_pointer = fopen(message_database, "rb+");

		for (index = 1; index <= 60; index++)
		{
			fseek(file_pointer, message_block + index *sizeof(int), SEEK_SET);
			fread(&bit_vector_position, sizeof(int), 1, file_pointer);

			if (bit_vector_position != 0)
			{
				item_data = allocate_string_memory(message_message_block_size);
				buffer = allocate_string_memory(buffer_size);

				data_position = message_offset_to_data_block(bit_vector_position);

				fseek(file_pointer, data_position, SEEK_SET);
				fread(&bit_vector_position, sizeof(int), 1, file_pointer);
				data_position = message_offset_to_data_block(bit_vector_position);

				fseek(file_pointer, data_position, SEEK_SET);
				fread(item_data, message_message_block_size, 1, file_pointer);

				sprintf(buffer, "%c %s\n", 4, item_data);
				strcat(result, buffer);
				count += 1;
			}
		}

		fclose(file_pointer);

		if (count == 0)
			return "NO COMMENTS CREATED YET!!\n";
		else
			return result;
	}
	else
		return "404 - NOT - FOUND\n";
}

char *message_delete(char *user_name)
{
	int category_offset, category_block, message_category_offset, message_category_position, inode_offset, message_offset, message_block;
	char *message = allocate_string_memory(buffer_size);
	char *comment = allocate_string_memory(buffer_size);
	char *parser = allocate_string_memory(buffer_size);


	strcpy(parser, strtok(NULL, "/"));

	if (!strcmp("MESSAGE", parser))
	{
		strcpy(message, strtok(NULL, "/"));
		category_offset = atoi(strtok(NULL, "/"));

		message_category_offset = message_select_message(message, category_offset);
		if (message_category_offset != -1)
		{
			FILE *file_pointer = fopen(message_database, "rb+");

			category_block = message_offset_to_data_block(category_offset);
			message_category_position = category_block + message_category_offset * sizeof(int);

			fseek(file_pointer, message_category_position, SEEK_SET);
			fread(&inode_offset, sizeof(int), 1, file_pointer);

			fseek(file_pointer, message_category_position, SEEK_SET);
			fwrite(&zero, sizeof(int), 1, file_pointer);

			fseek(file_pointer, message_offset_to_offset_block(inode_offset), SEEK_SET);
			fwrite(&zero, sizeof(int), 1, file_pointer);

			fclose(file_pointer);

			return "MESSAGE DELETED SUCCESSFULLY!!\n";
		}
		else
			return "NO SUCH MESSAGE AVALIABLE!!\n";
	}
	else if (!strcmp("COMMENT", parser))
	{
		strcpy(comment, strtok(NULL, "/"));
		message_offset = atoi(strtok(NULL, "/"));

		message_category_offset = message_select_message(comment, message_offset);
		if (message_category_offset != -1)
		{
			FILE *file_pointer = fopen(message_database, "rb+");

			message_block = message_offset_to_data_block(message_offset);
			message_category_position = message_block + message_category_offset * sizeof(int);

			fseek(file_pointer, message_category_position, SEEK_SET);
			fread(&inode_offset, sizeof(int), 1, file_pointer);

			fseek(file_pointer, message_category_position, SEEK_SET);
			fwrite(&zero, sizeof(int), 1, file_pointer);

			fseek(file_pointer, message_offset_to_offset_block(inode_offset), SEEK_SET);
			fwrite(&zero, sizeof(int), 1, file_pointer);

			fclose(file_pointer);

			return "MESSAGE DELETED SUCCESSFULLY!!\n";
		}
		else
			return "NO SUCH MESSAGE AVALIABLE!!\n";
	}
	else
		return "404 - NOT - FOUND\n";
}

//////////////////////////////////////////////
//											//
//		***CALENDER HELPER FUNCTIONS***		//
//											//
//////////////////////////////////////////////

int get_calender_user_count(int category)
{
	int user_count;

	FILE *file_pointer = fopen(calender_database, "rb+");
	fseek(file_pointer, (category - 1) * calender_category_data_size, SEEK_SET);
	fread(&user_count, sizeof(int), 1, file_pointer);
	fclose(file_pointer);

	return user_count;
}

void set_calender_user_count(int category, int user_count)
{
	FILE *file_pointer = fopen(calender_database, "rb+");
	fseek(file_pointer, (category - 1) * calender_category_data_size, SEEK_SET);
	fwrite(&user_count, sizeof(int), 1, file_pointer);
	fclose(file_pointer);
}

int get_calender_user_offset(int category, char *service_man_name)
{
	int index, user_offset, category_offset,user_count;
	char *service_man_name_from_database;
	FILE *file_pointer = fopen(calender_database, "rb+");

	category_offset = (category - 1) * calender_category_data_size;
	user_count = get_calender_user_count(category);

	for (index = 1; index <= user_count; index++)
	{
		fseek(file_pointer, category_offset + index * calender_user_block_size, SEEK_SET);
		user_offset = ftell(file_pointer);
		service_man_name_from_database = allocate_string_memory(data_size);
		fread(service_man_name_from_database, data_size, 1, file_pointer);
		if (!strcmp(service_man_name_from_database, service_man_name))
		{
			fclose(file_pointer);
			return user_offset;
		}
	}

	fclose(file_pointer);
	return -1;
}

//////////////////////////////////////////////
//											//
//		***CALENDER CORE FUNCTIONS***		//
//											//
//////////////////////////////////////////////

char *calender_add(int category)
{
	int user_count;
	struct calender_service_man_details new_service_man;
	FILE *file_pointer = fopen(calender_database, "rb+");

	memset(&new_service_man, '\0', sizeof(struct calender_service_man_details));

	strcpy(new_service_man.name, strtok(NULL, "/"));
	strcpy(new_service_man.number, strtok(NULL, "/"));
	strcpy(new_service_man.address, strtok(NULL, "/"));
	user_count = get_calender_user_count(category);

	if (user_count == calender_max_user_count)
		return "MAXIMUM USER COUNT REACHED\n.UNABLE TO CREATE NEW USERS.\n";
	else if (get_calender_user_offset(category, new_service_man.name) != -1)
		return "USERNAME ALREADY USED. TRY WITH DIFERENT NAME.\n";
	else
	{
		user_count = get_calender_user_count(category);
		fseek(file_pointer, (category - 1) * calender_category_data_size + (user_count + 1) * calender_user_block_size, SEEK_SET);
		fwrite(&new_service_man, sizeof(struct calender_service_man_details), 1, file_pointer);
		fclose(file_pointer);

		set_calender_user_count(category,user_count + 1);

		return "NEW SERVICE CREATED SUCCUSSFULLY";
	}

	return NULL;
}

char *calender_view(int category)
{
	char *result = allocate_string_memory(buffer_size), *user_name, *buffer;
	int index, category_offset, user_count;
	char *service_man_name_from_database;
	FILE *file_pointer = fopen(calender_database, "rb+");

	category_offset = (category - 1) * calender_category_data_size;
	user_count = get_calender_user_count(category);

	if (user_count == 0)
		return "NO SERVICE MAN CREATED YET!!\n";

	for (index = 1; index <= user_count; index++)
	{
		user_name = allocate_string_memory(data_size);
		buffer = allocate_string_memory(data_size);

		fseek(file_pointer, category_offset + index * calender_user_block_size, SEEK_SET);
		fread(user_name, data_size, 1, file_pointer);
		sprintf(buffer, "%c %s\n", 4, user_name);
		strcat(result, buffer);
	}

	fclose(file_pointer);
	return result;
}

char *calender_select(int category)
{
	int user_offset;
	char *parser = allocate_string_memory(buffer_size);
	char *serviceman_name = allocate_string_memory(data_size);
	char *result, *date, *date_parser;
	FILE *read_pointer = fopen(calender_database, "rb+");
	FILE *write_pointer = fopen(calender_database, "rb+");

	strcpy(parser, strtok(NULL, "/"));

	if (!strcmp(parser, "NAME"))
	{
		strcpy(serviceman_name, strtok(NULL, "/"));
		user_offset = get_calender_user_offset(category, serviceman_name);
		result = allocate_string_memory(data_size);
		if(user_offset==-1)
			return "NO SUCH SERVICE MAN EXIST!!";
		else
		{
			sprintf(result, "%d", user_offset);
			return result;
		}
	}
	else if (!strcmp(parser, "NUMBER"))
	{
		user_offset = atoi(strtok(NULL, "/"));
		fseek(read_pointer, user_offset + data_size, SEEK_SET);
		result = allocate_string_memory(data_size);
		fread(result, data_size, 1, read_pointer);
		fclose(read_pointer);

		return result;
	}
	else if (!strcmp(parser, "ADDRESS"))
	{
		user_offset = atoi(strtok(NULL, "/"));
		fseek(read_pointer, user_offset + 2 * data_size, SEEK_SET);
		result = allocate_string_memory(2 * data_size);
		fread(result, 2 * data_size, 1, read_pointer);
		fclose(read_pointer);

		return result;
	}
	else if (!strcmp(parser, "DATE"))
	{
		SYSTEMTIME datetime;
		struct calender_date applied_date, mark_date, current_date;
		int day_block,day_status,advance_time;
		date = allocate_string_memory(buffer_size);

		GetSystemTime(&datetime);
		current_date.day = datetime.wDay;
		current_date.month = datetime.wMonth;
		current_date.year = datetime.wYear;
		
		user_offset = atoi(strtok(NULL, "/"));
		strcpy(date, strtok(NULL, "/"));

		sscanf(date, "%d-%d-%d", &applied_date.day, &applied_date.month, &applied_date.year);
		advance_time = get_difference_between_date(current_date, applied_date);
		if (is_valid_date(applied_date))
		{
			if (advance_time > 1 && advance_time < 180)
			{
				sscanf(initial_date, "%d-%d-%d", &mark_date.day, &mark_date.month, &mark_date.year);
				day_block = get_difference_between_date(mark_date, applied_date) % 180;

				fseek(read_pointer, user_offset + 4 * data_size + day_block * sizeof(int), SEEK_SET);
				fread(&day_status, sizeof(int), 1, read_pointer);
				if (day_status == 1)
					return "SLOT IS NOT EMPTY.\n";
				else
				{
					fseek(write_pointer, user_offset + 4 * data_size + day_block * sizeof(int), SEEK_SET);
					fwrite(&one, sizeof(int), 1, write_pointer);
					fclose(write_pointer);
					return "SLOT IS BOOKED.\n";
				}
			}
			else
				return "UNABLE TO BOOK AT GIVEN SLOT";
		}
		else
			return "INVALID DATE!!\n";
	}
	else
		return "404 - NOT - FOUND\n";
}

//////////////////////////////////////////////
//											//
//		***HAMARA GAR FUNCTIONS***			//
//											//
//////////////////////////////////////////////

char *blob_action()
{
	char parser[data_size], user_name[data_size];
	strcpy(user_name, strtok(NULL, "/"));
	strcpy(parser, strtok(NULL, "/"));

	if (!strcmp(parser, "VIEWFILES"))
		return blob_viewfiles(user_name);
	else if (!strcmp(parser, "ADDFILE"))
		return blob_addfile(user_name);
	else if (!strcmp(parser, "GETDETAILS"))
		return blob_filedetails(user_name);
	else if (!strcmp(parser, "DELETEFILE"))
		return blob_deletefile(user_name);
	else if (!strcmp(parser, "DOWNLOADFILE"))
		return blob_downloadfile(user_name);
	else
		return "404 - NOT - FOUND\n";
}

char *message_action()
{
	char parser[data_size], user_name[data_size];
	strcpy(user_name, strtok(NULL, "/"));
	strcpy(parser, strtok(NULL, "/"));

	if (!strcmp(parser, "CREATE"))
		return message_create(user_name);
	else if (!strcmp(parser, "SELECT"))
		return message_select(user_name);
	else if (!strcmp(parser, "RETRIVE"))
		return message_retrive(user_name);
	else if (!strcmp(parser, "DELETE"))
		return message_delete(user_name);
	else
		return "404 - NOT - FOUND\n";
}

char *blob()
{
	char parser[data_size];
	strcpy(parser, strtok(NULL, "/"));

	if (!strcmp(parser, "LOGIN"))
		return blob_login();
	else if (!strcmp(parser, "SIGNUP"))
		return blob_signup();
	else if (!strcmp(parser, "ACTION"))
		return blob_action();
	else
		return "404 - NOT - FOUND\n";
}

char *message()
{
	char parser[data_size];
	strcpy(parser, strtok(NULL, "/"));

	if (!strcmp(parser, "LOGIN"))
		return message_login();
	else if (!strcmp(parser, "SIGNUP"))
		return message_signup();
	else if (!strcmp(parser, "ACTION"))
		return message_action();
	else
		return "404 - NOT - FOUND\n";
}

char *calender()
{
	int category;
	char parser[data_size];

	category = atoi(strtok(NULL, "/"));
	strcpy(parser, strtok(NULL, "/"));

	if (!strcmp(parser, "ADD"))
		return calender_add(category);
	if (!strcmp(parser, "VIEW"))
		return calender_view(category);
	else if (!strcmp(parser, "SELECT"))
		return calender_select(category);
	else
		return "404 - NOT - FOUND\n";
}

char *execute_command(char *command)
{
	char parser[data_size];
	strcpy(parser, strtok(command, "/"));

	if (!strcmp(parser, "BLOB"))
		return blob();
	else if (!strcmp(parser, "MESSAGE"))
		return message();
	else if (!strcmp(parser, "CALENDER"))
		return calender();
	else
		return "404 - NOT - FOUND\n";
}

void process_message(int *csock)
{
	char *message, *responce;
	message = allocate_string_memory(buffer_size + 128);
	responce = allocate_string_memory(buffer_size + 128);
	strcpy(message, receive_message(csock));
	strcpy(responce, execute_command(message));
	send_message(csock, responce);
}

//////////////////////////////////////////////
//											//
//	    ***SERVER THREAD FUNCTIONS***		//
//											//
//////////////////////////////////////////////

DWORD WINAPI SocketHandler(void* lp)
{
	int *csock = (int*)lp;
	process_message(csock);
	return 0;
}