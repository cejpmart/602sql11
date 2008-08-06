// Network API functions common for client and server.

/***********************            Send            *************************/
// The Send function is the lowest protocol independent level of sending
// Other sending function only transform the parameters. 
// Sending arrays of buffers whose data total exceeds the packet size is allowed
//  (subdividing into packets done on the protocol-dependent level).

BOOL Send(cAddress * pAddr, t_fragmented_buffer * frbu, BYTE bDataType)
{ unsigned DataSent, total_length;
  if (!frbu) // sending only a header (DT_BREAK, DT_SERVERDOWN)
  { t_fragmented_buffer empty_frbu(NULL, 0);
    empty_frbu.reset_pointer();
    return pAddr->Send(&empty_frbu, bDataType, 0, DataSent);
  }
  else
  { total_length = frbu->total_length();
    frbu->reset_pointer();
    for (unsigned DataTotalSent = 0;  DataTotalSent < total_length;  DataTotalSent += DataSent)
      if (!pAddr->Send(frbu, bDataType, total_length, DataSent))
        return FALSE;
    return TRUE;
  }
}

