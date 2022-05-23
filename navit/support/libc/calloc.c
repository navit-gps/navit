void *
calloc(int nelem, int size)
{
	void *ret=malloc(nelem*size);
	if (ret) 
		memset(ret, 0, nelem*size);
	return ret;
}
