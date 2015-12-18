
#define PLAY_TO_MEMORY	0
#define PLAY_TO_DECODE	1

int get_program_count(int fd_pat);
int start_program(int fd_pmt, int fd_a,
				  int fd_v, int fd_pcr, int uProgNumber, int flag);
