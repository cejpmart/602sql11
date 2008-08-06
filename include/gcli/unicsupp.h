// unicsupp.h

wxString Get_error_num_textWX(xcdp_t xcdp, int err);
wxString bin2hexWX(const uns8 * data, unsigned len);
void WXUpcase(cdp_t cdp, wxString & s);


#if wxUSE_UNICODE


#else


#endif
