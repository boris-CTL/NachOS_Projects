#include "syscall.h"

main()
	{
		int	m;
		for (m = 10; m < 14; m++){
			PrintInt(m);
			Sleep(4000000); 
			PrintInt(m*m);
		}
	}
