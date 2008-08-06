// stackwalk.h
bool stackwalk_prepare(void);
void write_frames(HANDLE hThread, CONTEXT& c, int clinum, int fixnum);
void proc_prep(void);
void close_process(void);


