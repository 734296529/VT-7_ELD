
#ifndef __STOPMODE_H_
#define	__STOPMODE_H_

#include <stdint.h>
#include <stdbool.h> 
#include "../Common/common.h"

#ifdef	__cplusplus
extern "C" {
#endif 

#define  ON   1
#define  OFF  0
	
void Enter_StopMode(void);
void Sleep_Manage(void);
void Sleep_Start(void);
void Sleep_Stop(void);

	
#ifdef	__cplusplus
}
#endif

#endif	/* __STOPMODE_H_ */
