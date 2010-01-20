gcc string.c dbf_head.c dbf_ndx.c dbf_misc.c dbf_rec.c luadbase.c -shared -O2 -Wall -L/usr/local/lib -llua -ldl -lm -lnsl -o dbase.so
