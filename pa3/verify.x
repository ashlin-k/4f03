/* verify.x: Remote verification protocol */

struct verify_arg
{
	unsigned int n;
	unsigned int l;
	unsigned int m;
};

program VERIFYPROG { version VERIFYVERS { 
	int RPC_INITVERIFYSERVER(verify_arg) = 1;
	string RPC_GETSEG(long) = 2; 
	string RPC_GETSTRING(void) = 3;
} = 1; } = 0x11220399;