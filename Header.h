#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define block_size 128
#define file_start 1270

int file_id = 0;
int file_no = 0;
char user[10];

struct file
{
	char filename[20];
	char username[10];
	int fileid;
	int file_offset;
	int blob_addr;
	int message_addr;
	int next;
	char pad[78]; // 128-44
};
struct msg_metadata
{
	char username[10];
	int msg_addr;
	int next;
	int avail;
	char pad[106];
};
struct msg
{
	char message[128];
};
struct blob
{
	char chunck[120];
	int end;
	int next;
};
struct bitmap
{
	char bit[128];
}key;

void clear_buffer()
{
	fflush(stdin);
}
int getFreeBlock(FILE *fs)
{
	int found = 1, offset = 0, rtn = 0;
	fseek(fs, offset, SEEK_SET);
	for (int j = 0; j < 1270 && found == 1; j++)
	{
		fread(&key, block_size, 1, fs);
		for (int i = 0; i < 128; i++)
		{
			if (key.bit[i] != '1')
			{
				key.bit[i] = '1';
				found = 0;
				rtn = offset + i;
				break;
			}
		}
		offset += block_size;
	}
	fseek(fs, offset - block_size, SEEK_SET);
	fwrite(&key, block_size, 1, fs);
	return rtn;
}
int getBlob_Address(FILE *blob_file, FILE *fs)
{
	if (feof(blob_file))
		return -1;
	blob blob_var;
	int total_chars = fread(&blob_var.chunck, 1, sizeof(blob_var.chunck), blob_file);
	if (total_chars < sizeof(blob_var.chunck))
		blob_var.chunck[total_chars] = '\0';
	blob_var.end = total_chars;
	int free = getFreeBlock(fs);
	blob_var.next = getBlob_Address(blob_file, fs);
	fseek(fs, (free + file_start) * block_size, SEEK_SET);
	fwrite(&blob_var, block_size, 1, fs);
	return free;
}
int insertFile(FILE *fs)
{
	file obj;
	printf("Enter File Name : ");
	scanf("%s", obj.filename);
	obj.fileid = file_id++;
	obj.file_offset = getFreeBlock(fs);
	strcpy(obj.username, user);
	obj.message_addr = -1;
	obj.next = -1;
	FILE *blob;
	char location[30];
	scanf("%s", location);
	fopen_s(&blob, location, "rb");

	if (blob)
	{
		obj.blob_addr = getBlob_Address(blob, fs);
		fseek(fs, (obj.file_offset + file_start) * block_size, SEEK_SET);
		fwrite(&obj, block_size, 1, fs);
		return obj.file_offset;
	}
	else
	{
		printf("Error : Cant Open File...!!");
		return -1;
	}
}
void createNewFile(FILE *fs)
{
	fseek(fs, 0, SEEK_SET);
	fread(&key, block_size, 1, fs);
	if (key.bit[0] == '1')
	{
		file ptr;
		int block = 0;
		fseek(fs, (block + file_start)*block_size, SEEK_SET);
		fread(&ptr, block_size, 1, fs);
		while (ptr.next != -1)
		{
 			block = ptr.next;
			fseek(fs, (block + file_start)*block_size, SEEK_SET);
			fread(&ptr, block_size, 1, fs);
		}
		ptr.next = insertFile(fs);
		fseek(fs, (block + file_start)*block_size, SEEK_SET);
		fwrite(&ptr, block_size, 1, fs);
	}
	else
	{
		insertFile(fs);
	}
}
int insertMsg(FILE *fs)
{
	int free = getFreeBlock(fs);
	msg_metadata meta;
	int msg_free = getFreeBlock(fs);
	msg data;
	meta.msg_addr = msg_free;
	strcpy(meta.username, user);
	meta.next = -1;
	fseek(fs, (free + file_start)*block_size, SEEK_SET);
	fwrite(&meta, block_size, 1, fs);
	printf("Enter Message : ");
	clear_buffer();
	gets(data.message);
	fseek(fs, (msg_free + file_start)*block_size, SEEK_SET);
	fwrite(&data, block_size, 1, fs);
	return free;
}
void createNewMsg(FILE *fs, int file_offset)
{
	fseek(fs, (file_offset + file_start)*block_size, SEEK_SET);
	file ptr;
	fread(&ptr, block_size, 1, fs);

	if (ptr.message_addr == -1)
	{
		ptr.message_addr = insertMsg(fs);
		fseek(fs, (file_offset + file_start)*block_size, SEEK_SET);
		fwrite(&ptr, block_size, 1, fs);
	}
	else
	{
		msg_metadata mmd;
		int msg_offset = ptr.message_addr;
		fseek(fs, (msg_offset + file_start)*block_size, SEEK_SET);
		fread(&mmd, block_size, 1, fs);

		while (mmd.next != -1)
		{
			msg_offset = mmd.next;
			fseek(fs, (mmd.next + file_start)*block_size, SEEK_SET);
			fread(&mmd, block_size, 1, fs);
		}

		mmd.next = insertMsg(fs);
		fseek(fs, (msg_offset + file_start)*block_size, SEEK_SET);
		fwrite(&mmd, block_size, 1, fs);
	}
}
void store(int offset, FILE *fs, FILE *op)
{
	if (offset == -1)
		return;
	offset = (offset + file_start)*block_size;
	fseek(fs, offset, SEEK_SET);
	blob ptr;
	fread(&ptr, block_size, 1, fs);
	int size = sizeof(ptr.chunck);
	if (ptr.next == -1)
		size = ptr.end;
	fwrite(ptr.chunck, size, 1, op);
	store(ptr.next, fs, op);
}
void saveFile(FILE *fs, int file_offset)
{
	fseek(fs, (file_offset + file_start)*block_size, SEEK_SET);
	file ptr;
	fread(&ptr, block_size, 1, fs);
	FILE *output;
	fopen_s(&output, ptr.filename, "wb+");
	store(ptr.blob_addr, fs, output);
	fclose(output);
}
void getMsg(FILE *fs, int file_offset)
{
	fseek(fs, (file_offset + file_start)*block_size, SEEK_SET);
	file ptr;
	fread(&ptr, block_size, 1, fs);

	if (ptr.message_addr == -1)
	{
		printf("\nNo Messages..!!\n");
	}
	else
	{
		msg_metadata mmd;
		int msg_offset = ptr.message_addr;

		while (msg_offset != -1)
		{
			fseek(fs, (msg_offset + file_start)*block_size, SEEK_SET);
			fread(&mmd, block_size, 1, fs);

			msg data;
			fseek(fs, (mmd.msg_addr + file_start)*block_size, SEEK_SET);
			fread(&data, block_size, 1, fs);
			printf("%s : %s \n", mmd.username, data.message);
			msg_offset = mmd.next;
		}
	}
}
void accessFiles(FILE *fs)
{
	fseek(fs, 0, SEEK_SET);
	fread(&key, block_size, 1, fs);
	if (key.bit[0] == '1')
	{
		file ptr;
		file_no = 0;
		int block = 0;
		fseek(fs, (block + file_start)*block_size, SEEK_SET);
		fread(&ptr, block_size, 1, fs);
		while (ptr.next != -1)
		{
			printf("File : %d , %s\n", file_no++, ptr.filename);
			block = ptr.next;
			fseek(fs, (block + file_start)*block_size, SEEK_SET);
			fread(&ptr, block_size, 1, fs);
		}
		printf("File : %d , %s\n", file_no++, ptr.filename);

		printf("Enter File Id : ");
		int id = 0;
		fflush(stdin);
		scanf("%d", &id);

		file_no = 0;
		block = 0;
		fseek(fs, (block + file_start)*block_size, SEEK_SET);
		fread(&ptr, block_size, 1, fs);
		while (true)
		{
			if (file_no == id)
			{
				printf("File : %d , %s\n\n", file_no, ptr.filename);
				//\n2.Delete Message\n3.Show My Messages\n5.Delete File\n6.Update File\n
				printf("Options : \n1.Add Message\n4.Enum Messages\n7.Save File\n");
				int option;

				scanf("%d", &option);
				switch (option)
				{
				case 1:
					createNewMsg(fs, block);
					break;
				case 2:
					break;
				case 3:
					break;
				case 4:
					getMsg(fs, block);
					break;
				case 5:
					break;
				case 6:
					break;
				case 7:
					saveFile(fs, block);
					break;
				default:
					break;
				}
				break;
			}
			if (ptr.next == -1)
				break;
			file_no++;
			block = ptr.next;
			fseek(fs, (block + file_start)*block_size, SEEK_SET);
			fread(&ptr, block_size, 1, fs);
		}
	}
}
void deleteFiles(FILE *fs)
{

}
void login(FILE *fs)
{
	printf("Enter Username : ");
	scanf("%s", user);
	int Loop = 1;
	while (Loop)
	{
		printf("\nOptions : \n1.Insert File\n2.Open File\n3.Delete File\n4.Exit\n\nEnter option : ");
		int option; scanf("%d", &option);
		switch (option)
		{
		case 1:
			createNewFile(fs);
			break;
		case 2:
			accessFiles(fs);
			break;
		case 3:
			deleteFiles(fs);
			break;
		case 4:
			Loop = 0;
			break;
		}
	}
}