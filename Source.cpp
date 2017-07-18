#include"Header.h"

int main()
{
	FILE *fs;
	fopen_s(&fs, "file.dat", "rb+");
	login(fs);

	return 0;
}