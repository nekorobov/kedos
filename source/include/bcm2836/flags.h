#ifndef _FLAGS_
#define _FLAGS_

#define RBUFFER_IS_FULL (1)
#define RBUFFER_IS_EMPTY (1 << 1)

#define R_NONBLOCK (1)
#define R_WAITFROM (1 << 1)
#define R_SENDTO   (1 << 2)

#endif 
