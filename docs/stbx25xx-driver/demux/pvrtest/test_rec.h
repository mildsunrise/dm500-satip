
#define PLAY_TO_MEMORY	0
#define PLAY_TO_DECODE	1
#define PLAY_TO_BUCKET  2
#define PLAY_TO_BUCKET_ONLY  3
#define PLAY_TO_MEMORY_ONLY 4

typedef struct {
                  unsigned short program;
		  unsigned short dmxid;
		  unsigned short pmt_pid;
		  unsigned short aud_pid;
		  unsigned short vid_pid;
		  unsigned short pcr_pid;
		} PROGRAM;
typedef struct {
		unsigned short programcount;
                unsigned short dmxid;
		unsigned short net_pid;
		PROGRAM prog[0];
	      } PRGUIDE;

int start_programx(PROGRAM *prog, int fd_v, int fd_a, int fd_pcr, int flag);
int get_program_info(PROGRAM *prog);
int get_pat(int dmxid, PRGUIDE **p);

