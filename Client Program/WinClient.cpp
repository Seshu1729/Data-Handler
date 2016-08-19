#include "stdafx.h"
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <conio.h>

#define buffer_size 1024
#define block_size 256
#define page_size 5
#define data_size 32
#define loader_size 50
#define clrscr() system("clear")
#define file_address_extention "documents/"
#define download_file_address_extention "downloads/"


//////////////////////////////////////////////
//											//
//		  ***GLOBAL VARIABLES***			//
//											//
//////////////////////////////////////////////


int host_port = 1101;
char *host_name = "127.0.0.1";

unsigned short wVersionRequested;
WSADATA wsaData;
int err;

struct sockaddr_in my_addr;
int buffer_len = 1024, bytecount;

int hsock;

char buffer[buffer_size], *responce;

//////////////////////////////////////////////
//											//
//		  ***CLIENT FUNCTIONS***			//
//											//
//////////////////////////////////////////////

int getsocket()
{
	int hsock;
	int * p_int;
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if (hsock == -1)
	{
		printf("Error initializing socket %d\n", WSAGetLastError());
		return -1;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if ((setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1) ||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1))
	{
		printf("Error setting options %d\n", WSAGetLastError());
		free(p_int);
		return -1;
	}
	free(p_int);

	return hsock;
}

void send_message(char *buffer)
{

	hsock = getsocket();
	if (connect(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == SOCKET_ERROR)
	{
		fprintf(stderr, "Error connecting socket %d\n", WSAGetLastError());
		return;
	}

	if ((bytecount = send(hsock, buffer, strlen(buffer), 0)) == SOCKET_ERROR)
	{
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		return;
	}
}

char *receive_message()
{
	char buffer[buffer_size];
	memset(buffer, '\0', buffer_size);
	if ((bytecount = recv(hsock, buffer, buffer_len, 0)) == SOCKET_ERROR)
	{
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		return NULL;
	}

	closesocket(hsock);
	return buffer;
}

char *process_message(char *buffer)
{
	char *return_buffer;
	send_message(buffer);
	return_buffer = receive_message();
	return return_buffer;
}


//////////////////////////////////////////////
//											//
//		  ***HEALPER FUNCTIONS***			//
//											//
//////////////////////////////////////////////

int get_file_size(char *file_address)
{
	int file_size_in_bytes,file_size;
	FILE *file_pointer = fopen(file_address, "r"); 

	if (file_pointer != NULL)
	{
		fseek(file_pointer, 0, SEEK_END);
		file_size_in_bytes = ftell(file_pointer);
		fclose(file_pointer);

		file_size = file_size_in_bytes / 1024;
		if (file_size_in_bytes % 1024)
			file_size++;
		return file_size;
	}

	return -1;
}

char *allocate_string_memory(int string_size)
{
	char *string;

	string = (char *)malloc(string_size);
	memset(string, '\0', string_size);

	return string;
}

int update_loader(char *loader, int total_count, int current_count, int filled_count)
{
	int index;

	if (total_count < loader_size)
	{
		for (index = 0; index < loader_size / total_count && filled_count < loader_size; index++)
		{
			loader[filled_count++] = 219;
			printf("\r%s", loader);
		}
	}
	else
	{
		if (current_count % (total_count / loader_size) == 0 && filled_count < loader_size)
		{
			loader[filled_count++] = 219;
			printf("\r%s", loader);
		}
	}
	return filled_count;
}

//////////////////////////////////////////////
//											//
//		    ***BLOB FUNCTIONS***			//
//											//
//////////////////////////////////////////////

void blob_viewfiles(char *user_name)
{
	clrscr();

	char buffer[buffer_size];
	sprintf(buffer, "%s/%s/%s/", "BLOB/ACTION", user_name, "VIEWFILES");
	responce = process_message(buffer);
	puts(responce);
}

void blob_addfile(char *user_name)
{
	clrscr();

	int index, filled_count, file_size, file_offset;
	char *file_data, *buffer;
	char *file_name = allocate_string_memory(data_size);
	char *file_address = allocate_string_memory(buffer_size);
	char uploader[51] = "__________________________________________________";

	printf("ENTER FILE NAME::");
	fflush(stdin); gets(file_name);

	strcat(file_address, file_address_extention);
	strcat(file_address, file_name);
	file_size = get_file_size(file_address);

	if (file_size == -1)
		printf("IOERROR : FILE NOT FOUND.\n");
	else if (file_size == 0)
		printf("IOERROR : EMPTY FILE SELECTED.\n");
	else
	{
		buffer = allocate_string_memory(buffer_size + 128);
		sprintf(buffer, "%s/%s/%s/%s/%d/", "BLOB/ACTION", user_name, "ADDFILE/FILEINFO", file_name, file_size);
		responce = process_message(buffer);
		file_offset = atoi(responce);

		if (file_offset > 0 || !strcmp(responce, "0"))
		{
			printf("UPLOADING FILE...\n");

			FILE *file_pointer = fopen(file_address, "rb+");
			printf("\n%s", uploader);

			for (index = 1,filled_count = 0; index <= file_size; index++)
			{
				file_data = allocate_string_memory(buffer_size + 128);
				buffer = allocate_string_memory(buffer_size + 128);

				fread(file_data, buffer_size, 1, file_pointer);
				sprintf(buffer, "%s/%s/%s/%d/%d/%s/", "BLOB/ACTION", user_name, "ADDFILE/FILEDATA", file_offset, index-1, file_data);
				responce = process_message(buffer);

				filled_count = update_loader(uploader, file_size, index, filled_count);
				fseek(file_pointer, buffer_size * index, SEEK_SET);
			}

			printf("\n\nFILE UPLOADING COMPLECTED.\n");
		}
		else
			printf("MEMORY ERROR : FAILED TO ALLOCATE MEMORY.\n");
	}
}

void blob_deletefile(char *user_name,int offset)
{
	clrscr();

	char file_name[16], buffer[buffer_size];

	sprintf(buffer, "%s/%s/%s/%d/", "BLOB/ACTION", user_name, "DELETEFILE", offset);
	responce = process_message(buffer);
	puts(responce);
}

void blob_downloadfile(char *user_name, char *file_name,int offset)
{
	clrscr();

	int index, filled_count, file_size;
	char *buffer, *responce;
	char *file_address = allocate_string_memory(data_size);
	char downloader[51] = "__________________________________________________";

	strcat(file_address, download_file_address_extention);
	strcat(file_address, file_name);

	buffer = allocate_string_memory(buffer_size + 128);
	responce = allocate_string_memory(buffer_size + 128);
	sprintf(buffer, "%s/%s/%s/%d/", "BLOB/ACTION", user_name,"DOWNLOADFILE/FILEINFO", offset);
	strcpy(responce,process_message(buffer));
	file_size = atoi(responce);

	if (file_size != 0)
	{
		printf("DOWNLOADING FILE...\n");

		FILE *file_pointer = fopen(file_address, "ab+");
		fseek(file_pointer, 0, SEEK_SET);
		printf("\n%s", downloader);

		for (index = 1, filled_count = 0; index <= file_size;index++)
		{
			buffer = allocate_string_memory(buffer_size + 128);
			responce = allocate_string_memory(buffer_size + 128);

			sprintf(buffer, "%s/%s/%s/%d/%d/", "BLOB/ACTION", user_name, "DOWNLOADFILE/FILEDATA", offset, index-1);
			strcpy(responce, process_message(buffer));
			fwrite(responce, strlen(responce), 1, file_pointer);
			filled_count = update_loader(downloader, file_size, index, filled_count);
		}
		printf("\n\nFILE DOWNLOADING COMPLECTED.\n");
	}
	else
		puts(responce);
}

void blob_selectfile(char *user_name, char *file_name, int offset)
{
	clrscr();

	int option;

	printf("YOU CAN MAKE FOLLOWING OPERATIONS ON FILES\n");
	printf("1.DOWNLOAD FILE\n2.DELETE FILE\n3.RETURN\n");
	printf("ENTER YOUR OPTION::");
	scanf("%d", &option);

	if (option == 1)
		blob_downloadfile(user_name,file_name,offset);
	else if (option == 2)
		blob_deletefile(user_name,offset);
	else if (option == 3)
		/* DO NOTHING */;
	else
		printf("PLEASE ENTER VALID OPTION.\n");
}

void blob_home_page(char *user_name)
{
	char *file_name, *buffer, *responce;
	int option, offset;

	while (1)
	{
		clrscr();

		blob_viewfiles(user_name);
		
		printf("1.ADD NEW FILE\n2.SELECT FILE\n3.LOGOUT\n");
		printf("ENTER YOUR OPTION::");
		scanf("%d", &option);

		if (option == 1)
			blob_addfile(user_name);
		else if (option == 2)
		{
			file_name = allocate_string_memory(data_size);
			buffer = allocate_string_memory(buffer_size);
			responce = allocate_string_memory(buffer_size);

			printf("ENTER FILE NAME::");
			fflush(stdin); gets(file_name);

			sprintf(buffer, "%s/%s/%s/%s/", "BLOB/ACTION", user_name,"GETDETAILS", file_name);
			strcpy(responce, process_message(buffer));
			offset = atoi(responce);
			if (offset != 0)
				blob_selectfile(user_name, file_name, offset);
			else
				puts(responce);
		}
		else if (option == 3)
			break;
		else
			printf("PLEASE ENTER VALID OPTION.\n");
		printf("ENTER ANY KEY TO CONTINUE...");
		fflush(stdin); getchar();
	}
}


//////////////////////////////////////////////
//											//
//		    ***MESSAGE FUNCTIONS***			//
//											//
//////////////////////////////////////////////

void message_message_page(char *user_name, int message_offset)
{
	int option;
	char *buffer, *responce, *message;

	while (1)
	{
		clrscr();

		buffer = allocate_string_memory(buffer_size);
		responce = allocate_string_memory(buffer_size);

		sprintf(buffer, "%s/%s/%s/%d/", "MESSAGE/ACTION", user_name, "RETRIVE/COMMENT", message_offset);
		strcpy(responce, process_message(buffer));
		puts(responce);

		printf("1.ADD COMMENT\n2.DELETE COMMENT\n3.RETURN\n");
		printf("ENTER YOUR OPTION::");
		scanf("%d", &option);

		if (option == 1)
		{
			clrscr();

			buffer = allocate_string_memory(buffer_size);
			message = allocate_string_memory(buffer_size);
			responce = allocate_string_memory(buffer_size);

			printf("ENTER COMMENT::");
			fflush(stdin); gets(message);

			sprintf(buffer, "%s/%s/%s/%s/%d/", "MESSAGE/ACTION", user_name, "CREATE/COMMENT", message, message_offset);
			strcpy(responce, process_message(buffer));
			puts(responce);
		}
		else if (option == 2)
		{
			buffer = allocate_string_memory(buffer_size);
			message = allocate_string_memory(buffer_size);
			responce = allocate_string_memory(buffer_size);

			printf("ENTER MESSAGE::");
			fflush(stdin); gets(message);

			sprintf(buffer, "%s/%s/%s/%s/%d/", "MESSAGE/ACTION", user_name, "DELETE/COMMENT", message, message_offset);
			strcpy(responce, process_message(buffer));
			puts(responce);
		}
		else if (option == 3)
			break;
		else
			printf("PLEASE ENTER VALID OPTION.\n");
		printf("ENTER ANY KEY TO CONTINUE...");
		fflush(stdin); getchar();
	}
}

void message_category_page(char *user_name,int catagory_offset)
{
	int option;
	char *buffer, *responce, *message;

	while (1)
	{
		clrscr();

		buffer = allocate_string_memory(buffer_size);
		responce = allocate_string_memory(buffer_size);

		sprintf(buffer, "%s/%s/%s/%d/", "MESSAGE/ACTION", user_name, "RETRIVE/MESSAGE", catagory_offset);
		strcpy(responce, process_message(buffer));
		puts(responce);

		printf("1.ADD MESSAGE\n2.SELECT MESSAGE\n3.DELETE MESSAGE\n4.RETURN\n");
		printf("ENTER YOUR OPTION::");
		scanf("%d", &option);

		if (option == 1)
		{
			clrscr();

			buffer = allocate_string_memory(buffer_size);
			message = allocate_string_memory(buffer_size);
			responce = allocate_string_memory(buffer_size);

			printf("ENTER MESSAGE::");
			fflush(stdin); gets(message);

			sprintf(buffer, "%s/%s/%s/%s/%d/", "MESSAGE/ACTION", user_name, "CREATE/MESSAGE", message, catagory_offset);
			strcpy(responce, process_message(buffer));
			puts(responce);
		}
		else if (option == 2)
		{
			int message_offset;

			buffer = allocate_string_memory(buffer_size);
			message = allocate_string_memory(buffer_size);

			printf("ENTER MESSAGE::");
			fflush(stdin); gets(message);

			sprintf(buffer, "%s/%s/%s/%s/%d/", "MESSAGE/ACTION", user_name, "SELECT/MESSAGE", message, catagory_offset);
			strcpy(responce, process_message(buffer));
			message_offset = atoi(responce);

			if (message_offset > 0)
				message_message_page(user_name, message_offset);
			else
				puts(responce);
			
		}
		else if (option == 3)
		{
			buffer = allocate_string_memory(buffer_size);
			message = allocate_string_memory(buffer_size);
			responce = allocate_string_memory(buffer_size);

			printf("ENTER MESSAGE::");
			fflush(stdin); gets(message);

			sprintf(buffer, "%s/%s/%s/%s/%d/", "MESSAGE/ACTION", user_name, "DELETE/MESSAGE", message, catagory_offset);
			strcpy(responce, process_message(buffer));
			puts(responce);
		}
		else if (option == 4)
			break;
		else
			printf("PLEASE ENTER VALID OPTION.\n");
		printf("ENTER ANY KEY TO CONTINUE...");
		fflush(stdin); getchar();
	}
}

void message_home_page(char *user_name)
{
	int option, message_option, catagory_offset, message_offset;
	char *category_name, *responce, *buffer;

	while(1)
	{
		clrscr();

		buffer = allocate_string_memory(buffer_size);
		sprintf(buffer, "%s%s%s%d", "MESSAGE/ACTION/", user_name, "/RETRIVE/CATAGORY/");
		responce = allocate_string_memory(buffer_size);
		strcpy(responce, process_message(buffer));
		puts(responce);

		printf("1.ADD CATAGORY\n2.SELECT CATAGORY\n3.RETURN\n");
		printf("ENTER YOUR OPTION::");
		scanf("%d", &option);

		if (option == 1)
		{
			clrscr();

			category_name = allocate_string_memory(buffer_size);
			printf("ENTER CATAGORY NAME::");
			fflush(stdin); gets(category_name);

			buffer = allocate_string_memory(buffer_size);
			sprintf(buffer, "%s/%s/%s/%s/", "MESSAGE/ACTION", user_name, "CREATE/CATAGORY", category_name);
			responce = allocate_string_memory(buffer_size);
			strcpy(responce, process_message(buffer));
			puts(responce);
		}
		else if (option == 2)
		{
			category_name = allocate_string_memory(buffer_size);
			responce = allocate_string_memory(buffer_size);

			printf("ENTER CATAGORY NAME::");
			fflush(stdin); gets(category_name);

			buffer = allocate_string_memory(buffer_size);
			sprintf(buffer, "%s/%s/%s/%s/", "MESSAGE/ACTION", user_name, "SELECT/CATAGORY", category_name);
			strcpy(responce, process_message(buffer));
			catagory_offset = atoi(responce);

			if (catagory_offset > 0)
				message_category_page(user_name, catagory_offset);
			else
				puts(responce);
			
		}
		else if (option == 3)
			break;
		else
			printf("PLEASE ENTER VALID OPTION.\n");
		printf("ENTER ANY KEY TO CONTINUE...");
		fflush(stdin); getchar();
	}
}


//////////////////////////////////////////////
//											//
//		   ***CALENDER FUNCTIONS***			//
//											//
//////////////////////////////////////////////

void calender_viewserviceman(int category)
{
	char *buffer = allocate_string_memory(buffer_size);
	char *responce = allocate_string_memory(buffer_size);

	sprintf(buffer, "%s/%d/%s/", "CALENDER", category, "VIEW");
	strcpy(responce, process_message(buffer));

	puts(responce);
}

void calender_addserviceman(int category)
{
	clrscr();

	char *serviceman_name = allocate_string_memory(data_size);
	char *serviceman_number = allocate_string_memory(data_size);
	char *serviceman_address = allocate_string_memory(2 * data_size);
	char *buffer = allocate_string_memory(buffer_size);
	char *responce = allocate_string_memory(buffer_size);

	printf("ENTER SERVICE MAN NAME::");
	fflush(stdin); gets(serviceman_name);

	printf("ENTER SERVICE MAN NUMBER::");
	fflush(stdin); gets(serviceman_number);

	printf("ENTER SERVICE MAN ADDRESS::");
	fflush(stdin); gets(serviceman_address);

	sprintf(buffer, "%s/%d/%s/%s/%s/%s/", "CALENDER", category, "ADD", serviceman_name, serviceman_number, serviceman_address);
	strcpy(responce, process_message(buffer));

	puts(responce);

}

void calender_selectserviceman(int category)
{
	int user_offset;
	char *serviceman_name = allocate_string_memory(data_size);
	char *serviceman_number = allocate_string_memory(data_size);
	char *serviceman_address = allocate_string_memory(2 * data_size);
	char *responce = allocate_string_memory(data_size);
	char *date = allocate_string_memory(data_size);
	char *buffer;


	printf("ENTER SERVICE MAN NAME::");
	fflush(stdin); gets(serviceman_name);


	buffer = allocate_string_memory(buffer_size);
	sprintf(buffer, "%s/%d/%s/%s/", "CALENDER", category, "SELECT/NAME", serviceman_name);
	strcpy(responce, process_message(buffer));
	user_offset = atoi(responce);

	if (user_offset > 0)
	{
		buffer = allocate_string_memory(buffer_size);
		sprintf(buffer, "%s/%d/%s/%d/", "CALENDER", category, "SELECT/NUMBER", user_offset);
		strcpy(serviceman_number, process_message(buffer));

		buffer = allocate_string_memory(buffer_size);
		sprintf(buffer, "%s/%d/%s/%d/", "CALENDER", category, "SELECT/ADDRESS", user_offset);
		strcpy(serviceman_address, process_message(buffer));

		clrscr();

		printf("NAME : %s\nNUMBER : %s\nADDRESS : %s\n", serviceman_name, serviceman_number, serviceman_address);

		printf("ENTER DATE(DD-MM-YYYY)::");
		fflush(stdin); gets(date);

		buffer = allocate_string_memory(buffer_size);
		sprintf(buffer, "%s/%d/%s/%d/%s/", "CALENDER", category, "SELECT/DATE", user_offset, date);
		strcpy(responce, process_message(buffer));
		puts(responce);
	}
	else
		puts(responce);
}

void calender_home_page(int category)
{
	int option;

	while (1)
	{
		clrscr();
		calender_viewserviceman(category);

		printf("1.ADD SERVICE MAN\n2.SELECT SERVICE MAN\n3.RETURN\n");
		printf("ENTER YOUR OPTION::");
		scanf("%d", &option);

		if (option == 1)
			calender_addserviceman(category);
		else if (option == 2)
			calender_selectserviceman(category);
		else if (option == 3)
			break;
		else
			printf("PLEASE ENTER VALID OPTION.\n");
		printf("ENTER ANY KEY TO CONTINUE...");
		fflush(stdin); getchar();
	}
}


//////////////////////////////////////////////
//											//
//		    ***HEADER FUNCTIONS***			//
//											//
//////////////////////////////////////////////

void blob_store()
{
	char *user_name, *password;
	int option;

	while (1)
	{
		clrscr();

		user_name = allocate_string_memory(data_size);
		password = allocate_string_memory(data_size);

		printf("WELCOME TO BLOB STORE\n");
		printf("1.LOGIN\n2.SIGNUP\n3.RETURN\n");
		printf("ENTER YOUR OPTION::");
		scanf("%d", &option);

		if (option == 1)
		{
			clrscr();
			printf("ENTER USERNAME::");
			fflush(stdin); gets(user_name);
			printf("ENTER PASSWORD::");
			fflush(stdin); gets(password);

			sprintf(buffer, "%s/%s/%s/%s/", "BLOB","LOGIN", user_name, password);
			responce = process_message(buffer);
			if (!strcmp(responce, "VALID USER"))
				blob_home_page(user_name);
			else
				puts(responce);
		}
		else if (option == 2)
		{
			clrscr();
			printf("ENTER USERNAME::");
			fflush(stdin); gets(user_name);
			printf("ENTER PASSWORD::");
			fflush(stdin); gets(password);

			sprintf(buffer, "%s/%s/%s/%s/", "BLOB", "SIGNUP", user_name, password);
			responce = process_message(buffer);
			puts(responce);
		}
		else if (option == 3)
			break;
		else
			printf("PLEASE ENTER VALID OPTION.\n");
		printf("ENTER ANY KEY TO CONTINUE...");
		fflush(stdin); getchar();
	}
}

void message_store()
{
	char *user_name, *password;
	int option;

	while (1)
	{
		clrscr();

		printf("WELCOME TO MESSAGE STORE\n");
		printf("1.LOGIN\n2.SIGNUP\n3.RETURN\n");
		printf("ENTER YOUR OPTION::");
		scanf("%d", &option);

		if (option == 1)
		{
			clrscr();

			user_name = allocate_string_memory(data_size);
			password = allocate_string_memory(data_size);
			printf("ENTER USERNAME::");
			fflush(stdin); gets(user_name);
			printf("ENTER PASSWORD::");
			fflush(stdin); gets(password);

			sprintf(buffer, "%s%s%s%s%s", "MESSAGE/LOGIN/", user_name, "/", password, "/");
			responce = process_message(buffer);
			if (!strcmp(responce, "VALID USER"))
				message_home_page(user_name);
			else
				puts(responce);
		}
		else if (option == 2)
		{
			clrscr();

			user_name = allocate_string_memory(data_size);
			password = allocate_string_memory(data_size);
			printf("ENTER USERNAME::");
			fflush(stdin); gets(user_name);
			printf("ENTER PASSWORD::");
			fflush(stdin); gets(password);

			sprintf(buffer, "%s%s%s%s%s", "MESSAGE/SIGNUP/", user_name, "/", password, "/");
			responce = process_message(buffer);
			puts(responce);
		}
		else if (option == 3)
			break;
		else
			printf("PLEASE ENTER VALID OPTION.\n");
		printf("ENTER ANY KEY TO CONTINUE...");
		fflush(stdin); getchar();
	}
}


void calender_store()
{
	int category;

	while (1)
	{
		clrscr();

		printf("WELCOME TO CALENDER STORE\n");
		printf("1.DOCTOR\n");
		printf("2.ENGINEER\n");
		printf("3.ARCHITECH\n");
		printf("4.TEACHER\n");
		printf("5.SOFTWARE ENGINEER\n");
		printf("6.MECHANIC\n");
		printf("7.LECTURER\n");
		printf("8.FARMER\n");
		printf("9.STORE MANAGER\n");
		printf("10.LAWYER\n");
		printf("0.RETURN\n");
		printf("ENTER YOUR OPTION::");
		scanf("%d", &category);

		if (category >= 1 && category <= 10)
			calender_home_page(category);
		else if (category == 0)
			break;
		else
			printf("PLEASE ENTER VALID OPTION.\n");
		printf("ENTER ANY KEY TO CONTINUE...");
		fflush(stdin); getchar();
	}
}


void main_page()
{
	int option;
	char *result;
	while (1)
	{
		clrscr();
		printf("WELCOME TO HAMARAGAR.COM\nWE HAVE THREE STORES AVALIABLE HERE\n1.BLOB STORE\n2.MESSAGE STORE\n3.CALENDER STORE\n4.EXIT\n");
		printf("ENTER YOUR OPTION::");
		scanf("%d", &option);
		if (option == 1)
			blob_store();
		else if (option == 2)
			message_store();
		else if (option == 3)
			calender_store();
		else if (option == 4)
			exit(0);
		else
			printf("PLEASE ENTER VALID OPTION.\n");
		printf("ENTER ANY KEY TO CONTINUE...");
		fflush(stdin); getchar();
	}
}


//////////////////////////////////////////////
//											//
//	    ***CLIENT CORE FUNCTIONS***		    //
//											//
//////////////////////////////////////////////

void socket_client()
{

	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0 || (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2))
	{
		fprintf(stderr, "Could not find sock dll %d\n", WSAGetLastError());
		return;
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = inet_addr(host_name);

	main_page();

}