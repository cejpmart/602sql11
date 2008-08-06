class t_input_processor
{ bool composing_metastatement;
  bool escaped;  // just after escaping '\'
  char quote;    // inside quoted string demited by [quote]
  std::string metastatement;
  std::string sql_statement;
  void process_metastatement(void);
  void process_sql_statement(void);
  void update_scanning_state(char c);

 public: 
  t_input_processor(void)
    { composing_metastatement=false;  metastatement="";  sql_statement=""; 
      escaped=false;  quote=0;
    }
  bool continuing_sql(void) const
  { const unsigned char * p = (const unsigned char *)sql_statement.c_str(); 
    while (*p)
      if (*p > ' ') return true;
      else p++;
    return false;  
  }
  void process_input(const char * input);
  void go(void);
  void reset(void);
  void print(void);

  void handle_help(char *topic);
  void handle_schema(char * params);
  void handle_status(char *);
  void handle_connect(char * params);
  void handle_servers(char *cmd);
  void handle_quit(char * params);
  void handle_echo(char * arg);
  void handle_command_file(char * params);
  void handle_output_file(char * params);
  void handle_pset(char * params);
  void handle_aligned(char * params);
  void handle_fieldsep(char * sep);
  void handle_recordsep(char * sep);
  void handle_expanded(char * params);
  void handle_tuples(char * params);
  void handle_print(char * params);
  void handle_reset(char * params);
  void handle_go(char * params);
  void handle_edit(char * params);
  void handle_displ_tables(char * params);
  void handle_displ_queries(char * params);
  void handle_displ_schemas(char * params);
  void handle_displ_users(char * params);
  void handle_displ_groups(char * params);
  void handle_import(char * params);
  void handle_export(char * params);
};


struct command_t {  // table of builtin commands
	const char * name;  // String compared with the entered command
	void (t_input_processor::*action)(char * params);  // Command to execute, with rest of the line as textual parameter
	const char * help;
};

extern struct command_t commands_list[];

void show_servers(char *cmd);
void show_status(void);

void process_command_file(char * fname, bool report_missing_file);
void set_output_file(char * fname);

bool do_connect(void);
bool do_schema(char * schema_name);

bool process_assignment(char * name, const char * value, bool feedback);

