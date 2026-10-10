set pagination off
source printer.py
break table_remove
run > /dev/null
print *t
dump-table t
quit