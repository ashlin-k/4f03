/* append.x: Remote appending protocol */

struct append_arg 
{
	unsigned int f;
	unsigned int l;
	unsigned int m;
	char c0;
	char c1;
	char c2;
	string hostname2<>;
};

program APPENDPROG { version APPENDVERS { 
	int RPC_INITAPPENDSERVER(append_arg) = 1;
	int RPC_APPEND(char) = 2; 
} = 1; } = 0x01220399;