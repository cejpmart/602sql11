// cache9.h
CFNC DllKernel BOOL remap_init(view_dyn * inst, BOOL remaping);
CFNC DllKernel BOOL remap_expand(view_dyn * inst, trecnum extstep);
CFNC DllKernel trecnum WINAPI  inter2exter(view_dyn * inst, trecnum inter);
CFNC DllKernel trecnum WINAPI  exter2inter(view_dyn * inst, trecnum exter);
CFNC DllKernel BOOL remap_delete(view_dyn * inst, trecnum inter);
CFNC DllKernel void remap_insert(view_dyn * inst, trecnum exter);
CFNC DllKernel void WINAPI remap_reset(view_dyn * inst);
