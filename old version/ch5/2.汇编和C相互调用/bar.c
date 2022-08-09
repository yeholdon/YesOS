void myprint(char* msg, int len);	// 同样，调用的本文件外的函数，要前置声明

int choose(int a, int b)
{
	if(a >= b){
		myprint("the 1st one\n", 13);
	}
	else{
		myprint("the 2nd one\n", 13);
	}

	return 0;
}
