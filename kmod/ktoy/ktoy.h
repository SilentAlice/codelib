#ifndef _KTOY_H_
#define _KTOY_H_

#define KTOY_IOC_MAGIC 0xCA

#define KTOY_IOC_RESET _IO(KTOY_IOC_MAGIC, 0)

#define KTOY_IOC_SET _IOW(KTOY_IOC_MAGIC, 1, int)
#define KTOY_IOC_GET _IOR(KTOY_IOC_MAGIC, 2, int)

#define KTOY_IOC_MAXNR 2

#endif /* _KTOY_H_ */
