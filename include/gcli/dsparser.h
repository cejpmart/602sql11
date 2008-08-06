
#if 0
CFNC DllXMLExt BOOL WINAPI DSParserOpen(cdp_t cdp, HANDLE *pHandle);
CFNC DllXMLExt void WINAPI DSParserClose(HANDLE Handle);
CFNC DllXMLExt BOOL WINAPI DSParserSetStrProp(HANDLE Handle, DSPProp Prop, const char *Value);
CFNC DllXMLExt BOOL WINAPI DSParserSetIntProp(HANDLE Handle, DSPProp Prop, int Value);
CFNC DllXMLExt BOOL WINAPI DSParserGetStrProp(HANDLE Handle, DSPProp Prop, const char **Value);
CFNC DllXMLExt BOOL WINAPI DSParserGetIntProp(HANDLE Handle, DSPProp Prop, int *Value);
CFNC DllXMLExt BOOL WINAPI DSParserParse(HANDLE Handle, const char *Source);
CFNC DllXMLExt BOOL WINAPI DSParserGetObjectName(HANDLE Handle, int Index, char *Name, tcateg *Categ);
CFNC DllXMLExt BOOL WINAPI DSParserGetColumnProps(HANDLE Handle, const char *TableName, int ColumnIndex, char *ColumnName, int *Type, t_specif *Specif, int *Exclude);
CFNC DllXMLExt BOOL WINAPI DSParserSetColumnProps(HANDLE Handle, const char *TableName, const char *ColumnName, int Type, t_specif Specif, bool Exclude);
CFNC DllExport BOOL WINAPI DSParserGetTableExcludeFlag(HANDLE Handle, const char *TableName, bool *Exclude);
CFNC DllExport BOOL WINAPI DSParserSetTableExcludeFlag(HANDLE Handle, const char *TableName, bool Exclude);
CFNC DllXMLExt BOOL WINAPI DSParserCreateObjects(HANDLE Handle);
CFNC DllXMLExt BOOL WINAPI DSParserGetLastError(HANDLE Handle, int *ErrCode, const char **ErrMsg);
CFNC DllXMLExt MFDState WINAPI MatchFormAndDAD(cdp_t cdp, const char *Form, const char *DAD);
#endif


