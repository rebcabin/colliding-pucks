/*      Copyright (C) 1989, California Institute of Technology.
        U. S. Government Sponsorship under NASA Contract NAS7-918
        is acknowledged.        */


#define isalpha(c) ((c) >= 'a' && (c) <= 'z' || (c) >= 'A' && (c) <= 'Z')
#define isupper(c) ((c) >= 'A' && (c) <= 'Z')
#define islower(c) ((c) >= 'a' && (c) <= 'z')
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isxdigit(c) ((c)>='a'&&(c)<='f' || (c)>='A'&&(c)<='F' || isdigit(c))
#define isalnum(c) (isalpha(c) || isdigit(c))
#define isspace(c) ((c)==' ' || (c)=='\t' || (c)=='\n' || (c)=='\013'\
                   || (c)=='\f' || (c)=='\r')
#define iscntrl(c) ((c) >= '\0' && (c) < ' ' || (c) == '\177')
#define ispunct(c) (!(isalnum(c) || iscntrl(c) || isspace(c)))
#define isprint(c) ((c) > ' ' && (c) < '\177')
#define isascii(c) ((c) >= 0 && (c) <= '\177')
#define tolower(c) (isupper(c) ? ((c)+'a'-'A') : (c))
#define toupper(c) (islower(c) ? ((c)-'a'+'A') : (c))
