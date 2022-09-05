#include  <stdio.h>



void	JSONTest(char *fname, void *cb) {
	FILE	*fp = fopen(fname, "r");
	JSONParser	*jsp;
	char	data[2];
	int		r = 0;

	jsp = startJSON("test");
	if (!jsp)
		return;
	if (!fp)
		return;
	data[1] = 0;
	jsp->cb = cb;
	for (int c = 0; (c = fgetc(fp)) != EOF; ) {
		data[0] = c;
		if ((r = processJSON(jsp, data, 1)) != JSON_CONTINUE)
			break;
	}
	LogPrint("Result: %d\r\n", r);
	fclose(fp);
}

int	jsoncb(JSONParser *j, int cmd, char *value) {
	char	nm[512];
	switch (cmd){
	case JP_NAME_VALUE:
		PrintName(j, nm);
		LogPrint("%s = \"%s\"\r\n", nm, value);
		break;
	}
	return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage %s <json file name>\r\n", argv[0]);
    return 1;
  }
  JSONTest(argv[1], jsoncb);
  
}
