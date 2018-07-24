#include "ctlslib.h"

int max(a, b)
int a, b;
	{
	if (a < b) return b;
	else if (a > b) return a;
	else return a;
	}

int min(a, b)
int a, b;
	{
	if (a < b) return a;
	else return b;
	}

double dmax(a, b)
double a, b;
	{
	if (a < b) return b;
	else return a;
	}

double dmin(a, b)
double a, b;
	{
	if (a < b) return a;
	else return b;
	}




