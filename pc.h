static HANDLE g_job = NULL;

static void init_job_object();
PROCESS_INFORMATION run_process(const char* program, const char* args);
void kill_process(PROCESS_INFORMATION pi);