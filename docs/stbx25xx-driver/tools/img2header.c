/*----------------------------------------------------------------------------+
|       This source code has been made available to you by IBM on an AS-IS
|       basis.  Anyone receiving this source is licensed under IBM
|       copyrights to use it in any way he or she deems fit, including
|       copying it, modifying it, compiling it, and redistributing it either
|       with or without modifications.  No license under IBM patents or
|       patent applications is to be implied by the copyright license.
|
|       Any user of this software should understand that IBM cannot provide
|       technical support for this software and will not be responsible for
|       any consequences resulting from the use of this software.
|
|       Any person who transfers this source code or any derivative work
|       must include the IBM copyright notice, this paragraph, and the
|       preceding two paragraphs in the transferred software.
|
|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/

#include <stdio.h>
#include <ctype.h>


#define DATA_VAR_ID    "__local_data"
#define SIZE_VAR_ID    "__local_data_size"
#define LINE_WIDTH	   20

const char IBM_Logo[] = 
{
"/*----------------------------------------------------------------------------+\n"           
"|       This source code has been made available to you by IBM on an AS-IS     \n"
"|       basis.  Anyone receiving this source is licensed under IBM             \n"
"|       copyrights to use it in any way he or she deems fit, including         \n"
"|       copying it, modifying it, compiling it, and redistributing it either   \n"
"|       with or without modifications.  No license under IBM patents or        \n"
"|       patent applications is to be implied by the copyright license.         \n"
"|                                                                              \n" 
"|       Any user of this software should understand that IBM cannot provide    \n"
"|       technical support for this software and will not be responsible for    \n"
"|       any consequences resulting from the use of this software.              \n"
"|                                                                              \n" 
"|       Any person who transfers this source code or any derivative work       \n" 
"|       must include the IBM copyright notice, this paragraph, and the         \n"
"|       preceding two paragraphs in the transferred software.                  \n"
"|                                                                              \n" 
"|       COPYRIGHT   I B M   CORPORATION 1997, 1999, 2001                       \n" 
"|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M                        \n"
"+----------------------------------------------------------------------------*/\n"
};



void usage(char *myname)
{
	printf( "Binary Image to C header converter v1.0,  by Yang Yudong, yangyud@cn.ibm.com\n"
	            "Usage:%s [options] input output\n"
		    "Options:\n"
		    "  -d  data_varaible_id  (default: " DATA_VAR_ID " )\n"
		    "  -n  size_varaible_id  (default: " SIZE_VAR_ID " )\n"
		    "  -s  define as static  (default: no )\n"
		    "  -w  num char per line (default: %d )\n"
		    "  -x  hex dump (default: no )\n"
		    "  -h, -?   Get help of commandline args\n"
		    , myname, LINE_WIDTH);
}


int main(int argc, char **argv)
{
	int i, c, w;
	FILE *fpinput = NULL;
	FILE *fpoutput = NULL;
	int out_line_width = LINE_WIDTH;
	int is_static = 0, is_hexdump = 0;
	char *pdataId = DATA_VAR_ID;
	char *psizeId = SIZE_VAR_ID;

	char *pInputName=NULL;
	char *pOutputName=NULL;


	for(i=1; i<argc; i++) {
		if('-' == argv[i][0])
			switch(argv[i][1]) {
			case 'h':
			case '?':
				usage(argv[0]);
				return 0;
			case 'd':
				if(i+1 < argc && (isalnum(argv[i+1][0]) || '_' == argv[i+1][0])) {
				   pdataId = argv[i+1];
				   i++;
				}
				else {
					usage(argv[0]);
					return 1;
				}
				break;
			case 'n':
				if(i+1 < argc && (isalnum(argv[i+1][0]) || '_' == argv[i+1][0])) {
				   psizeId = argv[i+1];
				   i++;
				}
				else {
					usage(argv[0]);
					return 1;
				}
				break;
			case 'w':
				if(i+1 < argc && isdigit(argv[i+1][0])) {
				   out_line_width = atoi(argv[i+1]);
				   i++;
				   if(out_line_width <= 0) {
					  usage(argv[0]);
					  return 1;
				   }
				}
				else {
					usage(argv[0]);
					return 1;
				}
				break;
			case 's':
				is_static = 1;
				break;
			case 'x':
				is_hexdump = 1;
				break;
			default:
				usage(argv[0]);
				return 1;
			}
		else {
			if(NULL == pInputName)  pInputName = argv[i];
			else if(NULL == pOutputName) pOutputName = argv[i];
		}
			
	}

	if(NULL == pInputName)  fpinput = stdin;
	else {
		fpinput = fopen(pInputName, "rb");
		if(NULL == fpinput) {
			fprintf(stderr, "%s:Error, cannot open input file '%s'.\n", argv[0], pInputName);
			return 1;
		}
	}

	if(NULL == pOutputName)  fpoutput = stdout;
	else {
		fpoutput = fopen(pOutputName, "w");
		if(NULL == fpoutput) {
			fprintf(stderr, "%s:Error, cannot open output file '%s'.\n", argv[0], pOutputName);
			return 1;
		}
	}

	//IBM logo dump is added by shaol
	fprintf(fpoutput, "%s\n", IBM_Logo);
	fprintf(fpoutput, "%s char %s[]= {\n", is_static ? "static" : "", pdataId);
	i=0; w= 0;
	if(is_hexdump)
		while((c = fgetc(fpinput)) != EOF) {
			fprintf(fpoutput,  "0x%02x, ", (unsigned char)c);
			i++; w++;
			if(w >= out_line_width) { fprintf(fpoutput, "\n");  w=0; }
		}
	else
		while((c = fgetc(fpinput)) != EOF) {
			fprintf(fpoutput,  "%3d, ", (unsigned char)c);
			i++; w++;
			if(w >= out_line_width) { fprintf(fpoutput, "\n");  w=0; }
		}

	fprintf(fpoutput, "\n};\n%s unsigned int %s = %d;\n", is_static ? "static" : "", psizeId, i);

	w = ferror(fpinput) | ferror(fpoutput);

	if(stdout != fpoutput)  fclose(fpoutput);
	if(stdin != fpinput) fclose(fpinput);

	return 0 != w;
}

