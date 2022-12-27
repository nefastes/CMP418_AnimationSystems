#pragma once
#include <string>
#include <system/file.h>


char* LoadJSON(const char* filename)
{
	std::string json_filename(filename);
	char* json_file_data = NULL;
	Int32 file_size = 0;
	gef::File* file = gef::File::Create();

	bool success = true;
	success = file->Open(json_filename.c_str());
	if (success)
	{
		success = file->GetSize(file_size);
		if (success)
		{
			// going to allocate an extra byte so we can NULL terminate the string
			json_file_data = (char*)malloc(file_size + 1);
			success = json_file_data != NULL;
			if (success)
			{
				Int32 bytes_read;
				success = file->Read(json_file_data, file_size, bytes_read);
				if (success)
					success = bytes_read == file_size;
			}
		}
		file->Close();
		delete file;
		file = NULL;
	}

	// NULL terminate the JSON string
	if (success && (file_size > 0) && json_file_data)
		json_file_data[file_size] = 0;


	return json_file_data;
}