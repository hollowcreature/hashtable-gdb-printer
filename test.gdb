set pagination off
set print address off
source printer.py
tbreak table_remove
run > /dev/null
print *t
dump-table t
break table_free
continue
print *t
dump-table t
quit