
/****************************************************************************/
/* (C) Janus Drozd, 1992-2008                                               */
/* verze: 0.0 (32-bit)                                                      */
/****************************************************************************/
#ifndef __CDP_H__
#define __CDP_H__

#define CDP_SIZE_BYTES 20000
typedef struct cd_t {
    unsigned char internal_data[CDP_SIZE_BYTES];
} cd_t;
typedef cd_t *cdp_t;
typedef cd_t *t_pconnection;
typedef cd_t *xcdp_t;


#define CDP_SIZE CDP_SIZE_BYTES

#endif  /* !def __CDP_H_ */

