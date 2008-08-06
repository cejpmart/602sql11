/* Global variables */
#include <string>
extern std::string command_from_args, input_filename, output_filename;
extern bool command_from_args_specified;

extern bool verbose;
extern bool disable_config_files;
extern bool interactive, interactive_output;
extern bool disp_aligned;
extern bool expanded, tuples_only, pager;
extern std::string field_separator, record_separator, disp_null, 
                   date_format, time_format, timestamp_format, real_format;
extern int disp_border;

extern tobjname ServerName;
extern tobjname LoginName;
extern char Password[MAX_PASSWORD_LEN+1];
extern tobjname SchemaName;
extern const char *configfile;

/* Functions to read parameters */
extern bool processing_config_file, processing_command_line;
void process_config_files(void);
void preprocess_options_from_argv(int argc, char **argv);
void process_options_from_argv(int argc, char **argv);
extern bool terminate_after_processing_options;

