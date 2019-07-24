#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
//#include <strsafe.h>

#define BUF_SIZE 100

const char PDF_HEADER[] = "%PDF-";
const char PDF_FOOTER[] = "%%EOF";


int ExtractPDF(char *filepath)
{
	HANDLE pdf_file = NULL, pkp_file = NULL;
	char *pdf_path, *unzip_path, *unzip_cmd;
	int path_size = 0, return_val = 0;
	DWORD num_bytes_read, num_bytes_write;
	char iobuffer[BUF_SIZE];
	int compare_state = 0, pdf_start = -1, pdf_end = -1;
 
	path_size = strlen(filepath);

	//throw an error if the extension is not .pkp to start with
	if (filepath[path_size-4] != '.' ||
		filepath[path_size-3] != 'p' ||
		filepath[path_size-2] != 'k' ||
		filepath[path_size-1] != 'p'  )
	{
		printf("Error: The file \"%s\" does does not have a .pkp extension.\n", filepath);
		return 1;
	}

	//create separate copy of path string
	pdf_path = malloc(sizeof(char) * path_size);
	if (pdf_path == NULL)
	{
		printf("ERROR: malloc() failed. Out of memory?\n");
		getc(stdin);
		exit(EXIT_FAILURE);
	}
	strcpy(pdf_path, filepath);

	//create path string to unzipped file
	unzip_path = malloc(sizeof(char) * path_size);
	if (unzip_path == NULL)
	{
		printf("ERROR: malloc() failed. Out of memory?\n");
		getc(stdin);
		exit(EXIT_FAILURE);
	}
	strcpy(unzip_path, pdf_path);
	memset(unzip_path+path_size-4, '\0', 4);

	//create commands strings to unzip the original pkp file
	unzip_cmd = malloc(sizeof(char) * (path_size + 7));
	sprintf(unzip_cmd, "7z x \"%s\"", pdf_path);

	//remove pkp extension from filename and replace with pdf
	printf("%s\n",unzip_cmd); fflush(stdout);
	pdf_path[path_size-3] = 'p';
	pdf_path[path_size-2] = 'd';
	pdf_path[path_size-1] = 'f';

	system(unzip_cmd);	//unzip the original pkp file

	//open the .pkp file for reading
    pkp_file = CreateFile(unzip_path,            // file to open
                      	  GENERIC_READ,          // open for reading
                      	  FILE_SHARE_READ,       // share for reading
                      	  NULL,                  // default security
                     	  OPEN_EXISTING,         // existing file only
                      	  FILE_ATTRIBUTE_NORMAL, // normal file
                      	  NULL);

    if (pkp_file == INVALID_HANDLE_VALUE) 
    { 
        printf(TEXT("Error: unable to unzip and open \"%s\" for read.\n"), filepath);
        return_val = 1;
        goto PDF_EXTRACT_EXIT; 
    }

	//create new .pdf file and open for writing
    pdf_file = CreateFile(pdf_path,               // name of the write
                       	  GENERIC_WRITE,          // open for writing
                       	  0,                      // do not share
                      	  NULL,                   // default security
                      	  CREATE_NEW,             // create new file only
                     	  FILE_ATTRIBUTE_NORMAL,  // normal file
                     	  NULL);                  // no attr. template

    if (pdf_file == INVALID_HANDLE_VALUE) 
    { 
        printf(TEXT("Error: Unable to create file \"%s\" for write.\n"), pdf_path);
        return_val = 1;
        goto PDF_EXTRACT_EXIT; 
    }

	//transfer PDF data from .pkp to new .pdf file
	while(pdf_end < 0)
	{
    	memset(iobuffer, '\0', BUF_SIZE);
	    if (FALSE == ReadFile(pkp_file, iobuffer, (DWORD)BUF_SIZE-1, &num_bytes_read, NULL))
	    {
	    	printf("Error: Could not read from \"%s\".\n", filepath);
	    	return_val = 1;
        	goto PDF_EXTRACT_EXIT; 
	    }

	    //loop does character by character comparison and updates
	    //the state variables to indicate when it found header/footer
	    for (int i = 0; i < num_bytes_read; i++)
	    {
	    	if (compare_state >= 10)
	    		break;

	    	switch(compare_state)
	    	{
	    		case 0:
	    			if (iobuffer[i] == PDF_HEADER[0])
	    				compare_state++;
	    			break;
	    		case 1: 
	    			if (iobuffer[i] == PDF_HEADER[1])
	    				compare_state++;
	    			else
	    				compare_state = 0;
	    			break;
	    		case 2: 
	    			if (iobuffer[i] == PDF_HEADER[2])
	    				compare_state++;
	    			else
	    				compare_state = 0;
	    			break;
	    		case 3: 
	    			if (iobuffer[i] == PDF_HEADER[3])
	    				compare_state++;
	    			else
	    				compare_state = 0;
	    			break;
	    		case 4: 
	    			if (iobuffer[i] == PDF_HEADER[4])
	    			{
	    				compare_state++;
	    				pdf_start = i+1;
	    			}
	    			else
	    				compare_state = 0;
	    			break;
	    		case 5:
	    			if (iobuffer[i] == PDF_FOOTER[0])
	    				compare_state++;
	    			break;
	    		case 6:
	    			if (iobuffer[i] == PDF_FOOTER[1])
	    				compare_state++;
	    			else
	    				compare_state = 5;
	    			break;
	    		case 7:
	    			if (iobuffer[i] == PDF_FOOTER[2])
	    				compare_state++;
	    			else
	    				compare_state = 5;
	    			break;
	    		case 8:
	    			if (iobuffer[i] == PDF_FOOTER[3])
	    				compare_state++;
	    			else
	    				compare_state = 5;
	    			break;
	    		case 9:
	    			if (iobuffer[i] == PDF_FOOTER[4])
	    			{
	    				compare_state++;
	    				pdf_end = i+1;
	    			}
	    			else
	    				compare_state = 5;
	    			break;
	    		default:
	    			compare_state = 0;
	    	} //switch
	    } //for

	    printf("cs: %d start: %d end: %d\n", compare_state, pdf_start, pdf_end);
	    if (pdf_end < 0)
	    {
		    if (pdf_start == -2)
		    {
		    	if (FALSE == WriteFile(pdf_file, iobuffer, num_bytes_read, &num_bytes_write, NULL))
			    {
			    	printf("Error: Could not write to \"%s\".\n", pdf_path);
			    	return_val = 1;
       				goto PDF_EXTRACT_EXIT; 
			    }
		    }
		    else if (pdf_start >= 0)
		    {
			    if (FALSE == WriteFile(pdf_file, PDF_HEADER, 5, &num_bytes_write, NULL))
			    {
			    	printf("Error: Could not write to \"%s\".\n", pdf_path);
			    	return_val = 1;
        			goto PDF_EXTRACT_EXIT; 
			    }
			    if (FALSE == WriteFile(pdf_file, iobuffer+pdf_start, num_bytes_read-pdf_start, &num_bytes_write, NULL))
			    {
			    	printf("Error: Could not write to \"%s\".\n", pdf_path);
		    		return_val = 1;
        			goto PDF_EXTRACT_EXIT; 
			    }
			    pdf_start = -2;
			}
		}
		else
		{
			if (pdf_start > 0)
			{
			    if (FALSE == WriteFile(pdf_file, PDF_HEADER, 5, &num_bytes_write, NULL))
			    {
			    	printf("Error: Could not write to \"%s\".\n", pdf_path);
			    	return_val = 1;
        			goto PDF_EXTRACT_EXIT; 
			    }
			    if (FALSE == WriteFile(pdf_file, iobuffer+pdf_start, pdf_end-pdf_start, &num_bytes_write, NULL))
			    {
			    	printf("Error: Could not write to \"%s\".\n", pdf_path);
			    	return_val = 1;
        			goto PDF_EXTRACT_EXIT; 
			    }
			}
			else if (pdf_start == -2)
			{
			    if (FALSE == WriteFile(pdf_file, iobuffer, pdf_end, &num_bytes_write, NULL))
			    {
			    	printf("Error: Could not write to \"%s\".\n", pdf_path);
			    	return_val = 1;

        			goto PDF_EXTRACT_EXIT; 
			    }
			}
			//This state should never happen in theory
			else
			{
				printf("Error: Could not find the start of the PDF data.\n");
				return_val = 1;

        		goto PDF_EXTRACT_EXIT; 
			}
			break;
		}

		//if we reach the end of the file without finding the end of the PDF
	    if (num_bytes_read < BUF_SIZE-1)
	    {
			printf("Error: Could not find PDF data.\n");
			return_val = 1;
        	goto PDF_EXTRACT_EXIT; 
	    }
	}


PDF_EXTRACT_EXIT:

	if (pkp_file)
		CloseHandle(pkp_file);

	if (remove(unzip_path) == -1)				//delete the unzipped file
		printf("Couldn't remove unzipped file \"%s\"\n", unzip_path);

	if (pdf_file)
		CloseHandle(pdf_file);

	if (return_val > 0)
		if (remove(pdf_path) == -1)
			printf("Couldn't remove PDF file \"%s\"\n", pdf_path);
		

	free(pdf_path);
	free(unzip_path);
	free(unzip_cmd);

	return return_val;
}


int main (int argc, char **argv)
{
	int num_failures = 0;

	for (int i = 1; i < argc; i++)
	{
		num_failures += ExtractPDF(argv[i]);
	}

	if (num_failures)
	{
		printf("\nFailures: %d. Press enter to close...\n", num_failures);
		getc(stdin);
	}
	else
	{
		printf("Success!\n");
	}

	return num_failures;
}